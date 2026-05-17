#pragma once

#include "ISerializable.hpp"
#include "Id.hpp"

namespace live2d {

class ParamDefFloat final : public ISerializable {
public:
    ParamDefFloat() = default;

    void read(class BinaryReader& br) override;

    float getMinValue() const { return mMinValue; }
    float getMaxValue() const { return mMaxValue; }
    float getDefaultValue() const { return mDefaultValue; }
    const Id* getParamID() const { return mParamId; }

private:
    float mMinValue = 0.0f;
    float mMaxValue = 0.0f;
    float mDefaultValue = 0.0f;
    const Id* mParamId = nullptr;
};

} // namespace live2d
