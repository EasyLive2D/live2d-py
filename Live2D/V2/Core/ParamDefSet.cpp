#include "ParamDefSet.hpp"
#include "ParamDefFloat.hpp"
#include "BinaryReader.hpp"

namespace live2d {

ParamDefSet::~ParamDefSet() = default;

void ParamDefSet::read(BinaryReader& br) {
    auto raw = br.readObject<std::vector<ParamDefFloat*>>();
    mParamDefList.reserve(raw.size());
    for (auto* p : raw) mParamDefList.emplace_back(p);
}

} // namespace live2d
