#pragma once
#include "L2DBaseModel.hpp"
#include "L2DTargetPoint.hpp"
#include "MatrixManager.hpp"
#include "../Model/Live2DModelOpenGL.hpp"
#include <string>
namespace live2d {
class LAppModel : public L2DBaseModel {
public:
    LAppModel();
    ~LAppModel() override;
    bool loadModelJson(const std::string& path);
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
    void startMotion(const std::string& group, int no, int priority);
    void startRandomMotion(const std::string& group, int priority);
    void clearMotions();
    void stopAllMotions() { clearMotions(); }
    void resetExpression();
    void resetPose();
    float getCanvasWidth() const { return (float)mLive2DModel->getCanvasWidth(); }
    float getCanvasHeight() const { return (float)mLive2DModel->getCanvasHeight(); }
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
    void setTexture(int no, int texId);
    L2DTargetPoint mDragMgr;
    MatrixManager mMatrixManager;
    bool mAutoBreath = true, mAutoBlink = true;
    bool mClearFlag = false;
    std::string mModelHomeDir;
};
} // namespace live2d
