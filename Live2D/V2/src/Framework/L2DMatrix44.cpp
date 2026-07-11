#include "L2DMatrix44.hpp"
#include <cstring>

namespace live2d {

L2DMatrix44::L2DMatrix44() { identity(); }

void L2DMatrix44::identity() {
    mMatrix.fill(0.0f);
    mMatrix[0] = mMatrix[5] = mMatrix[10] = mMatrix[15] = 1.0f;
}

void L2DMatrix44::setMatrix(const float m[16]) {
    std::memcpy(mMatrix.data(), m, 16 * sizeof(float));
}

void L2DMatrix44::multTranslate(float x, float y, float z) {
    mMatrix[12] += x; mMatrix[13] += y; mMatrix[14] += z;
}

void L2DMatrix44::multScale(float sx, float sy, float sz) {
    mMatrix[0] *= sx; mMatrix[5] *= sy; mMatrix[10] *= sz;
}

L2DMatrix44 L2DMatrix44::mul(const L2DMatrix44& a, const L2DMatrix44& b) {
    L2DMatrix44 r;
    auto& rm = r.getArray();
    auto& am = const_cast<L2DMatrix44&>(a).getArray();
    auto& bm = const_cast<L2DMatrix44&>(b).getArray();
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            rm[i + j * 4] = am[i] * bm[j * 4] + am[i + 4] * bm[j * 4 + 1]
                          + am[i + 8] * bm[j * 4 + 2] + am[i + 12] * bm[j * 4 + 3];
    return r;
}

} // namespace live2d
