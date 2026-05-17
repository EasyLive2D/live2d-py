#include "RotationDeformer.hpp"
#include "RotationContext.hpp"
#include "AffineEnt.hpp"
#include "DeformerContext.hpp"
#include "../Core/BinaryReader.hpp"
#include "../Core/PivotManager.hpp"
#include "../Model/ModelContext.hpp"
#include "../Util/UtMath.hpp"
#include <cmath>
#include <cstdio>

namespace live2d {

void RotationDeformer::read(BinaryReader& br) {
    Deformer::read(br);
    mPivotManager = br.readObject<PivotManager*>();
    mAffines = br.readObject<std::vector<AffineEnt*>>();
    Deformer::readOpacity(br);
}

DeformerContext* RotationDeformer::init(ModelContext* mc) {
    (void)mc;
    return new RotationContext(this);
}

void RotationDeformer::setupInterpolate(ModelContext* mc, DeformerContext* dc) {
    auto* rctx = static_cast<RotationContext*>(dc);
    if (!mPivotManager->checkParamUpdated(mc)) return;

    bool success = false;
    int pivotCount = mPivotManager->calcPivotValues(mc, success);
    rctx->setOutsideParam(success);
    interpolateOpacity(mc, mPivotManager, rctx, success);

    auto& pivotIndices = mc->getTempPivotTableIndices();
    auto& pivotT = mc->getTempT();
    mPivotManager->calcPivotIndices(pivotIndices, pivotT, pivotCount);

    if (pivotCount <= 0) {
        auto* a = mAffines[pivotIndices[0]];
        rctx->mInterpolatedAffine->mOriginX = a->mOriginX;
        rctx->mInterpolatedAffine->mOriginY = a->mOriginY;
        rctx->mInterpolatedAffine->mScaleX = a->mScaleX;
        rctx->mInterpolatedAffine->mScaleY = a->mScaleY;
        rctx->mInterpolatedAffine->mRotationDeg = a->mRotationDeg;
    } else {
        int tableSize = 1 << pivotCount;
        std::vector<float> weights(tableSize);
        for (int i = 0; i < tableSize; i++) {
            float idx = static_cast<float>(i), w = 1.0f;
            for (int j = 0; j < pivotCount; j++) {
                w *= (static_cast<int>(idx) % 2 == 0) ? (1.0f - pivotT[j]) : pivotT[j];
                idx /= 2.0f;
            }
            weights[i] = w;
        }
        float ox = 0, oy = 0, sx = 0, sy = 0, rot = 0;
        for (int i = 0; i < tableSize; i++) {
            auto* a = mAffines[pivotIndices[i]];
            ox += weights[i] * a->mOriginX; oy += weights[i] * a->mOriginY;
            sx += weights[i] * a->mScaleX; sy += weights[i] * a->mScaleY;
            rot += weights[i] * a->mRotationDeg;
        }
        rctx->mInterpolatedAffine->mOriginX = ox; rctx->mInterpolatedAffine->mOriginY = oy;
        rctx->mInterpolatedAffine->mScaleX = sx; rctx->mInterpolatedAffine->mScaleY = sy;
        rctx->mInterpolatedAffine->mRotationDeg = rot;
    }
    auto* ref = mAffines[pivotIndices[0]];
    rctx->mInterpolatedAffine->mReflectX = ref->mReflectX;
    rctx->mInterpolatedAffine->mReflectY = ref->mReflectY;
}

static void getDirectionOnDst(ModelContext* mc, Deformer* targetDef, DeformerContext* tgtCtx,
                              float ox, float oy, float dx, float dy, float retDir[2]) {
    std::vector<float> tpVec = {ox, oy};
    std::vector<float> toVec(2);
    targetDef->transformPoints(mc, tgtCtx, tpVec, toVec, 1, 0, 2);

    float stepSize = 1.0f;
    for (int it = 0; it < 10; it++) {
        std::vector<float> testVec = {ox + stepSize * dx, oy + stepSize * dy};
        std::vector<float> transVec(2);
        targetDef->transformPoints(mc, tgtCtx, testVec, transVec, 1, 0, 2);
        transVec[0] -= toVec[0]; transVec[1] -= toVec[1];
        if (transVec[0] != 0 || transVec[1] != 0) {
            retDir[0] = transVec[0]; retDir[1] = transVec[1];
            return;
        }
        testVec = {ox - stepSize * dx, oy - stepSize * dy};
        targetDef->transformPoints(mc, tgtCtx, testVec, transVec, 1, 0, 2);
        transVec[0] -= toVec[0]; transVec[1] -= toVec[1];
        if (transVec[0] != 0 || transVec[1] != 0) {
            retDir[0] = -transVec[0]; retDir[1] = -transVec[1];
            return;
        }
        stepSize *= 0.1f;
    }
}

bool RotationDeformer::setupTransform(ModelContext* mc, DeformerContext* dc) {
    auto* rctx = static_cast<RotationContext*>(dc);
    rctx->setAvailable(true);

    if (!needTransform()) {
        rctx->setTotalScale_notForClient(rctx->mInterpolatedAffine->mScaleX);
        rctx->setTotalOpacity(rctx->getInterpolatedOpacity());
        return true;
    }

    auto* targetId = getTargetId();
    if (rctx->mTmpDeformerIndex == DEFORMER_INDEX_NOT_INIT)
        rctx->mTmpDeformerIndex = mc->getDeformerIndex(targetId);

    if (rctx->mTmpDeformerIndex < 0) {
        rctx->setAvailable(false);
        return false;
    }

    auto* parentDef = mc->getDeformer(rctx->mTmpDeformerIndex);
    auto* parentCtx = mc->getDeformerContext(rctx->mTmpDeformerIndex);
    if (!parentDef || !parentCtx) { rctx->setAvailable(false); return false; }

    float ox = rctx->mInterpolatedAffine->mOriginX;
    float oy = rctx->mInterpolatedAffine->mOriginY;
    float dx = 0, dy = (parentCtx->getDeformer()->getType() == TYPE_ROTATION) ? -10.0f : -0.1f;

    float tDir[2];
    getDirectionOnDst(mc, parentDef, parentCtx, ox, oy, dx, dy, tDir);
    float dir[2] = {dx, dy};
    float angle = UtMath::getAngleNotAbs(dir, tDir);

    std::vector<float> originVec = {ox, oy};
    std::vector<float> originDst(2);
    parentDef->transformPoints(mc, parentCtx, originVec, originDst, 1, 0, 2);

    rctx->mTransformedAffine->mOriginX = originDst[0];
    rctx->mTransformedAffine->mOriginY = originDst[1];
    rctx->mTransformedAffine->mScaleX = rctx->mInterpolatedAffine->mScaleX;
    rctx->mTransformedAffine->mScaleY = rctx->mInterpolatedAffine->mScaleY;
    rctx->mTransformedAffine->mRotationDeg =
        rctx->mInterpolatedAffine->mRotationDeg - angle * UtMath::RAD_TO_DEG;

    float ps = parentCtx->getTotalScale();
    rctx->setTotalScale_notForClient(ps * rctx->mTransformedAffine->mScaleX);
    rctx->setTotalOpacity(parentCtx->getTotalOpacity() * rctx->getInterpolatedOpacity());
    rctx->mTransformedAffine->mReflectX = rctx->mInterpolatedAffine->mReflectX;
    rctx->mTransformedAffine->mReflectY = rctx->mInterpolatedAffine->mReflectY;
    rctx->setAvailable(parentCtx->isAvailable());
    return rctx->isAvailable();
}

void RotationDeformer::transformPoints(ModelContext*, DeformerContext* dc,
                                       const std::vector<float>& src,
                                       std::vector<float>& dst,
                                       int numPoint, int ptOffset, int ptStep) {
    auto* rctx = static_cast<RotationContext*>(dc);
    auto* af = rctx->mTransformedAffine ? rctx->mTransformedAffine
                                        : rctx->mInterpolatedAffine;
    float sinR = std::sin(UtMath::DEG_TO_RAD * af->mRotationDeg);
    float cosR = std::cos(UtMath::DEG_TO_RAD * af->mRotationDeg);
    float ts = rctx->getTotalScale();
    float rx = af->mReflectX ? -1.0f : 1.0f, ry = af->mReflectY ? -1.0f : 1.0f;
    float sx = cosR * ts * rx, shx = -sinR * ts * ry;
    float shy = sinR * ts * rx, sy = cosR * ts * ry;
    float ox = af->mOriginX, oy = af->mOriginY;

    int total = numPoint * ptStep;
    for (int i = ptOffset; i < total; i += ptStep) {
        float x = src[i], y = src[i + 1];
        dst[i] = sx * x + shx * y + ox;
        dst[i + 1] = shy * x + sy * y + oy;
    }
}

} // namespace live2d
