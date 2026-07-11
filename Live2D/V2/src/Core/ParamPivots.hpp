#pragma once

#include "ISerializable.hpp"
#include "Id.hpp"
#include <vector>

namespace live2d {

class ParamPivots final : public ISerializable {
public:
    static constexpr int PARAM_INDEX_NOT_INIT = -2;

    ParamPivots() = default;

    void read(class BinaryReader& br) override;

    int getParamIndex(int initVersion);
    void setParamIndex(int index, int initVersion) {
        mParamIndex = index;
        mInitVersion = initVersion;
    }

    const Id* getParamID() const { return mParamId; }
    int getPivotCount() const { return mPivotCount; }
    const std::vector<float>& getPivotValues() const { return mPivotValues; }

    int getTmpPivotIndex() const { return mTmpPivotIndex; }
    void setTmpPivotIndex(int index) { mTmpPivotIndex = index; }
    float getTmpT() const { return mTmpT; }
    void setTmpT(float value) { mTmpT = value; }

private:
    int mPivotCount = 0;
    const Id* mParamId = nullptr;
    std::vector<float> mPivotValues;
    int mParamIndex = PARAM_INDEX_NOT_INIT;
    int mInitVersion = -1;
    int mTmpPivotIndex = 0;
    float mTmpT = 0.0f;
};

} // namespace live2d
