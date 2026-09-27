#include "L2DTargetPoint.hpp"
#include <cmath>

namespace Live2D {
namespace V2 {

namespace {
// Python l2d_target_point.py 常量
constexpr float FRAME_RATE = 30.0f;
constexpr float TIME_TO_MAX_SPEED = 0.15f;
constexpr float FACE_PARAM_MAX_V = 40.0f / 7.5f;
constexpr float MAX_V = FACE_PARAM_MAX_V / FRAME_RATE;
constexpr float FRAME_TO_MAX_SPEED = TIME_TO_MAX_SPEED * FRAME_RATE;
constexpr float EPSILON = 0.01f;
}   // namespace

void L2DTargetPoint::update(float deltaSec) {
    updateByWeight(deltaSec * FRAME_RATE);
}

void L2DTargetPoint::updateByWeight(float deltaTimeWeight) {
    float dx = mTargetX - mX;
    float dy = mTargetY - mY;
    if (std::abs(dx) <= EPSILON && std::abs(dy) <= EPSILON)
        return;

    float maxA = deltaTimeWeight * MAX_V / FRAME_TO_MAX_SPEED;
    float d = std::sqrt(dx * dx + dy * dy);
    float vx = MAX_V * dx / d;
    float vy = MAX_V * dy / d;
    float ax = vx - mFaceVX;
    float ay = vy - mFaceVY;
    float a = std::sqrt(ax * ax + ay * ay);
    if (a < -maxA || a > maxA) {
        ax *= maxA / a;
        ay *= maxA / a;
    }

    mFaceVX += ax;
    mFaceVY += ay;

    float maxV = 0.5f * (std::sqrt(maxA * maxA + 16.0f * maxA * d - 8.0f * maxA * d) - maxA);
    float curV = std::sqrt(mFaceVX * mFaceVX + mFaceVY * mFaceVY);
    if (curV > maxV) {
        mFaceVX *= maxV / curV;
        mFaceVY *= maxV / curV;
    }

    mX += mFaceVX;
    mY += mFaceVY;
}

}   // namespace V2
}   // namespace Live2D
