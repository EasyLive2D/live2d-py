#include "ClippingManagerOpenGL.hpp"
#include "DrawParamOpenGL.hpp"
#include "ClipContext.hpp"
#include "../Model/ModelContext.hpp"
#include "../Draw/Mesh.hpp"
#include "../Draw/MeshContext.hpp"
#include "../Draw/IDrawData.hpp"
#include "../Core/ModelImpl.hpp"
#include "../Model/ALive2DModel.hpp"
#include "../Core/DEF.hpp"
#include "../Core/PartsDataContext.hpp"
#include "../Core/PartsData.hpp"
#include <GL/glew.h>
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace live2d {

static constexpr int CHANNEL_COUNT = 4;
static constexpr int CLIP_MASK_SIZE = 256;

ClippingManagerOpenGL::ClippingManagerOpenGL(DrawParamOpenGL* dp) : mDpGL(dp) {
    // Init channel colors
    mChannelColors.resize(CHANNEL_COUNT);
    // Match v2 Python: ch0=Alpha, ch1=Red, ch2=Green, ch3=Blue
    mChannelColors[0] = {0.0f, 0.0f, 0.0f, 1.0f};
    mChannelColors[1] = {1.0f, 0.0f, 0.0f, 0.0f};
    mChannelColors[2] = {0.0f, 1.0f, 0.0f, 0.0f};
    mChannelColors[3] = {0.0f, 0.0f, 1.0f, 0.0f};

    if (mDpGL) {
        mDpGL->createFramebuffer();
    }
}

ClippingManagerOpenGL::~ClippingManagerOpenGL() {
    for (auto* ctx : mClipContextList) delete ctx;
}

static ClipContext* findSameClip(const std::vector<ClipContext*>& list,
                                  const std::vector<std::string>& ids) {
    for (auto* ctx : list) {
        if (ctx->mClipIDList.size() != ids.size()) continue;
        int match = 0;
        for (auto& a : ctx->mClipIDList)
            for (auto& b : ids)
                if (a == b) { match++; break; }
        if (match == (int)ids.size()) return ctx;
    }
    return nullptr;
}

void ClippingManagerOpenGL::init(ModelContext* mc,
                                  const std::vector<IDrawData*>& drawDataList,
                                  const std::vector<MeshContext*>& drawContextList) {
    int clipCount = 0;
    for (size_t i = 0; i < drawDataList.size(); i++) {
        auto* clipIDs = drawDataList[i]->getClipIDList();
        if (!clipIDs || clipIDs->empty()) continue;
        clipCount++;

        auto* clipCtx = findSameClip(mClipContextList, *clipIDs);
        if (!clipCtx) {
            clipCtx = new ClipContext(mc, *clipIDs);
            mClipContextList.push_back(clipCtx);
        }

        auto* drawId = drawDataList[i]->getId();
        int drawIdx = mc->getDrawDataIndex(drawId);
        // Mask source: only the drawable matching the clip ID (dedup)
        for (auto& cid : *clipIDs) {
            int maskIdx = mc->getDrawDataIndex(&Id::getID(cid));
            if (maskIdx >= 0) {
                bool found = false;
                for (int m : clipCtx->mClippingMaskDrawIndexList)
                    if (m == maskIdx) { found = true; break; }
                if (!found) clipCtx->mClippingMaskDrawIndexList.push_back(maskIdx);
            }
        }
        // All drawables with this clip ID reference
        clipCtx->addClippedDrawData(drawId->str(), drawIdx);
        drawContextList[i]->mClipBufPre_clipContext = clipCtx;
    }
}

void ClippingManagerOpenGL::calcClippedDrawTotalBounds(ModelContext* mc, ClipContext* clip) {
    int canvasW = mc->mModel->getModelImpl()->getCanvasWidth();
    int canvasH = mc->mModel->getModelImpl()->getCanvasHeight();
    float maxDim = (float)std::max(canvasW, canvasH);
    // Match v2 Python: Float32Array(a4) with trailing zeros makes min always 0
    float minX = 0, minY = 0, maxX = 0, maxY = 0;

    int nAvail = 0, nVertOk = 0;
    for (int idx : clip->mClippedDrawIndexList) {
        auto* ctx = mc->getDrawContext(idx);
        if (!ctx->mAvailable) continue;
        nAvail++;

        auto& pts = ctx->mTransformedPoints.empty()
                        ? ctx->mInterpolatedPoints
                        : ctx->mTransformedPoints;
        if (pts.size() < 2) continue;
        nVertOk++;

        float dminX=1e9f,dmaxX=-1e9f,dminY=1e9f,dmaxY=-1e9f;
        for (size_t j = VERTEX_OFFSET; j < pts.size(); j += VERTEX_STEP) {
            float x = pts[j], y = pts[j+1];
            if (x<dminX)dminX=x; if(y<dminY)dminY=y;
            if (x>dmaxX)dmaxX=x; if(y>dmaxY)dmaxY=y;
            if (x < minX) minX = x;
            if (y < minY) minY = y;
            if (x > maxX) maxX = x;
            if (y > maxY) maxY = y;
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

void ClippingManagerOpenGL::setupLayoutBounds(int count) {
    int rows = count / CHANNEL_COUNT;
    int remainder = count % CHANNEL_COUNT;
    int idx = 0;

    for (int ch = 0; ch < CHANNEL_COUNT; ch++) {
        int n = rows + (ch < remainder ? 1 : 0);
        if (n == 1) {
            auto* clip = mClipContextList[idx++];
            clip->mLayoutChannelNo = ch;
            clip->mLayoutBounds[0] = 0; clip->mLayoutBounds[1] = 0;
            clip->mLayoutBounds[2] = 1; clip->mLayoutBounds[3] = 1;
        } else if (n == 2) {
            for (int i = 0; i < n; i++) {
                auto* clip = mClipContextList[idx++];
                clip->mLayoutChannelNo = ch;
                clip->mLayoutBounds[0] = (float)(i % 2) * 0.5f;
                clip->mLayoutBounds[1] = 0;
                clip->mLayoutBounds[2] = 0.5f;
                clip->mLayoutBounds[3] = 1;
            }
        } else if (n <= 4) {
            for (int i = 0; i < n; i++) {
                auto* clip = mClipContextList[idx++];
                clip->mLayoutChannelNo = ch;
                clip->mLayoutBounds[0] = (float)(i % 2) * 0.5f;
                clip->mLayoutBounds[1] = (float)(i / 2) * 0.5f;
                clip->mLayoutBounds[2] = 0.5f;
                clip->mLayoutBounds[3] = 0.5f;
            }
        } else if (n <= 9) {
            for (int i = 0; i < n; i++) {
                auto* clip = mClipContextList[idx++];
                clip->mLayoutChannelNo = ch;
                clip->mLayoutBounds[0] = (float)(i % 3) / 3.0f;
                clip->mLayoutBounds[1] = (float)(i / 3) / 3.0f;
                clip->mLayoutBounds[2] = 1.0f / 3.0f;
                clip->mLayoutBounds[3] = 1.0f / 3.0f;
            }
        }
    }
}

// Build clip matrix from ClipContext layout + bounds
// forMask=true:  includes T(-1,-1)*S(2,2) NDC mapping (used as u_mvpMatrix in MASK path)
// forMask=false: maps directly to [0,1] texture coords (used as u_clipMatrix in CLIP path)
static void buildClipMatrix(std::array<float, 16>& out, ClipContext* clip, bool forMask) {
    out.fill(0);
    float bx = clip->mLayoutBounds[0], by = clip->mLayoutBounds[1];
    float bw = clip->mLayoutBounds[2], bh = clip->mLayoutBounds[3];
    float cx = clip->mAllClippedDrawRect[0], cy = clip->mAllClippedDrawRect[1];
    float cw = clip->mAllClippedDrawRect[2], ch = clip->mAllClippedDrawRect[3];

    if (cw <= 0 || ch <= 0) { out[0] = out[5] = out[10] = out[15] = 1; return; }

    float pad = 0.05f;
    float ex = cw * pad, ey = ch * pad;
    float ew = cw + ex * 2, eh = ch + ey * 2;
    float sx = bw / ew;
    float sy = bh / eh;

    if (forMask) {
        // v2 Python matrixForMask: T(-1,-1)*S(2,2)*T(lx,ly)*S(lw/ew,lh/eh)*T(-cx+ex,-cy+ey)
        out[0] = 2.0f * sx;
        out[5] = 2.0f * sy;
        out[10] = 1.0f;
        out[12] = -1.0f + 2.0f * bx - 2.0f * sx * (cx - ex);
        out[13] = -1.0f + 2.0f * by - 2.0f * sy * (cy - ey);
        out[15] = 1.0f;
    } else {
        // v2 Python matrixForDraw: T(lx,ly)*S(lw/ew,lh/eh)*T(-cx+ex,-cy+ey)
        out[0] = sx;
        out[5] = sy;
        out[10] = 1.0f;
        out[12] = bx - sx * (cx - ex);
        out[13] = by - sy * (cy - ey);
        out[15] = 1.0f;
    }
}

void ClippingManagerOpenGL::setupClip(ModelContext* mc, DrawParamOpenGL* dp) {
    int activeCount = 0;
    for (auto* clip : mClipContextList) {
        calcClippedDrawTotalBounds(mc, clip);
        if (clip->mIsUsing) activeCount++;
    }
    if (activeCount == 0) return;

    // Save current viewport
    GLint savedViewport[4];
    glGetIntegerv(GL_VIEWPORT, savedViewport);

    mDpGL->bindFramebuffer(mDpGL->mFramebuffer);
    glViewport(0, 0, CLIP_MASK_SIZE, CLIP_MASK_SIZE);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    setupLayoutBounds(activeCount);

    for (auto* clip : mClipContextList) {
        if (!clip->mIsUsing) continue;

        buildClipMatrix(clip->mMatrixForMask, clip, true);
        buildClipMatrix(clip->mMatrixForDraw, clip, false);

        // Store clip matrix for DRAW on all clipped contexts
        for (int idx : clip->mClippedDrawIndexList) {
            auto* ctx = mc->getDrawContext(idx);
            std::memcpy(ctx->mClipMatrixForDraw, clip->mMatrixForDraw.data(), 16 * sizeof(float));
        }

        // Render mask SOURCE drawables to FBO via mask render path
        for (int idx : clip->mClippingMaskDrawIndexList) {
            auto* dd = static_cast<Mesh*>(mc->getDrawData(idx));
            auto* ctx = mc->getDrawContext(idx);
            dp->setClipMatrix(clip->mMatrixForMask.data());
            dp->mClipChannel = clip->mLayoutChannelNo;
            dp->setClipBufPre_clipContextForMask((void*)1);
            dd->draw(dp, mc, ctx);
            dp->setClipBufPre_clipContextForMask(nullptr);
        }
    }

    mDpGL->bindFramebuffer(0);
    glViewport(savedViewport[0], savedViewport[1], savedViewport[2], savedViewport[3]);
}

} // namespace live2d
