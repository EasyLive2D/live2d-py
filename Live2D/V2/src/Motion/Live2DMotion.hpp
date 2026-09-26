#pragma once
#include "AMotion.hpp"
#include "Motion.hpp"
#include <cstdint>
#include <string>
#include <vector>
namespace Live2D {
namespace V2 {
class BinaryReader;
class ModelContext;
class Live2DMotion : public AMotion
{
public:
    Live2DMotion();
    void updateParam(ModelContext* context, float timeSec, float weight) override;
    float getDurationSec() const override;
    bool isLoop() const override;
    bool isFinished() const override { return mFinished; }
    void reset() override { mFinished = false; }
    static Live2DMotion* load(const std::vector<uint8_t>& data);
    std::vector<Motion> mMotions;
    int mFps = 30;
    float mDurationMs = 0;
    float mLoopDurationMs = 0;
    bool mLoop = false;
    bool mLoopFadeIn = true;
    bool mFinished = false;
};
}   // namespace V2
}   // namespace Live2D