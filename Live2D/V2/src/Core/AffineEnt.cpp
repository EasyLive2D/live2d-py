#include "AffineEnt.hpp"
#include "../Core/BinaryReader.hpp"
#include "../Core/DEF.hpp"

namespace Live2D {
namespace V2 {

void AffineEnt::read(BinaryReader& br)
{
    mOriginX = br.readFloat32();
    mOriginY = br.readFloat32();
    mScaleX = br.readFloat32();
    mScaleY = br.readFloat32();
    mRotationDeg = br.readFloat32();
    if (br.getFormatVersion() >= LIVE2D_FORMAT_VERSION_V2_10_SDK2) {
        mReflectX = br.readBoolean();
        mReflectY = br.readBoolean();
    }
}

}   // namespace V2
}   // namespace Live2D