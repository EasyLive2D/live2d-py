#include "WarpDeformer.hpp"
#include "WarpContext.hpp"
#include "DeformerContext.hpp"
#include "../Core/BinaryReader.hpp"
#include "../Core/PivotManager.hpp"
#include "../Model/ModelContext.hpp"
#include "../Util/UtInterpolate.hpp"
#include <cstdio>
#include <stdexcept>

namespace live2d {

void WarpDeformer::read(BinaryReader& br) {
    Deformer::read(br);
    mCol = br.readInt32();
    mRow = br.readInt32();
    mPivotMgr = br.readObject<PivotManager*>();
    mPivotPoints = br.readObject<std::vector<std::vector<float>>>();
    Deformer::readOpacity(br);
}

DeformerContext* WarpDeformer::init(ModelContext* mc) {
    (void)mc;
    return new WarpContext(this);
}

void WarpDeformer::setupInterpolate(ModelContext* mc, DeformerContext* dc) {
    auto* wctx = static_cast<WarpContext*>(dc);
    if (!mPivotMgr->checkParamUpdated(mc)) return;

    int pointCount = getPointCount();
    bool success = false;
    UtInterpolate::interpolatePoints(mc, mPivotMgr, success, pointCount,
                                     mPivotPoints, wctx->mInterpolatedPoints, 0, 2);
    wctx->setOutsideParam(success);
    interpolateOpacity(mc, mPivotMgr, wctx, success);
}

bool WarpDeformer::setupTransform(ModelContext* mc, DeformerContext* dc) {
    auto* wctx = static_cast<WarpContext*>(dc);
    wctx->setAvailable(true);

    if (!needTransform()) {
        wctx->setTotalOpacity(wctx->getInterpolatedOpacity());
    } else {
        auto* targetId = getTargetId();
        if (wctx->mTmpDeformerIndex == DEFORMER_INDEX_NOT_INIT)
            wctx->mTmpDeformerIndex = mc->getDeformerIndex(targetId);

        if (wctx->mTmpDeformerIndex < 0) {
            printf("deformer is not reachable\n");
            wctx->setAvailable(false);
        } else {
            auto* parentDef = mc->getDeformer(wctx->mTmpDeformerIndex);
            auto* parentCtx = mc->getDeformerContext(wctx->mTmpDeformerIndex);
            if (parentDef && parentCtx->isAvailable()) {
                float ps = parentCtx->getTotalScale();
                wctx->setTotalScale_notForClient(ps);
                wctx->setTotalOpacity(parentCtx->getTotalOpacity() * wctx->getInterpolatedOpacity());
                parentDef->transformPoints(mc, parentCtx,
                    wctx->mInterpolatedPoints, wctx->mTransformedPoints,
                    getPointCount(), 0, 2);
                wctx->setAvailable(true);
            } else {
                wctx->setAvailable(false);
            }
        }
    }
    return wctx->isAvailable();
}

void WarpDeformer::transformPoints(ModelContext* mc, DeformerContext* dc,
                                   const std::vector<float>& srcPoints,
                                   std::vector<float>& dstPoints,
                                   int numPoint, int ptOffset, int ptStep) {
    auto* wctx = static_cast<WarpContext*>(dc);
    auto& pivot = !wctx->mTransformedPoints.empty()
                      ? wctx->mTransformedPoints
                      : wctx->mInterpolatedPoints;
    transformPoints_sdk2(srcPoints, dstPoints, numPoint, ptOffset, ptStep,
                          pivot, mRow, mCol);
}

// Static grid-based vertex transform (ported 1:1 from Python WarpDeformer)
void WarpDeformer::transformPoints_sdk2(
    const std::vector<float>& src, std::vector<float>& dst,
    int pointCount, int srcOffset, int srcStep,
    const std::vector<float>& grid, int row, int col) {

    int total = pointCount * srcStep;
    float aT = 0, aS = 0;
    float bl = 0, bk_ = 0, bf = 0, be = 0;
    bool computedExtrap = false;

    for (int ba = srcOffset; ba < total; ba += srcStep) {
        float a4 = src[ba];
        float aX = src[ba + 1];
        float bd = a4 * row;
        float a7 = aX * col;

        if (bd < 0 || a7 < 0 || row <= bd || col <= a7) {
            int a1 = row + 1;
            if (!computedExtrap) {
                computedExtrap = true;
                aT = 0.25f * (grid[((0) + (0) * a1) * 2] +
                              grid[((row) + (0) * a1) * 2] +
                              grid[((0) + (col) * a1) * 2] +
                              grid[((row) + (col) * a1) * 2]);
                aS = 0.25f * (grid[((0) + (0) * a1) * 2 + 1] +
                              grid[((row) + (0) * a1) * 2 + 1] +
                              grid[((0) + (col) * a1) * 2 + 1] +
                              grid[((row) + (col) * a1) * 2 + 1]);
                float aM = grid[((row) + (col) * a1) * 2] -
                           grid[((0) + (0) * a1) * 2];
                float aL = grid[((row) + (col) * a1) * 2 + 1] -
                           grid[((0) + (0) * a1) * 2 + 1];
                float bh = grid[((row) + (0) * a1) * 2] -
                           grid[((0) + (col) * a1) * 2];
                float bg = grid[((row) + (0) * a1) * 2 + 1] -
                           grid[((0) + (col) * a1) * 2 + 1];
                bl = (aM + bh) * 0.5f;
                bk_ = (aL + bg) * 0.5f;
                bf = (aM - bh) * 0.5f;
                be = (aL - bg) * 0.5f;
                aT -= 0.5f * (bl + bf);
                aS -= 0.5f * (bk_ + be);
            }

            if ((-2 < a4 && a4 < 3) && (-2 < aX && aX < 3)) {
                // Full extrapolation logic ported 1:1 from Python WarpDeformer
                if (a4 <= 0) {
                    if (aX <= 0) {
                        float a3 = grid[((0) + (0) * a1) * 2];
                        float a2 = grid[((0) + (0) * a1) * 2 + 1];
                        float a8 = aT - 2 * bl;
                        float a6 = aS - 2 * bk_;
                        float aK = aT - 2 * bf;
                        float aJ = aS - 2 * be;
                        float aO = aT - 2 * bl - 2 * bf;
                        float aN = aS - 2 * bk_ - 2 * be;
                        float bj = 0.5f * (a4 - (-2));
                        float bi = 0.5f * (aX - (-2));
                        if (bj + bi <= 1.0f) {
                            dst[ba] = aO + (aK - aO) * bj + (a8 - aO) * bi;
                            dst[ba + 1] = aN + (aJ - aN) * bj + (a6 - aN) * bi;
                        } else {
                            dst[ba] = a3 + (a8 - a3) * (1 - bj) + (aK - a3) * (1 - bi);
                            dst[ba + 1] = a2 + (a6 - a2) * (1 - bj) + (aJ - a2) * (1 - bi);
                        }
                    } else if (aX >= 1) {
                        float aK = grid[((0) + (col) * a1) * 2];
                        float aJ = grid[((0) + (col) * a1) * 2 + 1];
                        float aO = aT - 2 * bl + 1 * bf;
                        float aN = aS - 2 * bk_ + 1 * be;
                        float a3 = aT + 3 * bf;
                        float a2 = aS + 3 * be;
                        float a8 = aT - 2 * bl + 3 * bf;
                        float a6 = aS - 2 * bk_ + 3 * be;
                        float bj = 0.5f * (a4 - (-2));
                        float bi = 0.5f * (aX - (1));
                        if (bj + bi <= 1.0f) {
                            dst[ba] = aO + (aK - aO) * bj + (a8 - aO) * bi;
                            dst[ba + 1] = aN + (aJ - aN) * bj + (a6 - aN) * bi;
                        } else {
                            dst[ba] = a3 + (a8 - a3) * (1 - bj) + (aK - a3) * (1 - bi);
                            dst[ba + 1] = a2 + (a6 - a2) * (1 - bj) + (aJ - a2) * (1 - bi);
                        }
                    } else {
                        int aH = (int)a7;
                        if (aH == col) aH = col - 1;
                        float bj = 0.5f * (a4 - (-2));
                        float bi = a7 - aH;
                        float bb = aH / (float)col;
                        float a9 = (aH + 1) / (float)col;
                        float aK = grid[((0) + (aH) * a1) * 2];
                        float aJ = grid[((0) + (aH) * a1) * 2 + 1];
                        float a3 = grid[((0) + (aH + 1) * a1) * 2];
                        float a2 = grid[((0) + (aH + 1) * a1) * 2 + 1];
                        float aO = aT - 2 * bl + bb * bf;
                        float aN = aS - 2 * bk_ + bb * be;
                        float a8 = aT - 2 * bl + a9 * bf;
                        float a6 = aS - 2 * bk_ + a9 * be;
                        if (bj + bi <= 1.0f) {
                            dst[ba] = aO + (aK - aO) * bj + (a8 - aO) * bi;
                            dst[ba + 1] = aN + (aJ - aN) * bj + (a6 - aN) * bi;
                        } else {
                            dst[ba] = a3 + (a8 - a3) * (1 - bj) + (aK - a3) * (1 - bi);
                            dst[ba + 1] = a2 + (a6 - a2) * (1 - bj) + (aJ - a2) * (1 - bi);
                        }
                    }
                } else if (a4 >= 1) {
                    if (aX <= 0) {
                        float a8 = grid[((row) + (0) * a1) * 2];
                        float a6 = grid[((row) + (0) * a1) * 2 + 1];
                        float a3 = aT + 3 * bl;
                        float a2 = aS + 3 * bk_;
                        float aO = aT + 1 * bl - 2 * bf;
                        float aN = aS + 1 * bk_ - 2 * be;
                        float aK = aT + 3 * bl - 2 * bf;
                        float aJ = aS + 3 * bk_ - 2 * be;
                        float bj = 0.5f * (a4 - (1));
                        float bi = 0.5f * (aX - (-2));
                        if (bj + bi <= 1.0f) {
                            dst[ba] = aO + (aK - aO) * bj + (a8 - aO) * bi;
                            dst[ba + 1] = aN + (aJ - aN) * bj + (a6 - aN) * bi;
                        } else {
                            dst[ba] = a3 + (a8 - a3) * (1 - bj) + (aK - a3) * (1 - bi);
                            dst[ba + 1] = a2 + (a6 - a2) * (1 - bj) + (aJ - a2) * (1 - bi);
                        }
                    } else if (aX >= 1) {
                        float aO = grid[((row) + (col) * a1) * 2];
                        float aN = grid[((row) + (col) * a1) * 2 + 1];
                        float aK = aT + 3 * bl + 1 * bf;
                        float aJ = aS + 3 * bk_ + 1 * be;
                        float a8 = aT + 1 * bl + 3 * bf;
                        float a6 = aS + 1 * bk_ + 3 * be;
                        float a3 = aT + 3 * bl + 3 * bf;
                        float a2 = aS + 3 * bk_ + 3 * be;
                        float bj = 0.5f * (a4 - (1));
                        float bi = 0.5f * (aX - (1));
                        if (bj + bi <= 1.0f) {
                            dst[ba] = aO + (aK - aO) * bj + (a8 - aO) * bi;
                            dst[ba + 1] = aN + (aJ - aN) * bj + (a6 - aN) * bi;
                        } else {
                            dst[ba] = a3 + (a8 - a3) * (1 - bj) + (aK - a3) * (1 - bi);
                            dst[ba + 1] = a2 + (a6 - a2) * (1 - bj) + (aJ - a2) * (1 - bi);
                        }
                    } else {
                        int aH = (int)a7;
                        if (aH == col) aH = col - 1;
                        float bj = 0.5f * (a4 - (1));
                        float bi = a7 - aH;
                        float bb = aH / (float)col;
                        float a9 = (aH + 1) / (float)col;
                        float aO = grid[((row) + (aH) * a1) * 2];
                        float aN = grid[((row) + (aH) * a1) * 2 + 1];
                        float a8 = grid[((row) + (aH + 1) * a1) * 2];
                        float a6 = grid[((row) + (aH + 1) * a1) * 2 + 1];
                        float aK = aT + 3 * bl + bb * bf;
                        float aJ = aS + 3 * bk_ + bb * be;
                        float a3 = aT + 3 * bl + a9 * bf;
                        float a2 = aS + 3 * bk_ + a9 * be;
                        if (bj + bi <= 1.0f) {
                            dst[ba] = aO + (aK - aO) * bj + (a8 - aO) * bi;
                            dst[ba + 1] = aN + (aJ - aN) * bj + (a6 - aN) * bi;
                        } else {
                            dst[ba] = a3 + (a8 - a3) * (1 - bj) + (aK - a3) * (1 - bi);
                            dst[ba + 1] = a2 + (a6 - a2) * (1 - bj) + (aJ - a2) * (1 - bi);
                        }
                    }
                } else {
                    // 0 < a4 < 1: in between, handle by Y (row) interpolation
                    if (aX <= 0) {
                        int aY = (int)bd;
                        if (aY == row) aY = row - 1;
                        float bj = bd - aY;
                        float bi = 0.5f * (aX - (-2));
                        float bp = aY / (float)row;
                        float bo = (aY + 1) / (float)row;
                        float a8 = grid[((aY) + (0) * a1) * 2];
                        float a6 = grid[((aY) + (0) * a1) * 2 + 1];
                        float a3 = grid[((aY + 1) + (0) * a1) * 2];
                        float a2 = grid[((aY + 1) + (0) * a1) * 2 + 1];
                        float aO = aT + bp * bl - 2 * bf;
                        float aN = aS + bp * bk_ - 2 * be;
                        float aK = aT + bo * bl - 2 * bf;
                        float aJ = aS + bo * bk_ - 2 * be;
                        if (bj + bi <= 1.0f) {
                            dst[ba] = aO + (aK - aO) * bj + (a8 - aO) * bi;
                            dst[ba + 1] = aN + (aJ - aN) * bj + (a6 - aN) * bi;
                        } else {
                            dst[ba] = a3 + (a8 - a3) * (1 - bj) + (aK - a3) * (1 - bi);
                            dst[ba + 1] = a2 + (a6 - a2) * (1 - bj) + (aJ - a2) * (1 - bi);
                        }
                    } else if (aX >= 1) {
                        int aY = (int)bd;
                        if (aY == row) aY = row - 1;
                        float bj = bd - aY;
                        float bi = 0.5f * (aX - (1));
                        float bp = aY / (float)row;
                        float bo = (aY + 1) / (float)row;
                        float aO = grid[((aY) + (col) * a1) * 2];
                        float aN = grid[((aY) + (col) * a1) * 2 + 1];
                        float aK = grid[((aY + 1) + (col) * a1) * 2];
                        float aJ = grid[((aY + 1) + (col) * a1) * 2 + 1];
                        float a8 = aT + bp * bl + 3 * bf;
                        float a6 = aS + bp * bk_ + 3 * be;
                        float a3 = aT + bo * bl + 3 * bf;
                        float a2 = aS + bo * bk_ + 3 * be;
                        if (bj + bi <= 1.0f) {
                            dst[ba] = aO + (aK - aO) * bj + (a8 - aO) * bi;
                            dst[ba + 1] = aN + (aJ - aN) * bj + (a6 - aN) * bi;
                        } else {
                            dst[ba] = a3 + (a8 - a3) * (1 - bj) + (aK - a3) * (1 - bi);
                            dst[ba + 1] = a2 + (a6 - a2) * (1 - bj) + (aJ - a2) * (1 - bi);
                        }
                    } else {
                        // Inner case: should not reach here normally
                        dst[ba] = aT + a4 * bl + aX * bf;
                        dst[ba + 1] = aS + a4 * bk_ + aX * be;
                    }
                }
            } else {
                // Far outside: generic formula
                dst[ba] = aT + a4 * bl + aX * bf;
                dst[ba + 1] = aS + a4 * bk_ + aX * be;
            }
        } else {
            // Within bounds: bilinear interpolation from grid
            float bn = bd - std::floor(bd);
            float bm = a7 - std::floor(a7);
            int aV = 2 * (static_cast<int>(bd) + static_cast<int>(a7) * (row + 1));
            if (bn + bm < 1.0f) {
                dst[ba] = grid[aV] * (1 - bn - bm) + grid[aV + 2] * bn +
                          grid[aV + 2 * (row + 1)] * bm;
                dst[ba + 1] = grid[aV + 1] * (1 - bn - bm) + grid[aV + 3] * bn +
                              grid[aV + 2 * (row + 1) + 1] * bm;
            } else {
                dst[ba] = grid[aV + 2 * (row + 1) + 2] * (bn - 1 + bm) +
                          grid[aV + 2 * (row + 1)] * (1 - bn) + grid[aV + 2] * (1 - bm);
                dst[ba + 1] = grid[aV + 2 * (row + 1) + 3] * (bn - 1 + bm) +
                              grid[aV + 2 * (row + 1) + 1] * (1 - bn) +
                              grid[aV + 3] * (1 - bm);
            }
        }
    }
}

} // namespace live2d
