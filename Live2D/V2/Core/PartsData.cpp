#include "PartsData.hpp"
#include "PartsDataContext.hpp"
#include "BinaryReader.hpp"
#include "../Draw/IDrawData.hpp"
#include "../Deformer/Deformer.hpp"

namespace live2d {

PartsData::~PartsData() {
    for (auto* d : mDeformerList) delete d;
    for (auto* d : mDrawDataList) delete d;
}

void PartsData::read(BinaryReader& br) {
    mLocked = br.readBit();
    mVisible = br.readBit();
    mId = br.readObject<const Id*>();
    mDeformerList = br.readObject<std::vector<Deformer*>>();
    mDrawDataList = br.readObject<std::vector<IDrawData*>>();
}

PartsDataContext* PartsData::init() {
    auto* ctx = new PartsDataContext(this);
    ctx->setPartsOpacity(isVisible() ? 1.0f : 0.0f);
    return ctx;
}

} // namespace live2d
