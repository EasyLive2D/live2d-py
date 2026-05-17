#pragma once

#include "ISerializable.hpp"
#include <vector>

namespace live2d {

class ParamDefSet;
class PartsData;

class ModelImpl final : public ISerializable {
public:
    ModelImpl() = default;

    void read(class BinaryReader& br) override;

    int getCanvasWidth() const { return mCanvasWidth; }
    int getCanvasHeight() const { return mCanvasHeight; }

    ParamDefSet* getParamDefSet() const { return mParamDefSet; }
    std::vector<PartsData*>& getPartsDataList() { return mPartsDataList; }

private:
    ParamDefSet* mParamDefSet = nullptr;
    std::vector<PartsData*> mPartsDataList;
    int mCanvasWidth = 400;
    int mCanvasHeight = 400;
};

} // namespace live2d
