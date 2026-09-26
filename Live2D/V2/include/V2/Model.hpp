#pragma once
#include "Framework/L2DBaseModel.hpp"
#include "Framework/L2DTargetPoint.hpp"
#include "Framework/MatrixManager.hpp"
#include "Graphics/GLRenderer.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>


namespace Live2D {
namespace V2 {
class Model : public L2DBaseModel
{
public:
    using StartCallback = std::function<void(const std::string&, int)>;
    using FinishCallback = std::function<void(const std::string&, int)>;

    Model();
    ~Model() override;
    void loadModelJson(const std::string& path, bool createRenderer = true);
    void resize(int w, int h);
    void drag(float x, float y);
    void touch(float x, float y);
    bool isMotionFinished() const;
    void setOffset(float dx, float dy);
    void setScale(float s);
    void setParameterValue(const std::string& id, float val, float weight = 1.0f);
    void addParameterValue(const std::string& id, float val, float weight = 1.0f);
    void setAutoBreathEnable(bool v) { mAutoBreath = v; }
    void setAutoBlinkEnable(bool v) { mAutoBlink = v; }
    int getParameterCount() const;
    int getPartCount() const;
    std::string getPartId(int index) const;
    void setPartOpacity(int index, float val);
    void update();
    void draw();
    bool hitTest(const std::string& area, float x, float y);
    void setExpression(const std::string& name);
    void setRandomExpression();
    void startMotion(const std::string& group, int no, int priority,
                     StartCallback onStart = nullptr, FinishCallback onFinish = nullptr);
    void startRandomMotion(const std::string& group, int priority, StartCallback onStart = nullptr,
                           FinishCallback onFinish = nullptr);
    void clearMotions();
    void stopAllMotions() { clearMotions(); }
    void resetExpression();
    void resetPose();
    float getCanvasWidth() const { return (float)mModelImpl->getCanvasWidth(); }
    float getCanvasHeight() const { return (float)mModelImpl->getCanvasHeight(); }
    int getPixelsPerUnit() const { return 1; }
    void rotate(float deg);
    float getParameterValue(int index) const;
    float getParameterMin(int index) const;
    float getParameterMax(int index) const;
    float getParameterDefault(int index) const;
    std::string getParameterId(int index) const;
    void setPartScreenColor(int index, float r, float g, float b, float a);
    void setPartMultiplyColor(int index, float r, float g, float b, float a);
    std::vector<float> getPartScreenColor(int index) const;
    std::vector<float> getPartMultiplyColor(int index) const;
    std::vector<std::string> hitPart(float x, float y, bool topOnly);

    // Rendering
    void CreateRenderer();
    void ReleaseRenderer();

    bool autoBreathEnabled() const { return mAutoBreath; }
    bool autoBlinkEnabled() const { return mAutoBlink; }

private:
    L2DTargetPoint mDragMgr;
    MatrixManager mMatrixManager;
    bool mAutoBreath = true, mAutoBlink = true;
    bool mClearFlag = false;
    std::string mModelHomeDir;

    StartCallback mOnStartMotion;
    FinishCallback mOnFinishMotion;
    bool mCallbacksPending = false;
    std::string mCurrentGroup;
    std::vector<std::string> mTexturePaths;
    int mCurrentMotionNo = 0;
    std::unique_ptr<GLRenderer> mRenderer;
};
}   // namespace V2
}   // namespace Live2D