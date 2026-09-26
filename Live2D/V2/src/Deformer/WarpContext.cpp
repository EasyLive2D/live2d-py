#include "WarpContext.hpp"
#include "WarpDeformer.hpp"

namespace Live2D {
namespace V2 {

WarpContext::WarpContext(WarpDeformer* deformer)
    : DeformerContext(deformer)
    , mWarpDeformer(deformer)
{
    int pointCount = deformer->getPointCount();
    mInterpolatedPoints.resize(pointCount * 2);

    if (deformer->needTransform()) {
        mTransformedPoints.resize(pointCount * 2);
    }
}

}   // namespace V2
}   // namespace Live2D