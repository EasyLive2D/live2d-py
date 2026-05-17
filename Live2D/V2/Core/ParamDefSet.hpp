#pragma once

#include "ISerializable.hpp"
#include <vector>

namespace live2d {

class ParamDefFloat;

class ParamDefSet final : public ISerializable {
public:
    ParamDefSet() = default;

    void read(class BinaryReader& br) override;

    const std::vector<ParamDefFloat*>& getParamDefFloatList() const { return mParamDefList; }

private:
    std::vector<ParamDefFloat*> mParamDefList;
};

} // namespace live2d
