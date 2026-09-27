#pragma once
#include <memory>

#include "DeformerContext.hpp"

namespace Live2D {
namespace V2 {

class RotationDeformer;
class AffineEnt;

class RotationContext final : public DeformerContext
{
public:
    explicit RotationContext(RotationDeformer* deformer);
    ~RotationContext() override;

    RotationDeformer* mRotationDeformer;
    std::unique_ptr<AffineEnt> mInterpolatedAffine;
    std::unique_ptr<AffineEnt> mTransformedAffine;
};

}   // namespace V2
}   // namespace Live2D