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

int IDrawData::sTotalMinOrder = DEFAULT_ORDER;
int IDrawData::sTotalMaxOrder = DEFAULT_ORDER;

static void setDrawOrders(const std::vector<int>& orders) {
    for (int i = static_cast<int>(orders.size()) - 1; i >= 0; i--) {
        int order = orders[i];
        if (order < IDrawData::sTotalMinOrder) IDrawData::sTotalMinOrder = order;
        else if (order > IDrawData::sTotalMaxOrder) IDrawData::sTotalMaxOrder = order;
    }
}

static std::vector<std::string>* convertClipIDForV2_11(const Id* clipId) {
    if (!clipId || clipId->str().empty()) return nullptr;
    auto* result = new std::vector<std::string>();
    const std::string& s = clipId->str();
    if (s.find(',') == std::string::npos) {
        result->push_back(s);
        return result;
    }
    size_t start = 0, end;
    while ((end = s.find(',', start)) != std::string::npos) {
        result->push_back(s.substr(start, end - start));
        start = end + 1;
    }
    result->push_back(s.substr(start));
    return result;
}

void IDrawData::read(BinaryReader& br) {
    mId = br.readObject<const Id*>();
    mTargetId = br.readObject<const Id*>();
    mPivotMgr = br.readObject<PivotManager*>();
    mAverageDrawOrder = br.readInt32();
    mPivotDrawOrders = br.readInt32Array();
    mPivotOpacities = br.readFloat32Array();

    if (br.getFormatVersion() >= LIVE2D_FORMAT_VERSION_AVAILABLE) {
        const Id* clipId = br.readObject<const Id*>();
        mClipIDList = convertClipIDForV2_11(clipId);
    } else {
        mClipIDList = nullptr;
    }
    setDrawOrders(mPivotDrawOrders);
}

void IDrawData::setupInterpolate(ModelContext* mc, MeshContext* ctx) {
    ctx->mParamOutside = false;
    ctx->mInterpolatedDrawOrder = UtInterpolate::interpolateInt(
        mc, mPivotMgr, ctx->mParamOutside, mPivotDrawOrders);
    // Match Python: skip opacity interpolation if outside param
    if (ctx->mParamOutside) return;
    ctx->mInterpolatedOpacity = UtInterpolate::interpolateFloat(
        mc, mPivotMgr, ctx->mParamOutside, mPivotOpacities);
}

float IDrawData::getOpacity(MeshContext* ctx) {
    return ctx->mInterpolatedOpacity;
}

int IDrawData::getDrawOrder(MeshContext* ctx) {
    return ctx->mInterpolatedDrawOrder;
}

} // namespace live2d
