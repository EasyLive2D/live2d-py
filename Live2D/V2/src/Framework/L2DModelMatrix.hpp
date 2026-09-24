#pragma once
#include <array>

namespace live2d {

class L2DModelMatrix {
public:
    L2DModelMatrix() { identity(); }
    L2DModelMatrix(float w, float h);

    void identity();
    void setWidth(float w);
    void setHeight(float h);
    void setCenterPosition(float x, float y);
    void translate(float x, float y);
    void scale(float sx, float sy);
    std::array<float, 16> getArray() const { return mMat; }
    float getScaleX() const { return mMat[0]; }
    float getScaleY() const { return mMat[5]; }
    float invertTransformX(float src) const { return (src - mMat[12]) / mMat[0]; }
    float invertTransformY(float src) const { return (src - mMat[13]) / mMat[5]; }

    float getScale() const { return mScale; }
    void setScale(float s) { mScale = s; }
    float getX() const { return mX; }
    void setX(float x) { mX = x; }
    float getY() const { return mY; }
    void setY(float y) { mY = y; }

    float mWidth = 0, mHeight = 0;
    float mX = 0, mY = 0;
    float mScale = 1.0f;

private:
    std::array<float, 16> mMat{};
};

} // namespace live2d
