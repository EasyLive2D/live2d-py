#include "WarpDeformer.hpp"
#include "BinaryReader.hpp"
#include "PivotManager.hpp"
#include "Moc.hpp"
#include "UtInterpolate.hpp"
#include <cmath>


namespace Live2D {
namespace V2 {

WarpDeformer::~WarpDeformer() = default;

void WarpDeformer::Read(BinaryReader& br) {
    Deformer::Read(br);
    mCol = br.ReadInt32();
    mRow = br.ReadInt32();
    mPivotMgr.reset(br.ReadObject<PivotManager*>());
    mPivotPoints = br.ReadObject<std::vector<std::vector<float>>>();
    Deformer::ReadOpacity(br);
}

void WarpDeformer::InitRuntimeState() {
    int pointCount = GetPointCount();
    mInterpolatedPoints.resize(pointCount * 2);
    if (NeedTransform()) {
        mTransformedPoints.resize(pointCount * 2);
    }
}

void WarpDeformer::SetupInterpolate(Moc* moc) {
    int pointCount = GetPointCount();
    bool outOfRange = false;
    UtInterpolate::InterpolatePoints(
        moc, mPivotMgr.get(), outOfRange, pointCount, mPivotPoints, mInterpolatedPoints, 0, 2);
    SetParamOutOfRange(outOfRange);
    InterpolateOpacity(moc, mPivotMgr.get(), outOfRange);
}

bool WarpDeformer::SetupTransform(Moc* moc) {
    SetAvailable(true);

    if (!NeedTransform()) {
        SetTotalOpacity(GetInterpolatedOpacity());
    } else {
        auto* targetId = GetTargetId();
        if (mTmpDeformerIndex == DEFORMER_INDEX_NOT_INIT)
            mTmpDeformerIndex = moc->GetDeformerIndex(targetId);

        if (mTmpDeformerIndex < 0) {
            LOGE("deformer is not reachable\n");
            SetAvailable(false);
        } else {
            auto* parentDef = moc->GetDeformer(mTmpDeformerIndex);
            if (parentDef && parentDef->IsAvailable()) {
                float ps = parentDef->GetTotalScale();
                SetTotalScale_notForClient(ps);
                SetTotalOpacity(parentDef->GetTotalOpacity() * GetInterpolatedOpacity());
                parentDef->TransformPoints(mInterpolatedPoints,
                                           mTransformedPoints,
                                           GetPointCount(),
                                           0,
                                           2);
                SetAvailable(true);
            } else {
                SetAvailable(false);
            }
        }
    }
    return IsAvailable();
}

void WarpDeformer::TransformPoints(const std::vector<float>& srcPoints,
                                   std::vector<float>& dstPoints, int numPoint, int ptOffset,
                                   int ptStep) {
    auto& pivot = !mTransformedPoints.empty() ? mTransformedPoints : mInterpolatedPoints;
    TransformPointsInner(srcPoints, dstPoints, numPoint, ptOffset, ptStep, pivot, mRow, mCol);
}

namespace {

// grid 访问：控制点 (xi, yi)（xi∈[0,row]、yi∈[0,col]），xy 交错存储，x 方向步长 stride
inline float GridX(const std::vector<float>& grid, int stride, int xi, int yi) {
    return grid[(xi + yi * stride) * 2];
}

inline float GridY(const std::vector<float>& grid, int stride, int xi, int yi) {
    return grid[(xi + yi * stride) * 2 + 1];
}

// 单位四边形上的双三角采样：四角 O=(wu,wv)=(0,0)、U=(1,0)、V=(0,1)、UV=(1,1)，
// 沿 O→UV 对角分割 —— wu+wv<=1 在三角形 (O,U,V) 上取值，否则在 (UV,V,U) 上
// 以补权重 (1-wu)、(1-wv) 取值。与网格内插值同款分割方式，保证查询点跨区域
// 移动时结果连续无缝。
inline void SampleSplitQuad(std::vector<float>& dst, int base, float ox, float oy, float ux,
                            float uy, float vx, float vy, float uvx, float uvy, float wu,
                            float wv) {
    if (wu + wv <= 1.0f) {
        dst[base] = ox + (ux - ox) * wu + (vx - ox) * wv;
        dst[base + 1] = oy + (uy - oy) * wu + (vy - oy) * wv;
    } else {
        dst[base] = uvx + (vx - uvx) * (1 - wu) + (ux - uvx) * (1 - wv);
        dst[base + 1] = uvy + (vy - uvy) * (1 - wu) + (uy - uvy) * (1 - wv);
    }
}

}   // namespace

// 网格形变顶点变换（自官方 live2d.js 等价移植，浮点运算顺序未做任何调整）
//
// grid 为 (row+1)x(col+1) 控制点阵；src 中每个点为形变器局部归一化坐标 (u,v)，
// 允许越出 [0,1]。按位置分三类处理：
//  1. 网格内：所在格双三角插值（对角分割，跨格连续无缝）；
//  2. 网格外 [-2,3] 带：用四角差分拟合的全局仿射模型 val(u,v) = fit + u*slopeU + v*slopeV
//     （fit 预先减去 0.5*(slopeU+slopeV)，使四角处的拟合均值恰等于四角真实均值），
//     在 u=-2/3、v=-2/3 处生成虚拟格点，与最近的真实格点/格边组成四边形，
//     复用同款双三角采样 —— 因此外推值在网格边界处与格点值完全衔接；
//  3. 更远处：直接取仿射模型值。
void WarpDeformer::TransformPointsInner(const std::vector<float>& src, std::vector<float>& dst,
                                        int pointCount, int srcOffset, int srcStep,
                                        const std::vector<float>& grid, int row, int col) {
    const int stride = row + 1;   // grid x 方向步长
    const int total = pointCount * srcStep;

    // 全局仿射拟合系数（惰性计算：首个越界点才需要）
    float fitX = 0, fitY = 0;
    float slopeUx = 0, slopeUy = 0, slopeVx = 0, slopeVy = 0;
    bool fitComputed = false;

    for (int base = srcOffset; base < total; base += srcStep) {
        const float u = src[base];
        const float v = src[base + 1];
        const float gx = u * row;   // 格坐标（x 方向共 row 格）
        const float gy = v * col;   // 格坐标（y 方向共 col 格）

        if (gx < 0 || gy < 0 || row <= gx || col <= gy) {
            if (!fitComputed) {
                fitComputed = true;
                // 四角均值
                fitX = 0.25f * (GridX(grid, stride, 0, 0) + GridX(grid, stride, row, 0) +
                                GridX(grid, stride, 0, col) + GridX(grid, stride, row, col));
                fitY = 0.25f * (GridY(grid, stride, 0, 0) + GridY(grid, stride, row, 0) +
                                GridY(grid, stride, 0, col) + GridY(grid, stride, row, col));
                // 主/反对角线的四角差分
                const float diagX = GridX(grid, stride, row, col) - GridX(grid, stride, 0, 0);
                const float diagY = GridY(grid, stride, row, col) - GridY(grid, stride, 0, 0);
                const float antiX = GridX(grid, stride, row, 0) - GridX(grid, stride, 0, col);
                const float antiY = GridY(grid, stride, row, 0) - GridY(grid, stride, 0, col);
                slopeUx = (diagX + antiX) * 0.5f;   // 沿 u 的斜率
                slopeUy = (diagY + antiY) * 0.5f;
                slopeVx = (diagX - antiX) * 0.5f;   // 沿 v 的斜率
                slopeVy = (diagY - antiY) * 0.5f;
                fitX -= 0.5f * (slopeUx + slopeVx);   // 保四角均值
                fitY -= 0.5f * (slopeUy + slopeVy);
            }

            if ((-2 < u && u < 3) && (-2 < v && v < 3)) {
                // 近网格外推带：虚拟格点 + 真实格点/格边组成的四边形上采样
                if (u <= 0) {
                    if (v <= 0) {
                        // 左下角区：仿射@(-2,-2) / 仿射@(0,-2) / 仿射@(-2,0) / 真实角点(0,0)
                        const float ox = fitX - 2 * slopeUx - 2 * slopeVx;
                        const float oy = fitY - 2 * slopeUy - 2 * slopeVy;
                        const float ux = fitX - 2 * slopeVx;
                        const float uy = fitY - 2 * slopeVy;
                        const float vx = fitX - 2 * slopeUx;
                        const float vy = fitY - 2 * slopeUy;
                        const float uvx = GridX(grid, stride, 0, 0);
                        const float uvy = GridY(grid, stride, 0, 0);
                        const float wu = 0.5f * (u - (-2));
                        const float wv = 0.5f * (v - (-2));
                        SampleSplitQuad(dst, base, ox, oy, ux, uy, vx, vy, uvx, uvy, wu, wv);
                    } else if (v >= 1) {
                        // 左上角区：仿射@(-2,1) / 真实角点(0,1) / 仿射@(-2,3) / 仿射@(0,3)
                        const float ox = fitX - 2 * slopeUx + 1 * slopeVx;
                        const float oy = fitY - 2 * slopeUy + 1 * slopeVy;
                        const float ux = GridX(grid, stride, 0, col);
                        const float uy = GridY(grid, stride, 0, col);
                        const float vx = fitX - 2 * slopeUx + 3 * slopeVx;
                        const float vy = fitY - 2 * slopeUy + 3 * slopeVy;
                        const float uvx = fitX + 3 * slopeVx;
                        const float uvy = fitY + 3 * slopeVy;
                        const float wu = 0.5f * (u - (-2));
                        const float wv = 0.5f * (v - 1);
                        SampleSplitQuad(dst, base, ox, oy, ux, uy, vx, vy, uvx, uvy, wu, wv);
                    } else {
                        // 左侧边区：仿射@(-2,v0) / 真实格点(0,cy) / 仿射@(-2,v1) / 真实格点(0,cy+1)
                        int cy = (int)gy;
                        if (cy == col)
                            cy = col - 1;
                        const float v0 = cy / (float)col;
                        const float v1 = (cy + 1) / (float)col;
                        const float ox = fitX - 2 * slopeUx + v0 * slopeVx;
                        const float oy = fitY - 2 * slopeUy + v0 * slopeVy;
                        const float ux = GridX(grid, stride, 0, cy);
                        const float uy = GridY(grid, stride, 0, cy);
                        const float vx = fitX - 2 * slopeUx + v1 * slopeVx;
                        const float vy = fitY - 2 * slopeUy + v1 * slopeVy;
                        const float uvx = GridX(grid, stride, 0, cy + 1);
                        const float uvy = GridY(grid, stride, 0, cy + 1);
                        const float wu = 0.5f * (u - (-2));
                        const float wv = gy - cy;
                        SampleSplitQuad(dst, base, ox, oy, ux, uy, vx, vy, uvx, uvy, wu, wv);
                    }
                } else if (u >= 1) {
                    if (v <= 0) {
                        // 右下角区：仿射@(1,-2) / 仿射@(3,-2) / 真实角点(1,0) / 仿射@(3,0)
                        const float ox = fitX + 1 * slopeUx - 2 * slopeVx;
                        const float oy = fitY + 1 * slopeUy - 2 * slopeVy;
                        const float ux = fitX + 3 * slopeUx - 2 * slopeVx;
                        const float uy = fitY + 3 * slopeUy - 2 * slopeVy;
                        const float vx = GridX(grid, stride, row, 0);
                        const float vy = GridY(grid, stride, row, 0);
                        const float uvx = fitX + 3 * slopeUx;
                        const float uvy = fitY + 3 * slopeUy;
                        const float wu = 0.5f * (u - 1);
                        const float wv = 0.5f * (v - (-2));
                        SampleSplitQuad(dst, base, ox, oy, ux, uy, vx, vy, uvx, uvy, wu, wv);
                    } else if (v >= 1) {
                        // 右上角区：真实角点(1,1) / 仿射@(3,1) / 仿射@(1,3) / 仿射@(3,3)
                        const float ox = GridX(grid, stride, row, col);
                        const float oy = GridY(grid, stride, row, col);
                        const float ux = fitX + 3 * slopeUx + 1 * slopeVx;
                        const float uy = fitY + 3 * slopeUy + 1 * slopeVy;
                        const float vx = fitX + 1 * slopeUx + 3 * slopeVx;
                        const float vy = fitY + 1 * slopeUy + 3 * slopeVy;
                        const float uvx = fitX + 3 * slopeUx + 3 * slopeVx;
                        const float uvy = fitY + 3 * slopeUy + 3 * slopeVy;
                        const float wu = 0.5f * (u - 1);
                        const float wv = 0.5f * (v - 1);
                        SampleSplitQuad(dst, base, ox, oy, ux, uy, vx, vy, uvx, uvy, wu, wv);
                    } else {
                        // 右侧边区：真实格点(1,cy) / 仿射@(3,v0) / 真实格点(1,cy+1) / 仿射@(3,v1)
                        int cy = (int)gy;
                        if (cy == col)
                            cy = col - 1;
                        const float v0 = cy / (float)col;
                        const float v1 = (cy + 1) / (float)col;
                        const float ox = GridX(grid, stride, row, cy);
                        const float oy = GridY(grid, stride, row, cy);
                        const float ux = fitX + 3 * slopeUx + v0 * slopeVx;
                        const float uy = fitY + 3 * slopeUy + v0 * slopeVy;
                        const float vx = GridX(grid, stride, row, cy + 1);
                        const float vy = GridY(grid, stride, row, cy + 1);
                        const float uvx = fitX + 3 * slopeUx + v1 * slopeVx;
                        const float uvy = fitY + 3 * slopeUy + v1 * slopeVy;
                        const float wu = 0.5f * (u - 1);
                        const float wv = gy - cy;
                        SampleSplitQuad(dst, base, ox, oy, ux, uy, vx, vy, uvx, uvy, wu, wv);
                    }
                } else {
                    if (v <= 0) {
                        // 下侧边区：仿射@(u0,-2) / 仿射@(u1,-2) / 真实格点(cx,0) / 真实格点(cx+1,0)
                        int cx = (int)gx;
                        if (cx == row)
                            cx = row - 1;
                        const float u0 = cx / (float)row;
                        const float u1 = (cx + 1) / (float)row;
                        const float ox = fitX + u0 * slopeUx - 2 * slopeVx;
                        const float oy = fitY + u0 * slopeUy - 2 * slopeVy;
                        const float ux = fitX + u1 * slopeUx - 2 * slopeVx;
                        const float uy = fitY + u1 * slopeUy - 2 * slopeVy;
                        const float vx = GridX(grid, stride, cx, 0);
                        const float vy = GridY(grid, stride, cx, 0);
                        const float uvx = GridX(grid, stride, cx + 1, 0);
                        const float uvy = GridY(grid, stride, cx + 1, 0);
                        const float wu = gx - cx;
                        const float wv = 0.5f * (v - (-2));
                        SampleSplitQuad(dst, base, ox, oy, ux, uy, vx, vy, uvx, uvy, wu, wv);
                    } else if (v >= 1) {
                        // 上侧边区：真实格点(cx,1) / 真实格点(cx+1,1) / 仿射@(u0,3) / 仿射@(u1,3)
                        int cx = (int)gx;
                        if (cx == row)
                            cx = row - 1;
                        const float u0 = cx / (float)row;
                        const float u1 = (cx + 1) / (float)row;
                        const float ox = GridX(grid, stride, cx, col);
                        const float oy = GridY(grid, stride, cx, col);
                        const float ux = GridX(grid, stride, cx + 1, col);
                        const float uy = GridY(grid, stride, cx + 1, col);
                        const float vx = fitX + u0 * slopeUx + 3 * slopeVx;
                        const float vy = fitY + u0 * slopeUy + 3 * slopeVy;
                        const float uvx = fitX + u1 * slopeUx + 3 * slopeVx;
                        const float uvy = fitY + u1 * slopeUy + 3 * slopeVy;
                        const float wu = gx - cx;
                        const float wv = 0.5f * (v - 1);
                        SampleSplitQuad(dst, base, ox, oy, ux, uy, vx, vy, uvx, uvy, wu, wv);
                    } else {
                        // 网格内部却走到此处：仅退化网格（row=0 或 col=0）时可达
                        dst[base] = fitX + u * slopeUx + v * slopeVx;
                        dst[base + 1] = fitY + u * slopeUy + v * slopeVy;
                    }
                }
            } else {
                // 远离网格：直接取仿射模型值
                dst[base] = fitX + u * slopeUx + v * slopeVx;
                dst[base + 1] = fitY + u * slopeUy + v * slopeVy;
            }
        } else {
            // 网格内：所在格双三角插值（沿格内 (0,0)→(1,1) 对角分割）
            const int cx = static_cast<int>(gx);
            const int cy = static_cast<int>(gy);
            const float fx = gx - std::floor(gx);
            const float fy = gy - std::floor(gy);
            if (fx + fy < 1.0f) {
                dst[base] = GridX(grid, stride, cx, cy) * (1 - fx - fy) +
                            GridX(grid, stride, cx + 1, cy) * fx +
                            GridX(grid, stride, cx, cy + 1) * fy;
                dst[base + 1] = GridY(grid, stride, cx, cy) * (1 - fx - fy) +
                                GridY(grid, stride, cx + 1, cy) * fx +
                                GridY(grid, stride, cx, cy + 1) * fy;
            } else {
                dst[base] = GridX(grid, stride, cx + 1, cy + 1) * (fx - 1 + fy) +
                            GridX(grid, stride, cx, cy + 1) * (1 - fx) +
                            GridX(grid, stride, cx + 1, cy) * (1 - fy);
                dst[base + 1] = GridY(grid, stride, cx + 1, cy + 1) * (fx - 1 + fy) +
                                GridY(grid, stride, cx, cy + 1) * (1 - fx) +
                                GridY(grid, stride, cx + 1, cy) * (1 - fy);
            }
        }
    }
}

}   // namespace V2
}   // namespace Live2D
