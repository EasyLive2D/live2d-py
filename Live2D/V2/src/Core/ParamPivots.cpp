#include "ParamPivots.hpp"
#include "BinaryReader.hpp"

namespace Live2D {
namespace V2 {

void ParamPivots::read(BinaryReader& br)
{
    mParamId = br.readObject<const Id*>();
    mPivotCount = br.readInt32();
    mPivotValues = br.readObject<std::vector<float>>();
}

int ParamPivots::getParamIndex(int initVersion)
{
    if (mInitVersion != initVersion) {
        mParamIndex = PARAM_INDEX_NOT_INIT;
    }
    return mParamIndex;
}

}   // namespace V2
}   // namespace Live2D