#include "ParamDefSet.hpp"
#include "BinaryReader.hpp"

namespace live2d {

void ParamDefSet::read(BinaryReader& br) {
    mParamDefList = br.readObject<std::vector<ParamDefFloat*>>();
}

} // namespace live2d
