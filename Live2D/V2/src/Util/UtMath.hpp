#pragma once

#include <cmath>

namespace live2d {

class UtMath {
public:
    static constexpr float DEG_TO_RAD = 0.017453292519943295f;
    static constexpr float RAD_TO_DEG = 57.29577951308232f;

    static float getAngleNotAbs(const float v1[2], const float v2[2]) {
        // Match Python UtMath.getAngleNotAbs: getAngleDiff(atan2(v1), atan2(v2))
        // = q1 - q2 normalized to [-π, π]
        float q1 = std::atan2(v1[1], v1[0]);
        float q2 = std::atan2(v2[1], v2[0]);
        float t = q1 - q2;
        while (t < -3.14159265f) t += 2.0f * 3.14159265f;
        while (t >  3.14159265f) t -= 2.0f * 3.14159265f;
        return t;
    }
};

} // namespace live2d
