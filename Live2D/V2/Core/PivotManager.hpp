#pragma once

#include "ISerializable.hpp"
#include <vector>

namespace live2d {

class ParamPivots;
class ModelContext;

class PivotManager final : public ISerializable {
public:
    PivotManager() = default;

    void read(class BinaryReader& br) override;

    bool checkParamUpdated(ModelContext* modelContext);
    int calcPivotValues(ModelContext* modelContext, bool& outRet);
    void calcPivotIndices(std::vector<int16_t>& indexArray, std::vector<float>& tArray, int interpolationCount);

    int getParamCount() const { return static_cast<int>(mParamPivotTable.size()); }

private:
    std::vector<ParamPivots*> mParamPivotTable;
};

} // namespace live2d
