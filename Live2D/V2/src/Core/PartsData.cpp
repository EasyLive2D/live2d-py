#include "PartsData.hpp"
#include "../Deformer/Deformer.hpp"
#include "../Draw/IDrawData.hpp"
#include "BinaryReader.hpp"
#include "PartsDataContext.hpp"

namespace Live2D {
namespace V2 {

PartsData::~PartsData() = default;

void PartsData::read(BinaryReader& br)
{
    mLocked = br.readBit();
    mVisible = br.readBit();
    mId = br.readObject<const Id*>();
    auto rawDefs = br.readObject<std::vector<Deformer*>>();
    mDeformerList.reserve(rawDefs.size());
    for (auto* d : rawDefs)
        mDeformerList.emplace_back(d);
    auto rawDraw = br.readObject<std::vector<IDrawData*>>();
    mDrawDataList.reserve(rawDraw.size());
    for (auto* d : rawDraw)
        mDrawDataList.emplace_back(d);
}

PartsDataContext* PartsData::init()
{
    auto* ctx = new PartsDataContext(this);
    ctx->setPartsOpacity(isVisible() ? 1.0f : 0.0f);
    return ctx;
}

}   // namespace V2
}   // namespace Live2D