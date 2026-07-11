#include "MatrixManager.hpp"
#include "L2DModelMatrix.hpp"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace live2d {

MatrixManager::MatrixManager() = default;

void MatrixManager::onResize(int w, int h) { mWidth = w; mHeight = h; }
void MatrixManager::setScale(float s) { mScale = s; }
void MatrixManager::setOffset(float dx, float dy) { mOffsetX = dx; mOffsetY = dy; }
void MatrixManager::rotate(float deg) { mRotation = deg; }

// 4x4 matrix multiply: r = a * b (column-major)
static void mul(float r[16], const float a[16], const float b[16]) {
    float t[16] = {};
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            t[i + j * 4] = a[i] * b[j * 4] + a[i + 4] * b[j * 4 + 1]
                         + a[i + 8] * b[j * 4 + 2] + a[i + 12] * b[j * 4 + 3];
    std::memcpy(r, t, sizeof(t));
}

std::array<float, 16> MatrixManager::getMvp(L2DModelMatrix* modelMatrix) const {
    // Match Python v2 MatrixManager.getMvp exactly

    // 1. Model matrix: copy and apply setWidth(2.0)
    float mm[16] = {};
    mm[0] = mm[5] = mm[10] = mm[15] = 1.0f;

    float canvasW = modelMatrix->mWidth > 0 ? modelMatrix->mWidth : 1.0f;
    float canvasH = modelMatrix->mHeight > 0 ? modelMatrix->mHeight : 1.0f;

    // setWidth(2.0): scale_x = 2/w, scale_y = -scale_x
    float sx_m = 2.0f / canvasW;
    float sy_m = -sx_m;
    mm[0] = sx_m * modelMatrix->getScale();
    mm[5] = sy_m * modelMatrix->getScale();

    // Model matrix translation from centerPosition
    float mm_w = canvasW * mm[0];  // width * scaleX
    float mm_h = canvasH * mm[5];  // height * scaleY (negative)
    mm[12] = 0 - mm_w * 0.5f;       // setCenterPosition(0,0)
    mm[13] = 0 - mm_h * 0.5f;

    // 2. Projection matrix
    float proj[16] = {};
    proj[0] = proj[5] = proj[10] = proj[15] = 1.0f;

    if (mHeight > mWidth) {
        // height > width: scale(1, w/h)
        proj[0] = 1.0f;
        proj[5] = (float)mWidth / mHeight;
    } else {
        proj[0] = (float)mHeight / mWidth;
        proj[5] = 1.0f;
    }

    // Apply user scale
    proj[0] *= mScale;
    proj[5] *= mScale;

    // Apply user offset
    proj[12] = mOffsetX;
    proj[13] = mOffsetY;

    // 3. proj = proj * mm
    mul(proj, proj, mm);

    // 4. Apply rotation: proj = rotation * proj
    if (mRotation != 0.0f) {
        float r = mRotation * 3.14159265f / 180.0f;
        float rot[16] = {};
        rot[0] = std::cos(r);  rot[4] = std::sin(r);
        rot[1] = -std::sin(r); rot[5] = std::cos(r);
        rot[10] = 1.0f; rot[15] = 1.0f;
        mul(proj, rot, proj);
    }

    std::array<float, 16> result;
    std::memcpy(result.data(), proj, sizeof(proj));
    return result;
}

} // namespace live2d
