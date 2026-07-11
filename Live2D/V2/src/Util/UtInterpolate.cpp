#include "UtInterpolate.hpp"
#include "../Model/ModelContext.hpp"
#include "../Core/PivotManager.hpp"
#include <cmath>

namespace live2d {

static void arraycopy(const float* src, int srcOff, float* dst, int dstOff, int count) {
    for (int i = 0; i < count; i++) dst[dstOff + i] = src[srcOff + i];
}

float UtInterpolate::interpolateFloat(ModelContext* mdc, PivotManager* pivotMgr, bool& ret,
                                      const std::vector<float>& pivotValue) {
    int interpCount = pivotMgr->calcPivotValues(mdc, ret);
    auto& indices = mdc->getTempPivotTableIndices();
    auto& tVals = mdc->getTempT();
    pivotMgr->calcPivotIndices(indices, tVals, interpCount);

    if (interpCount <= 0) return pivotValue[indices[0]];

    if (interpCount == 1) {
        float bj = pivotValue[indices[0]], bi = pivotValue[indices[1]];
        return bj + (bi - bj) * tVals[0];
    }

    // Generic path for interpCount >= 2
    int tableSize = 1 << interpCount;
    std::vector<float> weights(tableSize);
    for (int i = 0; i < tableSize; i++) {
        float idx = static_cast<float>(i);
        float w = 1.0f;
        for (int j = 0; j < interpCount; j++) {
            w *= (static_cast<int>(idx) % 2 == 0) ? (1.0f - tVals[j]) : tVals[j];
            idx /= 2.0f;
        }
        weights[i] = w;
    }

    float result = 0.0f;
    for (int i = 0; i < tableSize; i++) {
        result += weights[i] * pivotValue[indices[i]];
    }
    return result;
}

int UtInterpolate::interpolateInt(ModelContext* mdc, PivotManager* pivotMgr, bool& ret,
                                   const std::vector<int>& pivotValue) {
    int interpCount = pivotMgr->calcPivotValues(mdc, ret);
    auto& indices = mdc->getTempPivotTableIndices();
    auto& tVals = mdc->getTempT();
    pivotMgr->calcPivotIndices(indices, tVals, interpCount);

    if (interpCount <= 0) return pivotValue[indices[0]];

    int tableSize = 1 << interpCount;
    std::vector<float> weights(tableSize);
    for (int i = 0; i < tableSize; i++) {
        float idx = static_cast<float>(i);
        float w = 1.0f;
        for (int j = 0; j < interpCount; j++) {
            w *= (static_cast<int>(idx) % 2 == 0) ? (1.0f - tVals[j]) : tVals[j];
            idx /= 2.0f;
        }
        weights[i] = w;
    }

    float result = 0.0f;
    for (int i = 0; i < tableSize; i++) {
        result += weights[i] * static_cast<float>(pivotValue[indices[i]]);
    }
    return static_cast<int>(result + 0.5f);
}

void UtInterpolate::interpolatePoints(ModelContext* mdc, PivotManager* pivotMgr, bool& ret,
                                      int pointCount,
                                      const std::vector<std::vector<float>>& pivotPoints,
                                      std::vector<float>& dstPoints, int ptOffset, int ptStep) {
    int interpCount = pivotMgr->calcPivotValues(mdc, ret);
    auto& indices = mdc->getTempPivotTableIndices();
    auto& tVals = mdc->getTempT();
    pivotMgr->calcPivotIndices(indices, tVals, interpCount);

    int coordCount = pointCount * 2;
    int dstIdx = ptOffset;

    if (interpCount <= 0) {
        auto& base = pivotPoints[indices[0]];
        if (ptStep == 2 && ptOffset == 0) {
            arraycopy(base.data(), 0, dstPoints.data(), 0, coordCount);
        } else {
            for (int i = 0; i < coordCount; i += 2) {
                dstPoints[dstIdx] = base[i];
                dstPoints[dstIdx + 1] = base[i + 1];
                dstIdx += ptStep;
            }
        }
        return;
    }

    int tableSize = 1 << interpCount;
    std::vector<float> weights(tableSize);
    for (int i = 0; i < tableSize; i++) {
        float idx = static_cast<float>(i);
        float w = 1.0f;
        for (int j = 0; j < interpCount; j++) {
            w *= (static_cast<int>(idx) % 2 == 0) ? (1.0f - tVals[j]) : tVals[j];
            idx /= 2.0f;
        }
        weights[i] = w;
    }

    for (int i = 0; i < coordCount; i += 2) {
        float x = 0.0f, y = 0.0f;
        for (int j = 0; j < tableSize; j++) {
            auto& base = pivotPoints[indices[j]];
            x += weights[j] * base[i];
            y += weights[j] * base[i + 1];
        }
        dstPoints[dstIdx] = x;
        dstPoints[dstIdx + 1] = y;
        dstIdx += ptStep;
    }
}

} // namespace live2d
