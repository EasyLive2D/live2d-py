#include "RotationContext.hpp"
#include "RotationDeformer.hpp"
#include "AffineEnt.hpp"

namespace live2d {

RotationContext::RotationContext(RotationDeformer* deformer)
    : DeformerContext(deformer)
    , mRotationDeformer(deformer)
    , mInterpolatedAffine(std::make_unique<AffineEnt>()) {
    if (deformer->needTransform()) {
        mTransformedAffine = std::make_unique<AffineEnt>();
    }
}

RotationContext::~RotationContext() = default;

} // namespace live2d
