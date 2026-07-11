#pragma once
#include <cstdint>
#include "AMotion.hpp"
#include "Motion.hpp"
#include <vector>
#include <string>
namespace live2d {
class BinaryReader;
class Live2DMotion : public AMotion {
public:
    Live2DMotion();
    void updateParam(ALive2DModel* model, float timeSec, float weight) override;
    float getDurationSec() const override;
    bool isLoop() const override;
    bool isFinished() const override { return mFinished; }
    static Live2DMotion* load(const std::vector<uint8_t>& data);
    std::vector<Motion> mMotions;
    int mFps = 30;
    float mDurationMs = 0;
    float mLoopDurationMs = 0;
    bool mLoop = false;
    bool mLoopFadeIn = true;
    bool mFinished = false;
};
} // namespace live2d
