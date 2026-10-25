#pragma once
#include "Moc.hpp"
#include <IModel.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include "Id.hpp"


namespace Live2D {
namespace V2 {

class Live2DModelOpenGL;
class MotionManager;
class EyeBlink;
class Pose;
class Physics;
class AMotion;


// 统一模型接口: Model -> BaseModel -> IModel 单继承链
class BaseModel : public IModel {
public:
    BaseModel();
    ~BaseModel() override;
    void LoadModelData(const std::vector<uint8_t>& data);
    AMotion* LoadMotion(const std::string& name, const std::vector<uint8_t>& data);
    AMotion* LoadExpression(const std::string& name, const std::vector<uint8_t>& data);
    Pose* LoadPose(const std::vector<uint8_t>& data);
    void LoadPhysics(const std::vector<uint8_t>& data);
    /// AABB 命中测试（drawID 匹配 drawable 的顶点包围盒）；x/y 为 canvas
    /// 顶点坐标域（与 mTransformedPoints 同域），像素坐标请先经
    /// Model::screenToModel 反算
    bool HitTestSimple(const std::string& drawID, float x, float y);
    MotionManager* GetMainMotionManager() const { return mMainMotionMgr.get(); }
    MotionManager* GetExpressionManager() const { return mExpressionMgr.get(); }
    void SetDrag(float x, float y) {
        mDragX = x;
        mDragY = y;
    }
    bool IsInitialized() const { return mInitialized; }
    void SetInitialized(bool v) { mInitialized = v; }
    bool IsUpdating() const { return mUpdating; }
    void SetUpdating(bool v) { mUpdating = v; }

protected:
    std::unique_ptr<EyeBlink> mEyeBlink;
    std::unique_ptr<Physics> mPhysics;
    std::unique_ptr<Pose> mPose;
    std::unique_ptr<MotionManager> mMainMotionMgr;
    std::unique_ptr<MotionManager> mExpressionMgr;
    std::unordered_map<std::string, std::vector<std::unique_ptr<AMotion>>> mMotions;
    std::unordered_map<std::string, std::unique_ptr<AMotion>> mExpressions;
    float mDragX = 0, mDragY = 0;
    bool mInitialized = false, mUpdating = false;
    std::unique_ptr<Moc> mMoc;
    Id::IdPool mIdPool;
};
}   // namespace V2
}   // namespace Live2D