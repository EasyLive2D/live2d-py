#include "ModelImpl.hpp"
#include "ParamDefSet.hpp"
#include "PartsData.hpp"
#include "BinaryReader.hpp"

namespace live2d {

ModelImpl::~ModelImpl() {
    delete mParamDefSet;
    for (auto* p : mPartsDataList) delete p;
}

void ModelImpl::read(BinaryReader& br) {
    mParamDefSet = br.readObject<ParamDefSet*>();
    mPartsDataList = br.readObject<std::vector<PartsData*>>();
    mCanvasWidth = br.readInt32();
    mCanvasHeight = br.readInt32();
}

} // namespace live2d
