#include "Mesh.hpp"
#include "MeshContext.hpp"
#include "../Core/BinaryReader.hpp"
#include "../Core/DEF.hpp"
#include "../Core/PivotManager.hpp"
#include "../Model/ModelContext.hpp"
#include "../Core/PartsData.hpp"
#include "../Deformer/Deformer.hpp"
#include "../Deformer/DeformerContext.hpp"
#include "../Graphics/ClipContext.hpp"
#include "../Graphics/DrawParamOpenGL.hpp"
#include "../Core/PartsDataContext.hpp"
#include "../Util/UtInterpolate.hpp"
#include <cstdio>
#include <stdexcept>

namespace live2d {

static int sInstanceCount = 0;

Mesh::Mesh() : IDrawData() {
    mInstanceNo = sInstanceCount++;
}

void Mesh::read(BinaryReader& br) {
    IDrawData::read(br);
    mTextureNo = br.readInt32();
    mPointCount = br.readInt32();
    mPolygonCount = br.readInt32();

    auto obj = br.readObject<std::vector<int32_t>>();
    mIndexArray.resize(mPolygonCount * 3);
    for (int i = mPolygonCount * 3 - 1; i >= 0; i--) {
        mIndexArray[i] = static_cast<int16_t>(obj[i]);
    }

    mPivotPoints = br.readObject<std::vector<std::vector<float>>>();
    mUvs = br.readObject<std::vector<float>>();

    if (br.getFormatVersion() >= LIVE2D_FORMAT_VERSION_V2_8_TEX_OPTION) {
        mOptionFlag = br.readInt32();
        int comp = (mOptionFlag & MASK_COLOR_COMPOSITION) >> 1;
        if (mOptionFlag != 0) {
            if ((mOptionFlag & 1) != 0) {
                (void)br.readInt32();
                throw std::runtime_error("not handled");
            }
            if ((mOptionFlag & MASK_COLOR_COMPOSITION) != 0) {
                mColorCompositionType = (mOptionFlag & MASK_COLOR_COMPOSITION) >> 1;
            } else {
                mColorCompositionType = COLOR_COMPOSITION_NORMAL;
            }
            if ((mOptionFlag & 32) != 0) {
                mCulling = false;
            }
        }
    } else {
        mOptionFlag = 0;
    }
}

MeshContext* Mesh::init(ModelContext* modelContext) {
    auto* ctx = new MeshContext(this);
    int vertexCount = mPointCount * VERTEX_STEP;
    bool needXform = needTransform();
    ctx->mInterpolatedPoints.resize(vertexCount);
    if (needXform) ctx->mTransformedPoints.resize(vertexCount);

    if (VERTEX_TYPE == VERTEX_TYPE_OFFSET0_STEP2) {
        if (REVERSE_TEXTURE_T) {
            for (int i = mPointCount - 1; i >= 0; i--) {
                int uvOffset = i << 1;
                mUvs[uvOffset + 1] = 1.0f - mUvs[uvOffset + 1];
            }
        }
    } else if (VERTEX_TYPE == VERTEX_TYPE_OFFSET2_STEP5) {
        for (int i = mPointCount - 1; i >= 0; i--) {
            int uvOffset = i << 1;
            int vOffset = i * VERTEX_STEP;
            float uvX = mUvs[uvOffset];
            float uvY = mUvs[uvOffset + 1];
            ctx->mInterpolatedPoints[vOffset] = uvX;
            ctx->mInterpolatedPoints[vOffset + 1] = uvY;
            ctx->mInterpolatedPoints[vOffset + 4] = 0;
            if (needXform) {
                ctx->mTransformedPoints[vOffset] = uvX;
                ctx->mTransformedPoints[vOffset + 1] = uvY;
                ctx->mTransformedPoints[vOffset + 4] = 0;
            }
        }
    }
    return ctx;
}

void Mesh::setupInterpolate(ModelContext* modelContext, MeshContext* meshContext) {
    if (!mPivotMgr->checkParamUpdated(modelContext)) return;
    IDrawData::setupInterpolate(modelContext, meshContext);
    if (meshContext->mParamOutside) return;

    bool paramOutside = false;
    UtInterpolate::interpolatePoints(modelContext, mPivotMgr, paramOutside,
                                     mPointCount, mPivotPoints,
                                     meshContext->mInterpolatedPoints,
                                     VERTEX_OFFSET, VERTEX_STEP);
    if (paramOutside) meshContext->mParamOutside = true;
}

void Mesh::setupTransform(ModelContext* mc, IDrawContext* dc) {
    auto* ctx = static_cast<MeshContext*>(dc);
    if (ctx->mParamOutside) return;

    // setupInterpolate already called for drawable level; skip base class call
    if (needTransform()) {
        auto* targetId = getTargetId();
        if (ctx->mTmpDeformerIndex == DEFORMER_INDEX_NOT_INIT)
            ctx->mTmpDeformerIndex = mc->getDeformerIndex(targetId);

        if (ctx->mTmpDeformerIndex < 0) {
            printf("deformer not found: %s\n", targetId->str().c_str());
        } else {
            auto* deformer = mc->getDeformer(ctx->mTmpDeformerIndex);
            auto* deformerCtx = mc->getDeformerContext(ctx->mTmpDeformerIndex);
            if (deformer && !deformerCtx->isOutsideParam()) {
                deformer->transformPoints(mc, deformerCtx,
                    ctx->mInterpolatedPoints, ctx->mTransformedPoints,
                    mPointCount, VERTEX_OFFSET, VERTEX_STEP);
                ctx->mAvailable = true;
            } else {
                ctx->mAvailable = false;
            }
            ctx->mBaseOpacity = deformerCtx->getTotalOpacity();
        }
    }
}

int sDrawCounts[3] = {0,0,0};
void Mesh::draw(DrawParamOpenGL* dp, ModelContext* mctx, MeshContext* dctx) {
    if (dctx->mParamOutside) return;
    if (mColorCompositionType >= 0 && mColorCompositionType < 3)
        sDrawCounts[mColorCompositionType]++;
    int texNr = mTextureNo;
    if (texNr < 0) texNr = 1;

    auto& vertices = !dctx->mTransformedPoints.empty()
                         ? dctx->mTransformedPoints
                         : dctx->mInterpolatedPoints;
    if (dctx->mClipBufPre_clipContext) {
        auto* cc = static_cast<ClipContext*>(dctx->mClipBufPre_clipContext);
        dp->mClipChannel = cc->mLayoutChannelNo;
        dp->setClipMatrix(cc->mMatrixForDraw.data());
    }
    dp->setClipBufPre_clipContextForDraw(dctx->mClipBufPre_clipContext);
    dp->setCulling(mCulling);

    auto* pctx2 = mctx->getPartsContext(dctx->mPartsIndex);
    float opacity = getOpacity(dctx) * dctx->mPartsOpacity * dctx->mBaseOpacity;
    dp->drawTexture(texNr, pctx2->mScreenColor, mIndexArray, vertices, mUvs,
                    opacity, mColorCompositionType, pctx2->mMultiplyColor);
}

} // namespace live2d
