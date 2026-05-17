#include "Avatar.hpp"
#include "PartsData.hpp"
#include "BinaryReader.hpp"

namespace live2d {

void Avatar::read(BinaryReader& br) {
    mId = br.readObject<const Id*>();
    mDrawDataList = br.readObject<std::vector<IDrawData*>>();
    mDeformerList = br.readObject<std::vector<Deformer*>>();
}

void Avatar::replacePartsData(PartsData* parts) {
    parts->setDeformer(mDeformerList);
    parts->setDrawData(mDrawDataList);
    mDeformerList.clear();
    mDrawDataList.clear();
}

} // namespace live2d
