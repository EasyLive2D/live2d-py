#include "L2DTargetPoint.hpp"
#include <cmath>

namespace live2d {

void L2DTargetPoint::update(float deltaSec) {
    float dx = mTargetX - mX;
    float dy = mTargetY - mY;
    if (std::abs(dx) < sEpsilon && std::abs(dy) < sEpsilon) return;
    // Smooth lerp: move ~10% per frame for responsive drag
    float t = deltaSec * 10.0f; if (t > 1.0f) t = 1.0f;
    mX += dx * t;
    mY += dy * t;
}

} // namespace live2d
