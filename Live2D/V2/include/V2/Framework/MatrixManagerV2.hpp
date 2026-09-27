#pragma once
#include <array>

namespace Live2D {
namespace V2 {
class L2DModelMatrix;

class MatrixManagerV2 {
public:
    MatrixManagerV2();

    void onResize(int width, int height);
    void setScale(float s);
    void setScaleX(float sx);
    void setScaleY(float sy);
    void setOffset(float dx, float dy);
    void rotate(float deg);
    std::array<float, 16> getMvp(L2DModelMatrix* modelMatrix) const;

    int getWidth() const { return mWidth; }
    int getHeight() const { return mHeight; }

private:
    int mWidth = 600, mHeight = 600;
    float mScaleX = 1.0f, mScaleY = 1.0f, mOffsetX = 0, mOffsetY = 0, mRotation = 0;
};

}   // namespace V2
}   // namespace Live2D