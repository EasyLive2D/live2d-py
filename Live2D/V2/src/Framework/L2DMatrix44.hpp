#pragma once

#include <array>

namespace live2d {

class L2DMatrix44 {
public:
    L2DMatrix44();

    void identity();
    void setMatrix(const float m[16]);
    void multTranslate(float x, float y, float z);
    void multScale(float sx, float sy, float sz);
    std::array<float, 16>& getArray() { return mMatrix; }

    static L2DMatrix44 mul(const L2DMatrix44& a, const L2DMatrix44& b);

private:
    std::array<float, 16> mMatrix;
};

} // namespace live2d
