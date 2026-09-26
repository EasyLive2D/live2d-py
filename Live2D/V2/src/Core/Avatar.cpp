#include "Avatar.hpp"
#include "BinaryReader.hpp"
#include "PartsData.hpp"

namespace Live2D {
namespace V2 {

Avatar::~Avatar() = default;

void Avatar::read(BinaryReader& br)
{
    mId = br.readObject<const Id*>();
    auto rawDraw = br.readObject<std::vector<IDrawData*>>();
    mDrawDataList.reserve(rawDraw.size());
    for (auto* d : rawDraw)
        mDrawDataList.emplace_back(d);
    auto rawDefs = br.readObject<std::vector<Deformer*>>();
    mDeformerList.reserve(rawDefs.size());
    for (auto* d : rawDefs)
        mDeformerList.emplace_back(d);
}

void Avatar::replacePartsData(PartsData* parts)
{
    parts->setDeformer(std::move(mDeformerList));
    parts->setDrawData(std::move(mDrawDataList));
}

}   // namespace V2
}   // namespace Live2D
