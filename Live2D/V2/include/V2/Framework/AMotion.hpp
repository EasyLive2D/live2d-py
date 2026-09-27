#pragma once
#include <functional>
#include <string>
namespace Live2D {
namespace V2 {
class ModelContext;
class AMotion
{
public:
    virtual ~AMotion() = default;
    virtual void updateParam(ModelContext* context, float timeSec, float weight) = 0;
    virtual float getDurationSec() const = 0;
    virtual bool isLoop() const = 0;
    virtual bool isFinished() const { return false; }
    virtual void reset() {}
    void setFadeIn(float sec) { mFadeInSec = sec; }
    void setFadeOut(float sec) { mFadeOutSec = sec; }
    float mFadeInSec = 1.0f, mFadeOutSec = 1.0f;
    float mWeight = 1.0f;   // Default full weight (fade not yet implemented)
    std::function<void(std::string, int)> mOnStart;
    std::function<void(std::string, int)> mOnFinish;
};
}   // namespace V2
}   // namespace Live2D