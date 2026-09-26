#pragma once
#include <memory>

#include "ISerializable.hpp"
#include <vector>

namespace Live2D {
namespace V2 {

class ParamDefFloat;

class ParamDefSet final : public ISerializable
{
public:
    ParamDefSet() = default;
    ~ParamDefSet() override;

    void read(class BinaryReader& br) override;

    const std::vector<std::unique_ptr<ParamDefFloat>>& getParamDefFloatList() const
    {
        return mParamDefList;
    }

private:
    std::vector<std::unique_ptr<ParamDefFloat>> mParamDefList;
};

}   // namespace V2
}   // namespace Live2D