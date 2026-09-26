#pragma once

#include "ISerializable.hpp"
#include "Id.hpp"

namespace Live2D {
namespace V2 {

class ParamDefFloat final : public ISerializable
{
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

}   // namespace V2
}   // namespace Live2D