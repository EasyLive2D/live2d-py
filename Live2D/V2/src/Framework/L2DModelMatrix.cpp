#include "L2DModelMatrix.hpp"
#include <cstring>

namespace live2d {

L2DModelMatrix::L2DModelMatrix(float w, float h) : mWidth(w), mHeight(h) {
    identity();
}

void L2DModelMatrix::identity() {
    mMat.fill(0);
    mMat[0] = mMat[5] = mMat[10] = mMat[15] = 1.0f;
}

void L2DModelMatrix::setWidth(float w) {
    float sx = w / mWidth;
    float sy = -sx;
    scale(sx, sy);
}

void L2DModelMatrix::setHeight(float h) {
    float sx = h / mHeight;
    float sy = -sx;
    scale(sx, sy);
}

void L2DModelMatrix::setCenterPosition(float x, float y) {
    float w = mWidth * getScaleX();
    float h = mHeight * getScaleY();
    translate(x - w * 0.5f, y - h * 0.5f);
}

void L2DModelMatrix::translate(float x, float y) {
    // Match Python: absolute assignment, not relative accumulation
    mMat[12] = x;
    mMat[13] = y;
    mMat[14] = 0;
    mMat[15] = 1;
}

void L2DModelMatrix::scale(float sx, float sy) {
    // Match Python: assignment, NOT multiplication
    mMat[0] = sx;  mMat[1] = 0;  mMat[2] = 0;  mMat[3] = 0;
    mMat[4] = 0;   mMat[5] = sy; mMat[6] = 0;  mMat[7] = 0;
}

} // namespace live2d
