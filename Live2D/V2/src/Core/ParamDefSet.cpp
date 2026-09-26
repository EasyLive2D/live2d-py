#include "ParamDefSet.hpp"
#include "BinaryReader.hpp"
#include "ParamDefFloat.hpp"

namespace Live2D {
namespace V2 {

ParamDefSet::~ParamDefSet() = default;

void ParamDefSet::read(BinaryReader& br)
{
    auto raw = br.readObject<std::vector<ParamDefFloat*>>();
    mParamDefList.reserve(raw.size());
    for (auto* p : raw)
        mParamDefList.emplace_back(p);
}

}   // namespace V2
}   // namespace Live2D