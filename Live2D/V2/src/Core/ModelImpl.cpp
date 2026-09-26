#include "ModelImpl.hpp"
#include "BinaryReader.hpp"
#include "ParamDefSet.hpp"
#include "PartsData.hpp"

namespace Live2D {
namespace V2 {

ModelImpl::~ModelImpl() = default;

void ModelImpl::read(BinaryReader& br)
{
    mParamDefSet.reset(br.readObject<ParamDefSet*>());
    auto rawParts = br.readObject<std::vector<PartsData*>>();
    mPartsDataList.reserve(rawParts.size());
    for (auto* p : rawParts)
        mPartsDataList.emplace_back(p);
    mCanvasWidth = br.readInt32();
    mCanvasHeight = br.readInt32();
}

}   // namespace V2
}   // namespace Live2D