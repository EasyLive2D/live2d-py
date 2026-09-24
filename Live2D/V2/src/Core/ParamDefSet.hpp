#pragma once
#include <memory>

#include "ISerializable.hpp"
#include <vector>

namespace live2d {

class ParamDefFloat;

class ParamDefSet final : public ISerializable {
public:
    ParamDefSet() = default;
    ~ParamDefSet() override;

    void read(class BinaryReader& br) override;

    const std::vector<std::unique_ptr<ParamDefFloat>>& getParamDefFloatList() const { return mParamDefList; }

private:
    std::vector<std::unique_ptr<ParamDefFloat>> mParamDefList;
};

} // namespace live2d
