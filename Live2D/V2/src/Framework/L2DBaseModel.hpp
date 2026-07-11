#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include "../Model/ALive2DModel.hpp"
#include "L2DModelMatrix.hpp"
namespace live2d {
class Live2DModelOpenGL;
class L2DMotionManager;
class L2DEyeBlink;
class L2DPose;
class L2DPhysics;
class AMotion;
class L2DBaseModel {
public:
    L2DBaseModel();
    virtual ~L2DBaseModel();
    void loadModelData(const std::vector<uint8_t>& data, int version);
    AMotion* loadMotion(const std::string& name, const std::vector<uint8_t>& data);
    AMotion* loadExpression(const std::string& name, const std::vector<uint8_t>& data);
    L2DPose* loadPose(const std::vector<uint8_t>& data);
    void loadPhysics(const std::vector<uint8_t>& data);
    bool hitTestSimple(const std::string& drawID, float x, float y);
    Live2DModelOpenGL* getLive2DModel() const { return mLive2DModel.get(); }
    L2DModelMatrix& getModelMatrix() { return mModelMatrix; }
    L2DMotionManager* getMainMotionManager() const { return mMainMotionMgr.get(); }
    L2DMotionManager* getExpressionManager() const { return mExpressionMgr.get(); }
    void setAlpha(float a) { mAlpha = (a < 0 ? 0 : (a > 1 ? 1 : a)); }
    float getAlpha() const { return mAlpha; }
    void setAccel(float x, float y, float z) { mAccelX = x; mAccelY = y; mAccelZ = z; }
    void setDrag(float x, float y) { mDragX = x; mDragY = y; }
    bool isInitialized() const { return mInitialized; }
    void setInitialized(bool v) { mInitialized = v; }
    bool isUpdating() const { return mUpdating; }
    void setUpdating(bool v) { mUpdating = v; }
    std::unique_ptr<Live2DModelOpenGL> mLive2DModel;
    L2DModelMatrix mModelMatrix;
    std::unique_ptr<L2DEyeBlink> mEyeBlink;
    std::unique_ptr<L2DPhysics> mPhysics;
    std::unique_ptr<L2DPose> mPose;
    std::unique_ptr<L2DMotionManager> mMainMotionMgr;
    std::unique_ptr<L2DMotionManager> mExpressionMgr;
    std::unordered_map<std::string, std::vector<std::unique_ptr<AMotion>>> mMotions;
    std::unordered_map<std::string, std::unique_ptr<AMotion>> mExpressions;
    float mAlpha = 1.0f, mAccelX = 0, mAccelY = 0, mAccelZ = 0, mDragX = 0, mDragY = 0;
    bool mInitialized = false, mUpdating = false;
};
} // namespace live2d
