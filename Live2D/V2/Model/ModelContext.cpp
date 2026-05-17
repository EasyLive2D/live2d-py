#include "ModelContext.hpp"
#include "ALive2DModel.hpp"
#include "../Core/ModelImpl.hpp"
#include "../Core/ParamDefSet.hpp"
#include "../Core/ParamDefFloat.hpp"
#include "../Core/PartsData.hpp"
#include "../Core/PartsDataContext.hpp"
#include "../Core/DEF.hpp"
#include "../Deformer/Deformer.hpp"
#include "../Deformer/DeformerContext.hpp"
#include "../Deformer/RotationContext.hpp"
#include "../Deformer/AffineEnt.hpp"
#include "../Draw/IDrawData.hpp"
#include "../Draw/Mesh.hpp"
#include "../Draw/MeshContext.hpp"
#include "../Graphics/DrawParamOpenGL.hpp"
#include "../Graphics/ClippingManagerOpenGL.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace live2d {

ModelContext::ModelContext(ALive2DModel* model) : mModel(model) {
    mTmpPivotTableIndices.resize(PIVOT_TABLE_SIZE);
    mTempTArray.resize(MAX_INTERPOLATION);
}

ModelContext::~ModelContext() { release(); }

void ModelContext::release() {
    mDeformerList.clear(); mDrawDataList.clear(); mPartsDataList.clear();
    for (auto* ctx : mDeformerContextList) delete ctx;
    for (auto* ctx : mDrawContextList) delete ctx;
    for (auto* ctx : mPartsContextList) delete ctx;
    mDeformerContextList.clear(); mDrawContextList.clear(); mPartsContextList.clear();
    delete mClipManager; mClipManager = nullptr;
}

void ModelContext::init() {
    mInitVersion++;
    if (!mPartsDataList.empty()) release();

    auto* modelImpl = mModel->getModelImpl();
    auto& partsDataList = modelImpl->getPartsDataList();

    std::vector<Deformer*> allDefs;
    std::vector<DeformerContext*> allDefCtxs;

    for (auto* part : partsDataList) {
        mPartsDataList.push_back(part);
        mPartsContextList.push_back(part->init());

        auto& baseDefs = part->getDeformer();
        for (auto* d : baseDefs) {
            allDefs.push_back(d);
            auto* ctx = d->init(this);
            ctx->mPartsIndex = static_cast<int>(mPartsContextList.size()) - 1;
            allDefCtxs.push_back(ctx);
        }

        auto& drawData = part->getDrawData();
        for (auto* dd : drawData) {
            auto* mesh = static_cast<Mesh*>(dd); // drawData is always Mesh in v2
            auto* meshCtx = mesh->init(this);
            meshCtx->mPartsIndex = static_cast<int>(mPartsContextList.size()) - 1;
            mDrawDataList.push_back(mesh);
            mDrawContextList.push_back(meshCtx);
        }
    }

    // DAG sort deformers
    const Id& dstBase = Id::DST_BASE_ID();
    size_t nDef = allDefs.size();
    while (true) {
        bool progress = false;
        for (size_t i = 0; i < nDef; i++) {
            auto* d = allDefs[i];
            if (!d) continue;
            auto* target = d->getTargetId();
            if (!target || *target == dstBase || getDeformerIndex(target) >= 0) {
                mDeformerList.push_back(d);
                mDeformerContextList.push_back(allDefCtxs[i]);
                allDefs[i] = nullptr;
                progress = true;
            }
        }
        if (!progress) break;
    }

    auto* paramSet = modelImpl->getParamDefSet();
    if (paramSet) {
        for (auto* p : paramSet->getParamDefFloatList()) {
            if (!p) continue;
            extendAndAddParam(p->getParamID(), p->getDefaultValue(),
                              p->getMaxValue(), p->getMinValue());
        }
    }

    mClipManager = new ClippingManagerOpenGL(mDpGL);
    mClipManager->init(this, mDrawDataList, mDrawContextList);
    mNeedSetup = true;
}

void ModelContext::update() {
    for (size_t i = 0; i < mParamValues.size(); i++) {
        if (mParamValues[i] != mLastParamValues[i]) {
            mUpdatedParamFlags[i] = PARAM_UPDATED;
            mLastParamValues[i] = mParamValues[i];
        }
    }

    int nDef = static_cast<int>(mDeformerList.size());
    int nDraw = static_cast<int>(mDrawDataList.size());
    int minOrder = IDrawData::getTotalMinOrder();
    int maxOrder = IDrawData::getTotalMaxOrder();
    int range = maxOrder - minOrder + 1;

    if (static_cast<int>(mOrderListFirstDrawIndex.size()) < range) {
        mOrderListFirstDrawIndex.resize(range);
        mOrderListLastDrawIndex.resize(range);
    }
    for (int i = 0; i < range; i++) {
        mOrderListFirstDrawIndex[i] = NOT_USED_ORDER;
        mOrderListLastDrawIndex[i] = NOT_USED_ORDER;
    }
    if (static_cast<int>(mNextListDrawIndex.size()) < nDraw)
        mNextListDrawIndex.resize(nDraw);
    for (int i = 0; i < nDraw; i++) mNextListDrawIndex[i] = NO_NEXT;

    for (int i = 0; i < nDef; i++) {
        mDeformerList[i]->setupInterpolate(this, mDeformerContextList[i]);
        mDeformerList[i]->setupTransform(this, mDeformerContextList[i]);
    }

    for (int i = 0; i < nDraw; i++) {
        auto* dd = static_cast<Mesh*>(mDrawDataList[i]);
        auto* ctx = mDrawContextList[i];
        dd->setupInterpolate(this, ctx);
        if (ctx->mParamOutside) continue;

        dd->setupTransform(this, ctx);
        int order = static_cast<int>(std::floor(
            IDrawData::getDrawOrder(ctx) - minOrder));
        int lastIdx = mOrderListLastDrawIndex[order];
        if (lastIdx == NOT_USED_ORDER) mOrderListFirstDrawIndex[order] = i;
        else mNextListDrawIndex[lastIdx] = i;
        mOrderListLastDrawIndex[order] = i;
    }

    // One-time dump ALL deformer and draw data for comparison
    static bool sDumpDone = false;
    if (!sDumpDone && getenv("V2CPP_XFORM_DUMP")) {
        sDumpDone = true;
        FILE* f = fopen("xform_dump_v2cpp.txt", "w");
        if (f) {
            fprintf(f, "=== PARAMS (%zu) ===\n", mParamValues.size());
            for (size_t i = 0; i < mParamValues.size(); i++)
                fprintf(f, "[%zu] %s=%.6f\n", i, mParamIdList[i]->str().c_str(), mParamValues[i]);
            fprintf(f, "\n=== DEFORMERS ===\n");
            for (size_t i = 0; i < mDeformerList.size(); i++) {
                auto* def = mDeformerList[i];
                auto* dctx = mDeformerContextList[i];
                const char* name = def->getId()->str().c_str();
                fprintf(f, "[%zu] %s type=%d parent=%s avail=%d scale=%.6f opacity=%.6f\n",
                    i, name, def->getType(),
                    def->getTargetId() ? def->getTargetId()->str().c_str() : "?",
                    dctx->isAvailable() ? 1 : 0,
                    dctx->getTotalScale(), dctx->getTotalOpacity());
                // Dump affine values for RotationDeformers
                if (def->getType() == 1) { // TYPE_ROTATION
                    auto* rctx = static_cast<RotationContext*>(dctx);
                    auto* ia = rctx->mInterpolatedAffine;
                    auto* ta = rctx->mTransformedAffine;
                    if (ia) fprintf(f, "  interpolated: ox=%.4f oy=%.4f sx=%.4f sy=%.4f rot=%.4f rx=%d ry=%d\n",
                        ia->mOriginX, ia->mOriginY, ia->mScaleX, ia->mScaleY, ia->mRotationDeg, (int)ia->mReflectX, (int)ia->mReflectY);
                    if (ta) fprintf(f, "  transformed:  ox=%.4f oy=%.4f sx=%.4f sy=%.4f rot=%.4f rx=%d ry=%d\n",
                        ta->mOriginX, ta->mOriginY, ta->mScaleX, ta->mScaleY, ta->mRotationDeg, (int)ta->mReflectX, (int)ta->mReflectY);
                }
            }
            fprintf(f, "\n=== DRAWS ===\n");
            for (size_t i = 0; i < mDrawDataList.size(); i++) {
                auto* dd = static_cast<Mesh*>(mDrawDataList[i]);
                auto* ctx = mDrawContextList[i];
                auto& verts = ctx->mTransformedPoints.empty() ? ctx->mInterpolatedPoints : ctx->mTransformedPoints;
                const char* did = dd->getId() ? dd->getId()->str().c_str() : "?";
                const char* pid = mPartsContextList[ctx->mPartsIndex]->mPartsData->getId()->str().c_str();
                fprintf(f, "[%zu] %s/%s avail=%d nVtx=%zu op=%.3f\n",
                    i, pid, did, ctx->mAvailable ? 1 : 0,
                    verts.size() / VERTEX_STEP,
                    dd->getOpacity(ctx) * ctx->mPartsOpacity * ctx->mBaseOpacity);
                // Dump all vertices
                for (size_t v = VERTEX_OFFSET; v < verts.size(); v += VERTEX_STEP)
                    fprintf(f, "  vtx[%zu]=(%.4f,%.4f)\n", (v-VERTEX_OFFSET)/VERTEX_STEP, verts[v], verts[v+1]);
            }
            fclose(f);
        }
    }

    for (int i = static_cast<int>(mUpdatedParamFlags.size()) - 1; i >= 0; i--)
        mUpdatedParamFlags[i] = false;
    mNeedSetup = false;
}

void ModelContext::preDraw(DrawParamOpenGL* dp) {
    if (mClipManager) { dp->setupDraw(); mClipManager->setupClip(this, dp); }
}

void ModelContext::draw(DrawParamOpenGL* dp) {
    if (mOrderListFirstDrawIndex.empty()) return;
    dp->setupDraw();
    for (size_t i = 0; i < mOrderListFirstDrawIndex.size(); i++) {
        int idx = mOrderListFirstDrawIndex[i];
        if (idx == NOT_USED_ORDER) continue;
        while (true) {
            auto* dd = static_cast<Mesh*>(mDrawDataList[idx]);
            auto* ctx = mDrawContextList[idx];
            const char* pid = mPartsContextList[ctx->mPartsIndex]->mPartsData->getId()->str().c_str();
            if (ctx->mAvailable) {
                ctx->mPartsOpacity = mPartsContextList[ctx->mPartsIndex]->getPartsOpacity();
                const char* lbl = mDrawDataList[idx]->getId() ? mDrawDataList[idx]->getId()->str().c_str() : "?";
                if (glPushDebugGroup) glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, lbl);
                dd->draw(dp, this, ctx);
                if (glPopDebugGroup) glPopDebugGroup();
                // Per-draw framebuffer dump (enable with V2CPP_DUMP env var)
                static bool sDumpFrames = (getenv("V2CPP_DUMP") != nullptr);
                static int sFrameIdx = 0;
                if (sDumpFrames && sFrameIdx < 25) {
                    sFrameIdx++;
                    char fname[64];
                    snprintf(fname, sizeof(fname), "frame_%02d_%s.ppm", sFrameIdx,
                             mDrawDataList[idx]->getId()->str().c_str());
                    int vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
                    std::vector<uint8_t> px(vp[2] * vp[3] * 4);
                    glReadPixels(0, 0, vp[2], vp[3], GL_RGBA, GL_UNSIGNED_BYTE, px.data());
                    FILE* fp = fopen(fname, "wb");
                    if (fp) {
                        fprintf(fp, "P6\n%d %d\n255\n", vp[2], vp[3]);
                        for (int y = vp[3]-1; y >= 0; y--)
                            for (int x = 0; x < vp[2]; x++) {
                                int pi = (y * vp[2] + x) * 4;
                                fputc(px[pi], fp); fputc(px[pi+1], fp); fputc(px[pi+2], fp);
                            }
                        fclose(fp);
                    }
                }
            }
            int next = mNextListDrawIndex[idx];
            if (next <= idx || next == NO_NEXT) break;
            idx = next;
        }
    }
    dp->endDraw();
}

int ModelContext::getParamIndex(const Id* paramId) {
    for (size_t i = 0; i < mParamIdList.size(); i++)
        if (mParamIdList[i] == paramId) return static_cast<int>(i);
    return extendAndAddParam(paramId, 0, PARAM_FLOAT_MAX, PARAM_FLOAT_MIN);
}

int ModelContext::getDeformerIndex(const Id* id) {
    for (int i = static_cast<int>(mDeformerList.size()) - 1; i >= 0; i--)
        if (mDeformerList[i] && mDeformerList[i]->getId() == id) return i;
    return -1;
}

int ModelContext::extendAndAddParam(const Id* paramId, float dv, float maxV, float minV) {
    mParamIdList.push_back(paramId);
    mParamValues.push_back(dv); mLastParamValues.push_back(dv);
    mParamMinValues.push_back(minV); mParamMaxValues.push_back(maxV);
    mParamDefaultValues.push_back(dv);
    mUpdatedParamFlags.push_back(false);
    return mNextParamPos++;
}

void ModelContext::setParamFloat(int index, float value) {
    if (value < mParamMinValues[index]) value = mParamMinValues[index];
    if (value > mParamMaxValues[index]) value = mParamMaxValues[index];
    mParamValues[index] = value;
}

float ModelContext::getParamFloat(int index) const {
    return (index >= 0 && index < (int)mParamValues.size()) ? mParamValues[index] : 0;
}

float ModelContext::getParamMax(int index) const { return mParamMaxValues[index]; }
float ModelContext::getParamMin(int index) const { return mParamMinValues[index]; }
float ModelContext::getParamDefault(int index) const { return mParamDefaultValues[index]; }
bool ModelContext::isParamUpdated(int index) const { return mUpdatedParamFlags[index]; }

void ModelContext::loadParam() {
    for (size_t i = 0; i < mSavedParamValues.size(); i++) mParamValues[i] = mSavedParamValues[i];
}
void ModelContext::saveParam() {
    if (mSavedParamValues.size() < mParamValues.size()) mSavedParamValues.resize(mParamValues.size());
    for (size_t i = 0; i < mParamValues.size(); i++) mSavedParamValues[i] = mParamValues[i];
}

IDrawData* ModelContext::getDrawData(int index) const {
    return (index >= 0 && index < (int)mDrawDataList.size()) ? mDrawDataList[index] : nullptr;
}
int ModelContext::getDrawDataIndex(const Id* drawDataId) const {
    for (int i = (int)mDrawDataList.size() - 1; i >= 0; i--)
        if (mDrawDataList[i] && mDrawDataList[i]->getId() == drawDataId) return i;
    return -1;
}
int ModelContext::getPartsDataIndex(const Id* id) const {
    for (int i = (int)mPartsDataList.size() - 1; i >= 0; i--)
        if (mPartsDataList[i] && mPartsDataList[i]->getId() == id) return i;
    return -1;
}
void ModelContext::setPartsOpacity(int i, float op) { mPartsContextList[i]->setPartsOpacity(op); }
float ModelContext::getPartsOpacity(int i) const { return mPartsContextList[i]->getPartsOpacity(); }
void ModelContext::setPartMultiplyColor(int i, float r, float g, float b, float a) {
    mPartsContextList[i]->setPartMultiplyColor(r, g, b, a);
}
void ModelContext::setPartScreenColor(int i, float r, float g, float b, float a) {
    mPartsContextList[i]->setPartScreenColor(r, g, b, a);
}

} // namespace live2d
