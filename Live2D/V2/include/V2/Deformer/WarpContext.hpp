#pragma once

#include "DeformerContext.hpp"
#include <vector>

namespace Live2D {
namespace V2 {

class WarpDeformer;

class WarpContext final : public DeformerContext
{
public:
    explicit WarpContext(WarpDeformer* deformer);

    WarpDeformer* mWarpDeformer;
    std::vector<float> mInterpolatedPoints;
    std::vector<float> mTransformedPoints;
};

}   // namespace V2
}   // namespace Live2D