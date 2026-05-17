#include "RotationContext.hpp"
#include "RotationDeformer.hpp"
#include "AffineEnt.hpp"

namespace live2d {

RotationContext::RotationContext(RotationDeformer* deformer)
    : DeformerContext(deformer)
    , mRotationDeformer(deformer)
    , mInterpolatedAffine(new AffineEnt()) {
    if (deformer->needTransform()) {
        mTransformedAffine = new AffineEnt();
    }
}

} // namespace live2d
