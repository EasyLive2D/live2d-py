#include "ModelContext.hpp"
#include "../Core/DEF.hpp"
#include "../Core/ModelImpl.hpp"
#include "../Core/ParamDefFloat.hpp"
#include "../Core/ParamDefSet.hpp"
#include "../Core/PartsData.hpp"
#include "../Core/PartsDataContext.hpp"
#include "../Deformer/Deformer.hpp"
#include "../Deformer/DeformerContext.hpp"
#include "../Draw/IDrawData.hpp"
#include "../Draw/Mesh.hpp"
#include "../Draw/MeshContext.hpp"
#include "../Graphics/ClippingManagerOpenGL.hpp"
#include "../Graphics/GLRenderer.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace live2d {

ModelContext::ModelContext()
{
    mTmpPivotTableIndices.resize(PIVOT_TABLE_SIZE);
    mTempTArray.resize(MAX_INTERPOLATION);
}

ModelContext::~ModelContext()
{
    release();
}

void ModelContext::release()
{
    mDeformerList.clear();
    mDrawDataList.clear();
    mPartsDataList.clear();
    mDeformerContextList.clear();
    mDrawContextList.clear();
    mPartsContextList.clear();
}

void ModelContext::init(ModelImpl* modelImpl)
{
    mInitVersion++;
    if (!mPartsDataList.empty())
        release();

    mCanvasWidth = modelImpl->getCanvasWidth();
    mCanvasHeight = modelImpl->getCanvasHeight();

    auto& partsDataList = modelImpl->getPartsDataList();

    std::vector<Deformer*> allDefs;
    std::vector<std::unique_ptr<DeformerContext>> allDefCtxs;

    for (auto& part : partsDataList) {
        mPartsDataList.push_back(part.get());
        mPartsContextList.emplace_back(part->init());

        auto& baseDefs = part->getDeformer();
        for (auto& d : baseDefs) {
            allDefs.push_back(d.get());
            auto ctx = std::unique_ptr<DeformerContext>(d->init(this));
            ctx->mPartsIndex = static_cast<int>(mPartsContextList.size()) - 1;
            allDefCtxs.push_back(std::move(ctx));
        }

        auto& drawData = part->getDrawData();
        for (auto& dd : drawData) {
            auto* mesh = static_cast<Mesh*>(dd.get());
            auto meshCtx = std::unique_ptr<MeshContext>(mesh->init(this));
            meshCtx->mPartsIndex = static_cast<int>(mPartsContextList.size()) - 1;
            mDrawDataList.push_back(mesh);
            mDrawContextList.push_back(std::move(meshCtx));
        }
    }

    // DAG sort deformers
    const Id& dstBase = Id::DST_BASE_ID();
    size_t nDef = allDefs.size();
    while (true) {
        bool progress = false;
        for (size_t i = 0; i < nDef; i++) {
            auto* d = allDefs[i];
            if (!d)
                continue;
            auto* target = d->getTargetId();
            if (!target || *target == dstBase || getDeformerIndex(target) >= 0) {
                mDeformerList.push_back(d);
                mDeformerContextList.push_back(std::move(allDefCtxs[i]));
                allDefs[i] = nullptr;
                progress = true;
            }
        }
        if (!progress)
            break;
    }

    auto* paramSet = modelImpl->getParamDefSet();
    if (paramSet) {
        for (auto& p : paramSet->getParamDefFloatList()) {
            if (!p)
                continue;
            extendAndAddParam(
                p->getParamID(), p->getDefaultValue(), p->getMaxValue(), p->getMinValue());
        }
    }

}

void ModelContext::update()
{
    for (size_t i = 0; i < mParamValues.size(); i++) {
        if (mParamValues[i] != mLastParamValues[i]) {
            mUpdatedParamFlags[i] = PARAM_UPDATED;
            mLastParamValues[i] = mParamValues[i];
        }
    }

    int nDef = static_cast<int>(mDeformerList.size());
    int nDraw = static_cast<int>(mDrawDataList.size());
    int minOrder = IDrawData::DEFAULT_ORDER;
    int maxOrder = IDrawData::DEFAULT_ORDER;
    for (auto* d : mDrawDataList) {
        int order = d->getAverageDrawOrder();
        if (order < minOrder)
            minOrder = order;
        else if (order > maxOrder)
            maxOrder = order;
    }
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
        mDeformerList[i]->setupInterpolate(this, mDeformerContextList[i].get());
        mDeformerList[i]->setupTransform(this, mDeformerContextList[i].get());
    }

    for (int i = 0; i < nDraw; i++) {
        auto* mesh = static_cast<Mesh*>(mDrawDataList[i]);
        auto* ctx = mDrawContextList[i].get();
        mesh->setupInterpolate(this, ctx);
        if (ctx->mParamOutside)
            continue;

        mesh->setupTransform(this, ctx);
        int order = static_cast<int>(std::floor(IDrawData::getDrawOrder(ctx) - minOrder));
        int lastIdx = mOrderListLastDrawIndex[order];
        if (lastIdx == NOT_USED_ORDER)
            mOrderListFirstDrawIndex[order] = i;
        else
            mNextListDrawIndex[lastIdx] = i;
        mOrderListLastDrawIndex[order] = i;
    }

    for (int i = static_cast<int>(mUpdatedParamFlags.size()) - 1; i >= 0; i--)
        mUpdatedParamFlags[i] = false;
}

int ModelContext::getParamIndex(const Id* paramId)
{
    auto it = mParamIndexCache.find(paramId);
    if (it != mParamIndexCache.end())
        return it->second;
    for (size_t i = 0; i < mParamIdList.size(); i++)
        if (mParamIdList[i] == paramId) {
            mParamIndexCache.emplace(paramId, static_cast<int>(i));
            return static_cast<int>(i);
        }
    int idx = extendAndAddParam(paramId, 0, PARAM_FLOAT_MAX, PARAM_FLOAT_MIN);
    mParamIndexCache.emplace(paramId, idx);
    return idx;
}

int ModelContext::getDeformerIndex(const Id* id)
{
    for (int i = static_cast<int>(mDeformerList.size()) - 1; i >= 0; i--)
        if (mDeformerList[i] && mDeformerList[i]->getId() == id)
            return i;
    return -1;
}

int ModelContext::extendAndAddParam(const Id* paramId, float dv, float maxV, float minV)
{
    mParamIdList.push_back(paramId);
    mParamValues.push_back(dv);
    mLastParamValues.push_back(dv);
    mParamMinValues.push_back(minV);
    mParamMaxValues.push_back(maxV);
    mParamDefaultValues.push_back(dv);
    mUpdatedParamFlags.push_back(false);
    return mNextParamPos++;
}

void ModelContext::setParamFloat(int index, float value)
{
    if (value < mParamMinValues[index])
        value = mParamMinValues[index];
    if (value > mParamMaxValues[index])
        value = mParamMaxValues[index];
    mParamValues[index] = value;
}

void ModelContext::setParamFloat(int index, float value, float weight)
{
    if (weight <= 0)
        return;
    if (weight >= 1) {
        setParamFloat(index, value);
        return;
    }
    float v = mParamValues[index] * (1.0f - weight) + value * weight;
    setParamFloat(index, v);
}

float ModelContext::getParamFloat(int index) const
{
    return (index >= 0 && index < (int)mParamValues.size()) ? mParamValues[index] : 0;
}

float ModelContext::getParamMax(int index) const
{
    return mParamMaxValues[index];
}
float ModelContext::getParamMin(int index) const
{
    return mParamMinValues[index];
}
float ModelContext::getParamDefault(int index) const
{
    return mParamDefaultValues[index];
}
bool ModelContext::isParamUpdated(int index) const
{
    return mUpdatedParamFlags[index];
}

void ModelContext::loadParam()
{
    for (size_t i = 0; i < mSavedParamValues.size(); i++) mParamValues[i] = mSavedParamValues[i];
}
void ModelContext::saveParam()
{
    if (mSavedParamValues.size() < mParamValues.size())
        mSavedParamValues.resize(mParamValues.size());
    for (size_t i = 0; i < mParamValues.size(); i++) mSavedParamValues[i] = mParamValues[i];
}

IDrawData* ModelContext::getDrawData(int index) const
{
    return (index >= 0 && index < (int)mDrawDataList.size()) ? mDrawDataList[index] : nullptr;
}
int ModelContext::getDrawDataIndex(const Id* drawDataId) const
{
    auto it = mDrawDataIndexCache.find(drawDataId);
    if (it != mDrawDataIndexCache.end())
        return it->second;
    for (int i = (int)mDrawDataList.size() - 1; i >= 0; i--)
        if (mDrawDataList[i] && mDrawDataList[i]->getId() == drawDataId) {
            mDrawDataIndexCache.emplace(drawDataId, i);
            return i;
        }
    mDrawDataIndexCache.emplace(drawDataId, -1);
    return -1;
}
int ModelContext::getPartsDataIndex(const Id* id) const
{
    auto it = mPartsDataIndexCache.find(id);
    if (it != mPartsDataIndexCache.end())
        return it->second;
    for (int i = (int)mPartsDataList.size() - 1; i >= 0; i--)
        if (mPartsDataList[i] && mPartsDataList[i]->getId() == id) {
            mPartsDataIndexCache.emplace(id, i);
            return i;
        }
    mPartsDataIndexCache.emplace(id, -1);
    return -1;
}
void ModelContext::setPartsOpacity(int i, float op)
{
    mPartsContextList[i]->setPartsOpacity(op);
}
float ModelContext::getPartsOpacity(int i) const
{
    return mPartsContextList[i]->getPartsOpacity();
}
void ModelContext::setPartMultiplyColor(int i, float r, float g, float b, float a)
{
    mPartsContextList[i]->setPartMultiplyColor(r, g, b, a);
}
void ModelContext::setPartScreenColor(int i, float r, float g, float b, float a)
{
    mPartsContextList[i]->setPartScreenColor(r, g, b, a);
}

}   // namespace live2d
