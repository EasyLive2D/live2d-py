#include "ModelImpl.hpp"
#include "ParamDefSet.hpp"
#include "PartsData.hpp"
#include "BinaryReader.hpp"

namespace live2d {

ModelImpl::~ModelImpl() = default;

void ModelImpl::read(BinaryReader& br) {
    mParamDefSet.reset(br.readObject<ParamDefSet*>());
    auto rawParts = br.readObject<std::vector<PartsData*>>();
    mPartsDataList.reserve(rawParts.size());
    for (auto* p : rawParts) mPartsDataList.emplace_back(p);
    mCanvasWidth = br.readInt32();
    mCanvasHeight = br.readInt32();
}

} // namespace live2d
