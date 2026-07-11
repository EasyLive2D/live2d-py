#include "IDrawData.hpp"
#include "MeshContext.hpp"
#include "../Core/BinaryReader.hpp"
#include "../Core/DEF.hpp"
#include "../Core/PivotManager.hpp"
#include "../Deformer/Deformer.hpp"
#include "../Model/ModelContext.hpp"
#include "../Util/UtInterpolate.hpp"
#include <string>

namespace live2d {

IDrawData::~IDrawData() = default;

void IDrawData::read(BinaryReader& br) {
    mId = br.readObject<const Id*>();
    mTargetId = br.readObject<const Id*>();
    mPivotMgr.reset(br.readObject<PivotManager*>());
    mAverageDrawOrder = br.readInt32();
    mPivotDrawOrders = br.readInt32Array();
    mPivotOpacities = br.readFloat32Array();

    if (br.getFormatVersion() >= LIVE2D_FORMAT_VERSION_AVAILABLE) {
        const Id* clipId = br.readObject<const Id*>();
        if (clipId && !clipId->str().empty()) {
            const std::string& s = clipId->str();
            size_t start = 0, end;
            while ((end = s.find(',', start)) != std::string::npos) {
                mClipIDList.push_back(s.substr(start, end - start));
                start = end + 1;
            }
            mClipIDList.push_back(s.substr(start));
        }
    }
}

void IDrawData::setupInterpolate(ModelContext* mc, MeshContext* ctx) {
    ctx->mParamOutside = false;
    ctx->mInterpolatedDrawOrder = UtInterpolate::interpolateInt(
        mc, mPivotMgr.get(), ctx->mParamOutside, mPivotDrawOrders);
    // Match Python: skip opacity interpolation if outside param
    if (ctx->mParamOutside) return;
    ctx->mInterpolatedOpacity = UtInterpolate::interpolateFloat(
        mc, mPivotMgr.get(), ctx->mParamOutside, mPivotOpacities);
}

float IDrawData::getOpacity(MeshContext* ctx) {
    return ctx->mInterpolatedOpacity;
}

int IDrawData::getDrawOrder(MeshContext* ctx) {
    return ctx->mInterpolatedDrawOrder;
}

} // namespace live2d
