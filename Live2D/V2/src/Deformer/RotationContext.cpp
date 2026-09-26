#include "RotationContext.hpp"
#include "AffineEnt.hpp"
#include "RotationDeformer.hpp"

namespace Live2D {
namespace V2 {

RotationContext::RotationContext(RotationDeformer* deformer)
    : DeformerContext(deformer)
    , mRotationDeformer(deformer)
    , mInterpolatedAffine(std::make_unique<AffineEnt>())
{
    if (deformer->needTransform()) {
        mTransformedAffine = std::make_unique<AffineEnt>();
    }
}

RotationContext::~RotationContext() = default;

}   // namespace V2
}   // namespace Live2D