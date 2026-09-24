#include "ParamDefFloat.hpp"
#include "BinaryReader.hpp"

namespace live2d {

void ParamDefFloat::read(BinaryReader& br) {
    mMinValue = br.readFloat32();
    mMaxValue = br.readFloat32();
    mDefaultValue = br.readFloat32();
    mParamId = br.readObject<const Id*>();
}

} // namespace live2d
