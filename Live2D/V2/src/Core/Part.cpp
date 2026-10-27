#include "Part.hpp"
#include "Deformer.hpp"
#include "Drawable.hpp"
#include "BinaryReader.hpp"

namespace Live2D {
namespace V2 {

Part::~Part() = default;

void Part::Read(BinaryReader& br) {
    mLocked = br.ReadBit();
    mVisible = br.ReadBit();
    mId = br.ReadObject<const Id*>();
    auto rawDefs = br.ReadObject<std::vector<Deformer*>>();
    mDeformers.reserve(rawDefs.size());
    for (auto* d : rawDefs)
        mDeformers.emplace_back(d);
    auto rawDraw = br.ReadObject<std::vector<Drawable*>>();
    mDrawables.reserve(rawDraw.size());
    for (auto* d : rawDraw)
        mDrawables.emplace_back(d);
}

}   // namespace V2
}   // namespace Live2D
