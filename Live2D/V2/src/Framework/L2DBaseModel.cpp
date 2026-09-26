#include "L2DBaseModel.hpp"
#include "../Core/BinaryReader.hpp"
#include "../Core/ModelImpl.hpp"
#include "../Draw/MeshContext.hpp"
#include "../Model/ModelContext.hpp"
#include "../Motion/AMotion.hpp"
#include "../Motion/Live2DMotion.hpp"
#include "Graphics/GLRenderer.hpp"
#include "L2DExpressionMotion.hpp"
#include "L2DEyeBlink.hpp"
#include "L2DMotionManager.hpp"
#include "L2DPhysics.hpp"
#include "L2DPose.hpp"

namespace Live2D {
namespace V2 {

L2DBaseModel::L2DBaseModel()
    : mModelImpl(nullptr)
    , mModelContext(nullptr)
    , mEyeBlink(std::make_unique<L2DEyeBlink>())
    , mPhysics(std::make_unique<L2DPhysics>())
    , mPose(std::make_unique<L2DPose>())
    , mMainMotionMgr(std::make_unique<L2DMotionManager>())
    , mExpressionMgr(std::make_unique<L2DMotionManager>())
{}

L2DBaseModel::~L2DBaseModel() = default;

void L2DBaseModel::loadModelData(const std::vector<uint8_t>& data, int version)
{
    BinaryReader br(data);
    br.readByte();
    br.readByte();
    br.readByte();
    version = br.readByte();
    br.setFormatVersion(version);
    auto* impl = static_cast<ModelImpl*>(br.readObjectRaw());
    mModelImpl.reset(impl);
    mModelContext = std::make_unique<ModelContext>();
    mModelContext->init(mModelImpl.get());
    mModelMatrix =
        L2DModelMatrix((float)mModelImpl->getCanvasWidth(), (float)mModelImpl->getCanvasHeight());
    mModelMatrix.setWidth(2);
    mModelMatrix.setCenterPosition(0, 0);
}
AMotion* L2DBaseModel::loadMotion(const std::string& name, const std::vector<uint8_t>& data)
{
    auto* m = Live2DMotion::load(data);
    mMotions[name].emplace_back(m);
    return m;
}
AMotion* L2DBaseModel::loadExpression(const std::string& name, const std::vector<uint8_t>& data)
{
    auto* m = L2DExpressionMotion::load(data);
    mExpressions[name].reset(m);
    return m;
}
L2DPose* L2DBaseModel::loadPose(const std::vector<uint8_t>& data)
{
    mPose.reset(L2DPose::load(data));
    return mPose.get();
}
void L2DBaseModel::loadPhysics(const std::vector<uint8_t>& data)
{
    mPhysics.reset(L2DPhysics::load(data));
}
bool L2DBaseModel::hitTestSimple(const std::string& drawID, float x, float y)
{
    auto* mc = mModelContext.get();
    int drawIdx = mc->getDrawDataIndex(&Id::getID(drawID));
    if (drawIdx < 0)
        return false;

    auto* dctx = mc->getDrawContext(drawIdx);
    if (!dctx || !dctx->mAvailable)
        return false;

    auto& verts =
        !dctx->mTransformedPoints.empty() ? dctx->mTransformedPoints : dctx->mInterpolatedPoints;
    if (verts.empty())
        return false;

    float left = (float)mModelImpl->getCanvasWidth(), right = 0;
    float top = (float)mModelImpl->getCanvasHeight(), bottom = 0;
    for (size_t j = 0; j < verts.size(); j += 2) {
        float vx = verts[j], vy = verts[j + 1];
        if (vx < left)
            left = vx;
        if (vx > right)
            right = vx;
        if (vy < top)
            top = vy;
        if (vy > bottom)
            bottom = vy;
    }

    float tx = mModelMatrix.invertTransformX(x);
    float ty = mModelMatrix.invertTransformY(y);
    return left <= tx && tx <= right && top <= ty && ty <= bottom;
}

}   // namespace V2
}   // namespace Live2D