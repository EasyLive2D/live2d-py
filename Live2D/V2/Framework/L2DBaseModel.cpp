#include "L2DBaseModel.hpp"
#include "../Model/Live2DModelOpenGL.hpp"
#include "../Motion/AMotion.hpp"
#include "../Motion/Live2DMotion.hpp"
#include "L2DEyeBlink.hpp"
#include "L2DPose.hpp"
#include "L2DPhysics.hpp"
#include "L2DExpressionMotion.hpp"
#include "L2DMotionManager.hpp"
#include "../Model/ModelContext.hpp"
#include "../Core/BinaryReader.hpp"
#include "../Core/ModelImpl.hpp"
#include "../Draw/MeshContext.hpp"
#include "Log.hpp"
namespace live2d {
L2DBaseModel::L2DBaseModel() {
    mLive2DModel = new Live2DModelOpenGL();
    mEyeBlink = new L2DEyeBlink();
    mPhysics = new L2DPhysics();
    mPose = new L2DPose();
    mMainMotionMgr = new L2DMotionManager();
    mExpressionMgr = new L2DMotionManager();
}
L2DBaseModel::~L2DBaseModel() {
    delete mLive2DModel;
    delete mEyeBlink; delete mPhysics; delete mPose;
    delete mMainMotionMgr; delete mExpressionMgr;
    for (auto& p : mMotions) for (auto* m : p.second) delete m;
    for (auto& p : mExpressions) delete p.second;
}
void L2DBaseModel::loadModelData(const std::vector<uint8_t>& data, int version) {
    BinaryReader br(data);
    br.readByte(); br.readByte(); br.readByte();
    version = br.readByte();
    br.setFormatVersion(version);
    auto* impl = static_cast<ModelImpl*>(br.readObjectRaw());
    mLive2DModel->setModelImpl(impl);
    mLive2DModel->getModelContext()->setDrawParam(mLive2DModel->getDrawParam());
    mLive2DModel->getModelContext()->init();
    // Initialize model matrix (match Python L2DBaseModel.loadModelData)
    mModelMatrix = L2DModelMatrix(
        (float)mLive2DModel->getCanvasWidth(),
        (float)mLive2DModel->getCanvasHeight());
    mModelMatrix.setWidth(2);
    mModelMatrix.setCenterPosition(0, 0);
}
void L2DBaseModel::loadTexture(int no, const std::string& path) {
    (void)no; (void)path; // stub
}
AMotion* L2DBaseModel::loadMotion(const std::string& name, const std::vector<uint8_t>& data) {
    auto* m = Live2DMotion::load(data);
    mMotions[name].push_back(m); return m;
}
AMotion* L2DBaseModel::loadExpression(const std::string& name, const std::vector<uint8_t>& data) {
    auto* m = L2DExpressionMotion::load(data);
    mExpressions[name] = m; return m;
}
L2DPose* L2DBaseModel::loadPose(const std::vector<uint8_t>& data) {
    delete mPose; mPose = L2DPose::load(data); return mPose;
}
void L2DBaseModel::loadPhysics(const std::vector<uint8_t>& data) {
    mPhysics = L2DPhysics::load(data);
}
bool L2DBaseModel::hitTestSimple(const std::string& drawID, float x, float y) {
    auto* mc = mLive2DModel->getModelContext();
    int drawIdx = mc->getDrawDataIndex(&Id::getID(drawID));
    if (drawIdx < 0) return false;

    auto* dctx = mc->getDrawContext(drawIdx);
    if (!dctx || !dctx->mAvailable) return false;

    auto& verts = !dctx->mTransformedPoints.empty()
                      ? dctx->mTransformedPoints
                      : dctx->mInterpolatedPoints;
    if (verts.empty()) return false;

    float left = (float)mLive2DModel->getCanvasWidth(), right = 0;
    float top = (float)mLive2DModel->getCanvasHeight(), bottom = 0;
    for (size_t j = 0; j < verts.size(); j += 2) {
        float vx = verts[j], vy = verts[j + 1];
        if (vx < left) left = vx;
        if (vx > right) right = vx;
        if (vy < top) top = vy;
        if (vy > bottom) bottom = vy;
    }

    float tx = mModelMatrix.invertTransformX(x);
    float ty = mModelMatrix.invertTransformY(y);
    return left <= tx && tx <= right && top <= ty && ty <= bottom;
}
} // namespace live2d
