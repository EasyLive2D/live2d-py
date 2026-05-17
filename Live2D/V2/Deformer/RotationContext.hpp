#pragma once

#include "DeformerContext.hpp"

namespace live2d {

class RotationDeformer;
class AffineEnt;

class RotationContext final : public DeformerContext {
public:
    explicit RotationContext(RotationDeformer* deformer);

    RotationDeformer* mRotationDeformer;
    AffineEnt* mInterpolatedAffine = nullptr;
    AffineEnt* mTransformedAffine = nullptr;
};

} // namespace live2d
