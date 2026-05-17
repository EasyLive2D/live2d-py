#pragma once

#include "../Core/ISerializable.hpp"

namespace live2d {

class AffineEnt final : public ISerializable {
public:
    AffineEnt() = default;

    void read(class BinaryReader& br) override;

    float mOriginX = 0.0f;
    float mOriginY = 0.0f;
    float mScaleX = 1.0f;
    float mScaleY = 1.0f;
    float mRotationDeg = 0.0f;
    bool mReflectX = false;
    bool mReflectY = false;
};

} // namespace live2d
