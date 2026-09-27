#include "Deformer.hpp"
#include "BinaryReader.hpp"
#include "DEF.hpp"
#include "PivotManager.hpp"
#include "ModelContext.hpp"
#include "UtInterpolate.hpp"
#include "DeformerContext.hpp"

namespace Live2D {
namespace V2 {

void Deformer::read(BinaryReader& br) {
    mId = br.readObject<const Id*>();
    mTargetId = br.readObject<const Id*>();
}

void Deformer::readOpacity(BinaryReader& br) {
    if (br.getFormatVersion() >= LIVE2D_FORMAT_VERSION_V2_10_SDK2) {
        mPivotOpacities = br.readFloat32Array();
    }
}

void Deformer::interpolateOpacity(ModelContext* mdc, PivotManager* pivotMgr, DeformerContext* bctx,
                                  bool& ret) {
    if (mPivotOpacities.empty()) {
        bctx->setInterpolatedOpacity(1.0f);
    } else {
        bctx->setInterpolatedOpacity(
            UtInterpolate::interpolateFloat(mdc, pivotMgr, ret, mPivotOpacities));
    }
}

bool Deformer::needTransform() const {
    return mTargetId != nullptr && *mTargetId != Id::DST_BASE_ID();
}

}   // namespace V2
}   // namespace Live2D