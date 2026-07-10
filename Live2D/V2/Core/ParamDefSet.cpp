#include "ParamDefSet.hpp"
#include "ParamDefFloat.hpp"
#include "BinaryReader.hpp"

namespace live2d {

ParamDefSet::~ParamDefSet() {
    for (auto* p : mParamDefList) delete p;
}

void ParamDefSet::read(BinaryReader& br) {
    mParamDefList = br.readObject<std::vector<ParamDefFloat*>>();
}

} // namespace live2d
