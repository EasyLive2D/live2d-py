#include "BaseModel.hpp"
#include "AMotion.hpp"
#include "BinaryReader.hpp"
#include "Drawable.hpp"
#include "ExpressionMotion.hpp"
#include "EyeBlink.hpp"
#include "Id.hpp"
#include "Motion.hpp"
#include "Moc.hpp"
#include "MotionManager.hpp"
#include "Physics.hpp"
#include "Pose.hpp"
#include <memory>

namespace Live2D {
namespace V2 {

BaseModel::BaseModel() :
    mIdPool(), mMoc(nullptr), mEyeBlink(nullptr), mPhysics(std::make_unique<Physics>()),
    mPose(std::make_unique<Pose>()), mMainMotionMgr(std::make_unique<MotionManager>()),
    mExpressionMgr(std::make_unique<MotionManager>()) {

    mEyeBlink = std::make_unique<EyeBlink>(mIdPool);
}

BaseModel::~BaseModel() = default;

void BaseModel::LoadModelData(const std::vector<uint8_t>& data) {
    BinaryReader br(data, mIdPool);
    br.ReadByte();
    br.ReadByte();
    br.ReadByte();
    br.SetFormatVersion(br.ReadByte());
    mMoc.reset(static_cast<Moc*>(br.ReadObjectRaw()));
    mMoc->Init();
}
AMotion* BaseModel::LoadMotion(const std::string& name, const std::vector<uint8_t>& data) {
    auto* m = Motion::Load(data, mIdPool);
    mMotions[name].emplace_back(m);
    return m;
}
AMotion* BaseModel::LoadExpression(const std::string& name, const std::vector<uint8_t>& data) {
    auto* m = ExpressionMotion::Load(data, mIdPool);
    mExpressions[name].reset(m);
    return m;
}
Pose* BaseModel::LoadPose(const std::vector<uint8_t>& data) {
    mPose.reset(Pose::Load(data, mIdPool));
    return mPose.get();
}
void BaseModel::LoadPhysics(const std::vector<uint8_t>& data) {
    mPhysics.reset(Physics::Load(data, mIdPool));
}
bool BaseModel::HitTestSimple(const std::string& drawID, float x, float y) {
    auto* moc = mMoc.get();
    int drawIdx = moc->GetDrawableIndex(drawID);
    if (drawIdx < 0)
        return false;

    auto* dd = moc->GetDrawable(drawIdx);
    if (!dd || !dd->mAvailable)
        return false;

    auto& verts =
        !dd->mTransformedPoints.empty() ? dd->mTransformedPoints : dd->mInterpolatedPoints;
    if (verts.empty())
        return false;

    float left = (float)mMoc->GetCanvasWidth(), right = 0;
    float top = (float)mMoc->GetCanvasHeight(), bottom = 0;
    for (size_t j = 0; j < verts.size(); j += 2) {
        float vx = verts[j], vy = verts[j + 1];
        if (vx < left)
            left = vx;
        if (vx > right)
            right = vx;
        if (vy < top)
            top = vy;
        if (vy > bottom)
            bottom = vy;
    }

    // x/y 为 canvas 顶点坐标域（调用方经 mvp 逆变换得到），直接 AABB 判定
    return left <= x && x <= right && top <= y && y <= bottom;
}

}   // namespace V2
}   // namespace Live2D