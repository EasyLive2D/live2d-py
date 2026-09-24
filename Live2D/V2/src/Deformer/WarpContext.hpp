#pragma once

#include "DeformerContext.hpp"
#include <vector>

namespace live2d {

class WarpDeformer;

class WarpContext final : public DeformerContext {
public:
    explicit WarpContext(WarpDeformer* deformer);

    WarpDeformer* mWarpDeformer;
    std::vector<float> mInterpolatedPoints;
    std::vector<float> mTransformedPoints;
};

} // namespace live2d
