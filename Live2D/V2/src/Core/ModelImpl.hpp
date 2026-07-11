#pragma once
#include <memory>

#include "ISerializable.hpp"
#include <vector>

#include "ParamDefSet.hpp"
#include "PartsData.hpp"

namespace live2d {

class ModelImpl final : public ISerializable {
public:
    ModelImpl() = default;
    ~ModelImpl() override;

    void read(class BinaryReader& br) override;

    int getCanvasWidth() const { return mCanvasWidth; }
    int getCanvasHeight() const { return mCanvasHeight; }

    ParamDefSet* getParamDefSet() const { return mParamDefSet.get(); }
    std::vector<std::unique_ptr<PartsData>>& getPartsDataList() { return mPartsDataList; }

private:
    std::unique_ptr<ParamDefSet> mParamDefSet;
    std::vector<std::unique_ptr<PartsData>> mPartsDataList;
    int mCanvasWidth = 400;
    int mCanvasHeight = 400;
};

} // namespace live2d
