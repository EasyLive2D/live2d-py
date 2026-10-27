#include "ClippingManagerVulkan.hpp"
#include "Moc.hpp"
#include "ClipContext.hpp"
#include "VkRenderer.hpp"

namespace Live2D {
namespace V2 {

static constexpr int CHANNEL_COUNT = 4;

ClippingManagerVulkan::ClippingManagerVulkan(VkRenderer& renderer) : mRenderer(renderer) {
    // 与 GL 版一致: ch0=Alpha, ch1=Red, ch2=Green, ch3=Blue
    mChannelColors.resize(CHANNEL_COUNT);
    mChannelColors[0] = {0.0f, 0.0f, 0.0f, 1.0f};
    mChannelColors[1] = {1.0f, 0.0f, 0.0f, 0.0f};
    mChannelColors[2] = {0.0f, 1.0f, 0.0f, 0.0f};
    mChannelColors[3] = {0.0f, 0.0f, 1.0f, 0.0f};
}

ClippingManagerVulkan::~ClippingManagerVulkan() = default;

// mask 索引集合相等即同组（与旧 clipID 字符串分组等价：索引由 clipID 一对一解析而来）
static ClipContext* findSameClip(const std::vector<std::unique_ptr<ClipContext>>& list,
                                 const int* masks, int maskCount) {
    for (auto& ctx : list) {
        if (ctx->mClippingMaskDrawIndexList.size() != (size_t)maskCount)
            continue;
        int match = 0;
        for (int a : ctx->mClippingMaskDrawIndexList)
            for (int k = 0; k < maskCount; k++)
                if (a == masks[k]) {
                    match++;
                    break;
                }
        if (match == maskCount)
            return ctx.get();
    }
    return nullptr;
}

void ClippingManagerVulkan::Init(Moc* moc) {
    int nDraw = moc->GetDrawableCount();
    mDrawableClipMap.assign(nDraw, nullptr);

    for (int i = 0; i < nDraw; i++) {
        int maskCount = 0;
        const int* masks = moc->GetDrawableMasks(i, maskCount);
        if (maskCount == 0)
            continue;

        auto* clipCtx = findSameClip(mClipContextList, masks, maskCount);
        if (!clipCtx) {
            auto newCtx = std::make_unique<ClipContext>();
            clipCtx = newCtx.get();
            mClipContextList.push_back(std::move(newCtx));
        }

        // mask 索引并入组内列表（去重）
        for (int k = 0; k < maskCount; k++) {
            bool found = false;
            for (int m : clipCtx->mClippingMaskDrawIndexList)
                if (m == masks[k]) {
                    found = true;
                    break;
                }
            if (!found)
                clipCtx->mClippingMaskDrawIndexList.push_back(masks[k]);
        }
        clipCtx->mClippedDrawIndexList.push_back(i);
        mDrawableClipMap[i] = clipCtx;
    }
}

void ClippingManagerVulkan::CalcClippedDrawTotalBounds(Moc* moc, ClipContext* clip) {
    float minX = 0, minY = 0, maxX = 0, maxY = 0;

    int nVertOk = 0;
    for (int idx : clip->mClippedDrawIndexList) {
        if (!moc->IsDrawableAvailable(idx))
            continue;
        int vcount = moc->GetDrawableVertexCount(idx);
        if (vcount < 1)
            continue;
        nVertOk++;

        const float* pts = moc->GetDrawableVertices(idx);
        for (int v = 0; v < vcount; v++) {
            float x = pts[v * 2], y = pts[v * 2 + 1];
            if (x < minX)
                minX = x;
            if (y < minY)
                minY = y;
            if (x > maxX)
                maxX = x;
            if (y > maxY)
                maxY = y;
        }
    }

    if (nVertOk == 0) {
        clip->mIsUsing = false;
    } else {
        clip->mAllClippedDrawRect[0] = minX;
        clip->mAllClippedDrawRect[1] = minY;
        clip->mAllClippedDrawRect[2] = maxX - minX;
        clip->mAllClippedDrawRect[3] = maxY - minY;
        clip->mIsUsing = true;
    }
}

void ClippingManagerVulkan::SetupLayoutBounds(int count) {
    int rows = count / CHANNEL_COUNT;
    int remainder = count % CHANNEL_COUNT;
    int idx = 0;

    for (int ch = 0; ch < CHANNEL_COUNT; ch++) {
        int n = rows + (ch < remainder ? 1 : 0);
        if (n == 1) {
            auto& clip = mClipContextList[idx++];
            clip->mLayoutChannelNo = ch;
            clip->mLayoutBounds = {0, 0, 1, 1};
        } else if (n == 2) {
            for (int i = 0; i < n; i++) {
                auto& clip = mClipContextList[idx++];
                clip->mLayoutChannelNo = ch;
                clip->mLayoutBounds = {(float)(i % 2) * 0.5f, 0, 0.5f, 1};
            }
        } else if (n <= 4) {
            for (int i = 0; i < n; i++) {
                auto& clip = mClipContextList[idx++];
                clip->mLayoutChannelNo = ch;
                clip->mLayoutBounds = {(float)(i % 2) * 0.5f, (float)(i / 2) * 0.5f, 0.5f, 0.5f};
            }
        } else if (n <= 9) {
            for (int i = 0; i < n; i++) {
                auto& clip = mClipContextList[idx++];
                clip->mLayoutChannelNo = ch;
                clip->mLayoutBounds = {
                    (float)(i % 3) / 3.0f, (float)(i / 3) / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f};
            }
        }
    }
}

// 裁剪矩阵推导与 ClippingManagerOpenGL::buildClipMatrix 逐行一致
static void buildClipMatrix(std::array<float, 16>& out, ClipContext* clip, bool forMask) {
    out.fill(0);
    float bx = clip->mLayoutBounds[0], by = clip->mLayoutBounds[1];
    float bw = clip->mLayoutBounds[2], bh = clip->mLayoutBounds[3];
    float cx = clip->mAllClippedDrawRect[0], cy = clip->mAllClippedDrawRect[1];
    float cw = clip->mAllClippedDrawRect[2], ch = clip->mAllClippedDrawRect[3];

    if (cw <= 0 || ch <= 0) {
        out[0] = out[5] = out[10] = out[15] = 1;
        return;
    }

    float pad = 0.05f;
    float ex = cw * pad, ey = ch * pad;
    float ew = cw + ex * 2, eh = ch + ey * 2;
    float sx = bw / ew;
    float sy = bh / eh;

    if (forMask) {
        out[0] = 2.0f * sx;
        out[5] = 2.0f * sy;
        out[10] = 1.0f;
        out[12] = -1.0f + 2.0f * bx - 2.0f * sx * (cx - ex);
        out[13] = -1.0f + 2.0f * by - 2.0f * sy * (cy - ey);
        out[15] = 1.0f;
    } else {
        out[0] = sx;
        out[5] = sy;
        out[10] = 1.0f;
        out[12] = bx - sx * (cx - ex);
        out[13] = by - sy * (cy - ey);
        out[15] = 1.0f;
    }
}

bool ClippingManagerVulkan::SetupClip(Moc* moc) {
    int activeCount = 0;
    for (auto& clip : mClipContextList) {
        CalcClippedDrawTotalBounds(moc, clip.get());
        if (clip->mIsUsing)
            activeCount++;
    }
    if (activeCount == 0)
        return false;

    SetupLayoutBounds(activeCount);

    for (auto& clip : mClipContextList) {
        if (!clip->mIsUsing)
            continue;

        buildClipMatrix(clip->mMatrixForMask, clip.get(), true);
        buildClipMatrix(clip->mMatrixForDraw, clip.get(), false);

        // 在 mask pass 中渲染遮罩源 drawable（normal 管线 + maskFlag=1，matrixForMask 作 mvp）
        for (int idx : clip->mClippingMaskDrawIndexList) {
            // 与旧路径 Mesh::draw 一致：参数越界的 mask 源跳过
            if (moc->IsDrawableParamOutside(idx))
                continue;
            mRenderer.SetClipMatrix(clip->mMatrixForMask.data());
            mRenderer.SetClipChannel(clip->mLayoutChannelNo);
            mRenderer.SetClipMaskMode(true);
            mRenderer.DrawDrawable(moc, idx);
            mRenderer.SetClipMaskMode(false);
        }
    }
    return true;
}

}   // namespace V2
}   // namespace Live2D
