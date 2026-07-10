#pragma once
#include <memory>

#include "DeformerContext.hpp"

namespace live2d {

class RotationDeformer;
class AffineEnt;

class RotationContext final : public DeformerContext {
public:
    explicit RotationContext(RotationDeformer* deformer);
    ~RotationContext() override;

    RotationDeformer* mRotationDeformer;
    std::unique_ptr<AffineEnt> mInterpolatedAffine;
    std::unique_ptr<AffineEnt> mTransformedAffine;
};

} // namespace live2d
