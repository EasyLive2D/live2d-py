#include "RotationDeformer.hpp"
#include "BinaryReader.hpp"
#include "DEF.hpp"
#include "PivotManager.hpp"
#include "Moc.hpp"
#include "AffineEnt.hpp"
#include <cmath>
#include <vector>

namespace Live2D {
namespace V2 {

namespace {

// 1:1 等价于原 UtMath.hpp 中的常量（与原字面量折叠为同一 float 位模式）
constexpr float PI = 3.14159265f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

static float GetAngleNotAbs(const float v1[2], const float v2[2]) {
    // Match Python UtMath.getAngleNotAbs: getAngleDiff(atan2(v1), atan2(v2))
    // = q1 - q2 normalized to [-π, π]
    float q1 = std::atan2(v1[1], v1[0]);
    float q2 = std::atan2(v2[1], v2[0]);
    float t = q1 - q2;
    while (t < -PI)
        t += 2.0f * PI;
    while (t > PI)
        t -= 2.0f * PI;
    return t;
}
}   // namespace

RotationDeformer::~RotationDeformer() = default;

void RotationDeformer::Read(BinaryReader& br) {
    Deformer::Read(br);
    mPivotManager.reset(br.ReadObject<PivotManager*>());
    auto rawAffines = br.ReadObject<std::vector<AffineEnt*>>();
    mAffines.reserve(rawAffines.size());
    for (auto* a : rawAffines)
        mAffines.emplace_back(a);
    Deformer::ReadOpacity(br);
}

void RotationDeformer::InitRuntimeState() {
    mInterpolatedAffine = std::make_unique<AffineEnt>();
    if (NeedTransform()) {
        mTransformedAffine = std::make_unique<AffineEnt>();
    }
}

void RotationDeformer::SetupInterpolate(Moc* moc) {
    bool outOfRange = false;
    int pivotCount = mPivotManager->CalcPivotValues(moc, outOfRange);
    SetParamOutOfRange(outOfRange);
    InterpolateOpacity(moc, mPivotManager.get(), outOfRange);

    auto& pivotIndices = moc->GetTempPivotTableIndices();
    auto& pivotT = moc->GetTempT();
    mPivotManager->CalcPivotIndices(pivotIndices, pivotT, pivotCount);

    if (pivotCount <= 0) {
        auto* a = mAffines[pivotIndices[0]].get();
        mInterpolatedAffine->mOriginX = a->mOriginX;
        mInterpolatedAffine->mOriginY = a->mOriginY;
        mInterpolatedAffine->mScaleX = a->mScaleX;
        mInterpolatedAffine->mScaleY = a->mScaleY;
        mInterpolatedAffine->mRotationDeg = a->mRotationDeg;
    } else {
        int tableSize = 1 << pivotCount;
        float weights[1 << MAX_INTERPOLATION];
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
            auto* a = mAffines[pivotIndices[i]].get();
            ox += weights[i] * a->mOriginX;
            oy += weights[i] * a->mOriginY;
            sx += weights[i] * a->mScaleX;
            sy += weights[i] * a->mScaleY;
            rot += weights[i] * a->mRotationDeg;
        }
        mInterpolatedAffine->mOriginX = ox;
        mInterpolatedAffine->mOriginY = oy;
        mInterpolatedAffine->mScaleX = sx;
        mInterpolatedAffine->mScaleY = sy;
        mInterpolatedAffine->mRotationDeg = rot;
    }
    auto* ref = mAffines[pivotIndices[0]].get();
    mInterpolatedAffine->mReflectX = ref->mReflectX;
    mInterpolatedAffine->mReflectY = ref->mReflectY;
}

static void getDirectionOnDst(Moc* moc, Deformer* targetDef, float ox, float oy, float dx, float dy,
                              float retDir[2]) {
    static std::vector<float> tpVec(2), toVec(2), testVec(2), transVec(2);
    tpVec[0] = ox, tpVec[1] = oy;
    targetDef->TransformPoints(tpVec, toVec, 1, 0, 2);

    float stepSize = 1.0f;
    for (int it = 0; it < 10; it++) {
        testVec[0] = ox + stepSize * dx, testVec[1] = oy + stepSize * dy;
        targetDef->TransformPoints(testVec, transVec, 1, 0, 2);
        transVec[0] -= toVec[0];
        transVec[1] -= toVec[1];
        if (transVec[0] != 0 || transVec[1] != 0) {
            retDir[0] = transVec[0];
            retDir[1] = transVec[1];
            return;
        }
        testVec[0] = ox - stepSize * dx, testVec[1] = oy - stepSize * dy;
        targetDef->TransformPoints(testVec, transVec, 1, 0, 2);
        transVec[0] -= toVec[0];
        transVec[1] -= toVec[1];
        if (transVec[0] != 0 || transVec[1] != 0) {
            retDir[0] = -transVec[0];
            retDir[1] = -transVec[1];
            return;
        }
        stepSize *= 0.1f;
    }
}

bool RotationDeformer::SetupTransform(Moc* moc) {
    SetAvailable(true);

    if (!NeedTransform()) {
        SetTotalScale_notForClient(mInterpolatedAffine->mScaleX);
        SetTotalOpacity(GetInterpolatedOpacity());
        return true;
    }

    auto* targetId = GetTargetId();
    if (mTmpDeformerIndex == DEFORMER_INDEX_NOT_INIT)
        mTmpDeformerIndex = moc->GetDeformerIndex(targetId);

    if (mTmpDeformerIndex < 0) {
        SetAvailable(false);
        return false;
    }

    auto* parentDef = moc->GetDeformer(mTmpDeformerIndex);
    if (!parentDef) {
        SetAvailable(false);
        return false;
    }

    float ox = mInterpolatedAffine->mOriginX;
    float oy = mInterpolatedAffine->mOriginY;
    float dx = 0, dy = (parentDef->GetType() == TYPE_ROTATION) ? -10.0f : -0.1f;

    float tDir[2];
    getDirectionOnDst(moc, parentDef, ox, oy, dx, dy, tDir);
    float dir[2] = {dx, dy};
    float angle = GetAngleNotAbs(dir, tDir);

    std::vector<float> originVec = {ox, oy};
    std::vector<float> originDst(2);
    parentDef->TransformPoints(originVec, originDst, 1, 0, 2);

    mTransformedAffine->mOriginX = originDst[0];
    mTransformedAffine->mOriginY = originDst[1];
    mTransformedAffine->mScaleX = mInterpolatedAffine->mScaleX;
    mTransformedAffine->mScaleY = mInterpolatedAffine->mScaleY;
    mTransformedAffine->mRotationDeg = mInterpolatedAffine->mRotationDeg - angle * RAD_TO_DEG;

    float ps = parentDef->GetTotalScale();
    SetTotalScale_notForClient(ps * mTransformedAffine->mScaleX);
    SetTotalOpacity(parentDef->GetTotalOpacity() * GetInterpolatedOpacity());
    mTransformedAffine->mReflectX = mInterpolatedAffine->mReflectX;
    mTransformedAffine->mReflectY = mInterpolatedAffine->mReflectY;
    SetAvailable(parentDef->IsAvailable());
    return IsAvailable();
}

void RotationDeformer::TransformPoints(const std::vector<float>& src, std::vector<float>& dst,
                                       int numPoint, int ptOffset, int ptStep) {
    auto* af = mTransformedAffine ? mTransformedAffine.get() : mInterpolatedAffine.get();
    float sinR = std::sin(DEG_TO_RAD * af->mRotationDeg);
    float cosR = std::cos(DEG_TO_RAD * af->mRotationDeg);
    float ts = GetTotalScale();
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

}   // namespace V2
}   // namespace Live2D