#include "ParamDefFloat.hpp"
#include "BinaryReader.hpp"

namespace Live2D {
namespace V2 {

void ParamDefFloat::read(BinaryReader& br)
{
    mMinValue = br.readFloat32();
    mMaxValue = br.readFloat32();
    mDefaultValue = br.readFloat32();
    mParamId = br.readObject<const Id*>();
}

}   // namespace V2
}   // namespace Live2D