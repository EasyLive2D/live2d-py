#include "WarpContext.hpp"
#include "WarpDeformer.hpp"

namespace live2d {

WarpContext::WarpContext(WarpDeformer* deformer)
    : DeformerContext(deformer)
    , mWarpDeformer(deformer) {
    int pointCount = deformer->getPointCount();
    mInterpolatedPoints.resize(pointCount * 2);

    if (deformer->needTransform()) {
        mTransformedPoints.resize(pointCount * 2);
    }
}

} // namespace live2d
