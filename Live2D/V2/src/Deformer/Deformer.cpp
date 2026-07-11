#include "Deformer.hpp"
#include "DeformerContext.hpp"
#include "../Core/BinaryReader.hpp"
#include "../Core/DEF.hpp"
#include "../Core/PivotManager.hpp"
#include "../Model/ModelContext.hpp"
#include "../Util/UtInterpolate.hpp"

namespace live2d {

void Deformer::read(BinaryReader& br) {
    mId = br.readObject<const Id*>();
    mTargetId = br.readObject<const Id*>();
}

void Deformer::readOpacity(BinaryReader& br) {
    if (br.getFormatVersion() >= LIVE2D_FORMAT_VERSION_V2_10_SDK2) {
        mPivotOpacities = br.readFloat32Array();
    }
}

void Deformer::interpolateOpacity(ModelContext* mdc, PivotManager* pivotMgr,
                                  DeformerContext* bctx, bool& ret) {
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

} // namespace live2d
