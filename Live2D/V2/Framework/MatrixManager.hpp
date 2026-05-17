#pragma once
#include <array>

namespace live2d {
class L2DModelMatrix;

class MatrixManager {
public:
    MatrixManager();

    void onResize(int width, int height);
    void setScale(float s);
    void setOffset(float dx, float dy);
    void rotate(float deg);
    std::array<float, 16> getMvp(L2DModelMatrix* modelMatrix) const;

    int getWidth() const { return mWidth; }
    int getHeight() const { return mHeight; }

private:
    int mWidth = 600, mHeight = 600;
    float mScale = 1.0f, mOffsetX = 0, mOffsetY = 0, mRotation = 0;
};

} // namespace live2d
