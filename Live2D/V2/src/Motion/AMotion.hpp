#pragma once
#include <string>
#include <functional>
namespace live2d {
class ALive2DModel;
class AMotion {
public:
    virtual ~AMotion() = default;
    virtual void updateParam(ALive2DModel* model, float timeSec, float weight) = 0;
    virtual float getDurationSec() const = 0;
    virtual bool isLoop() const = 0;
    virtual bool isFinished() const { return false; }
    void setFadeIn(float sec) { mFadeInSec = sec; }
    void setFadeOut(float sec) { mFadeOutSec = sec; }
    float mFadeInSec = 1.0f, mFadeOutSec = 1.0f;
    float mWeight = 1.0f;  // Default full weight (fade not yet implemented)
    std::function<void(std::string, int)> mOnStart;
    std::function<void(std::string, int)> mOnFinish;
};
} // namespace live2d
