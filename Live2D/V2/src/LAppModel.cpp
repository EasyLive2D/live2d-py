#include "LAppModel.hpp"
#include "Core/Id.hpp"
#include "Core/PartsData.hpp"
#include "Core/PartsDataContext.hpp"
#include "Draw/Mesh.hpp"
#include "Draw/MeshContext.hpp"
#include "Framework/L2DExpressionMotion.hpp"
#include "Framework/L2DEyeBlink.hpp"
#include "Framework/L2DMotionManager.hpp"
#include "Framework/L2DPhysics.hpp"
#include "Framework/L2DPose.hpp"
#include "Graphics/GLRenderer.hpp"
#include "Log.hpp"
#include "Model/ModelContext.hpp"
#include "Motion/Live2DMotion.hpp"
#include "Util/UtSystem.hpp"
#include "nlohmann/json.hpp"
#include <GL/glew.h>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stb_image.h>
#include <string>
#include <vector>

namespace live2d {

using json = nlohmann::json;

// Helper: read entire file using std::filesystem::u8path for Unicode path support
static std::vector<uint8_t> readFile(const std::string& path)
{
    std::filesystem::path fp = std::filesystem::u8path(path);
    std::ifstream f(fp, std::ios::binary | std::ios::ate);
    if (!f) return {};   // missing file: tellg() would be -1 -> vector((size_t)-1) throws
    auto sz = f.tellg();
    f.seekg(0);
    std::vector<uint8_t> data((size_t)sz);
    f.read((char*)data.data(), sz);
    f.close();
    return data;
}

// Simple JSON texture path extractor
static void parseTexturePaths(const json& data, std::vector<std::string>& texPaths)
{
    auto textures = data.find("textures");
    if (textures != data.end()) {
        texPaths = textures->get<std::vector<std::string>>();
    }
}

LAppModel::LAppModel()
    : mRenderer(nullptr)
{}
LAppModel::~LAppModel() = default;

void LAppModel::loadModelJson(const std::string& path, bool createRenderer)
{
    // Read JSON (use filesystem for Unicode path support)
    std::filesystem::path fp = std::filesystem::u8path(path);
    std::ifstream f(fp);
    auto data = json::parse(f);
    f.close();

    // Get base directory and load .moc from JSON model field
    mModelHomeDir = path.substr(0, path.find_last_of("/\\") + 1);
    Info("Model home directory: %s", mModelHomeDir.c_str());
    std::string mocPath = mModelHomeDir + data["model"].get<std::string>();

    auto mocData = readFile(mocPath);
    if (mocData.empty())
    {
        Error("Failed to read .moc file: %s", mocPath.c_str());
        std::abort();
    }
    loadModelData(mocData, 0);
    mModelMatrix.mWidth = (float)mModelImpl->getCanvasWidth();
    mModelMatrix.mHeight = (float)mModelImpl->getCanvasHeight();
    Info("Load model: %s", mocPath.c_str());

    // Load texture paths from JSON
    parseTexturePaths(data, mTexturePaths);

    // Load physics (Python: getPhysicsFile() is not None — lapp_model.py:63-64)
    auto physics = data.find("physics");
    if (physics != data.end() && physics->is_string()) {
        auto phyFile = physics->get<std::string>();
        auto phyData = readFile(mModelHomeDir + phyFile);
        if (!phyData.empty()) {
            Info("Load physics: %s", phyFile.c_str());
            loadPhysics(phyData);
        }
    }

    // Load pose file (Python: getPoseFile() is None for both missing key and
    // explicit null — e.g. Resources/v2/托尔/model0.json has "pose": null)
    auto pose = data.find("pose");
    if (pose != data.end() && pose->is_string()) {
        auto poseFile = pose->get<std::string>();
        auto posePath = mModelHomeDir + poseFile;
        auto poseData = readFile(posePath);
        if (!poseData.empty()) {
            Info("Load pose: %s", poseFile.c_str());
            mPose.reset(L2DPose::load(poseData));
            // Initialize part/param indices once (PartData::initIndex uses the
            // "VISIBLE:" prefix; a bare-id pre-pass here used to append phantom
            // parameters via getParamIndex's extend-on-miss).
            mPose->initParam(mModelContext.get());
        }
    }

    // Load motion files
    auto motions = data.find("motions");
    if (motions != data.end()) {
        for (auto& [groupName, motionArray] : motions->items()) {
            auto& motVec = mMotions[groupName];
            for (auto& motionEntry : motionArray) {
                auto motFile = motionEntry["file"].get<std::string>();
                auto motPath = mModelHomeDir + motFile;
                auto motData = readFile(motPath);
                if (!motData.empty()) {
                    auto* motion = Live2DMotion::load(motData);
                    Info("Load motion: %s", motFile.c_str());
                    motVec.emplace_back(motion);
                }
            }
            if (motVec.empty())
                mMotions.erase(groupName);
        }
    }

    // Load expression files
    auto expressions = data.find("expressions");
    if (expressions != data.end()) {
        for (auto& [index, expEntry] : expressions->items()) {
            auto expFile = expEntry["file"].get<std::string>();
            auto expPath = mModelHomeDir + expFile;
            auto expData = readFile(expPath);
            auto expName = expEntry["name"].get<std::string>();
            if (!expData.empty()) {
                auto* expr = L2DExpressionMotion::load(expData);
                Info("Load expression: %s", expName.c_str());
                mExpressions[expName].reset(expr);
            }
        }
    }

    if (createRenderer) {
        CreateRenderer();
    }
}
void LAppModel::resize(int w, int h)
{
    mMatrixManager.onResize(w, h);
}
void LAppModel::drag(float x, float y)
{
    // Convert screen coords to scene coords (match Python MatrixManager.screenToScene)
    float w = (float)mMatrixManager.getWidth();
    float h = (float)mMatrixManager.getHeight();
    float sx = (x - w * 0.5f) * 2.0f / h;
    float sy = (y - h * 0.5f) * -2.0f / h;
    mDragMgr.set(sx, sy);
}
bool LAppModel::isMotionFinished() const
{
    return mMainMotionMgr->isFinished();
}
void LAppModel::setOffset(float dx, float dy)
{
    mMatrixManager.setOffset(dx, dy);
}
void LAppModel::setScale(float s)
{
    mMatrixManager.setScale(s);
}
void LAppModel::setParameterValue(const std::string& id, float val, float w)
{
    int idx = mModelContext->getParamIndex(&Id::getID(id));
    mModelContext->setParamFloat(idx, val, w);
    // Match Python: also update savedParamValues so loadParam() doesn't revert
    if (idx >= 0 && idx < (int)mModelContext->mSavedParamValues.size())
        mModelContext->mSavedParamValues[idx] =
            mModelContext->mSavedParamValues[idx] * (1.0f - w) + val * w;
}
void LAppModel::addParameterValue(const std::string& id, float val, float w)
{
    int idx = mModelContext->getParamIndex(&Id::getID(id));
    mModelContext->setParamFloat(idx, mModelContext->getParamFloat(idx) + val * w);
}
int LAppModel::getParameterCount() const
{
    return (int)mModelContext->mParamValues.size();
}
int LAppModel::getPartCount() const
{
    return (int)mModelContext->mPartsDataList.size();
}
std::string LAppModel::getPartId(int index) const
{
    auto& parts = mModelContext->mPartsDataList;
    if (index >= 0 && index < (int)parts.size() && parts[index]->getId())
        return parts[index]->getId()->str();
    return "";
}
void LAppModel::setPartOpacity(int index, float val)
{
    mModelContext->setPartsOpacity(index, val);
}
void LAppModel::update()
{
    mDragMgr.update(0.016f);
    setDrag(mDragMgr.getX(), mDragMgr.getY());

    // Match v2 Python update() flow
    bool updated = false;
    if (mClearFlag) {
        mMainMotionMgr->stopAllMotions();
        if (mPose) {
            for (auto& g : mPose->mMGroups)
                for (auto& p : g.parts) p.initIndex(mModelContext.get());
        }
        mClearFlag = false;
    } else {
        mModelContext->loadParam();
        updated = mMainMotionMgr->updateParam(mModelContext.get());
    }
    mModelContext->saveParam();

    // Check motion finish callback
    // Save and clear BEFORE calling, so re-entrant StartMotion
    // (called from within the callback) can set new callbacks safely.
    if (mCallbacksPending && mMainMotionMgr->isFinished()) {
        auto onFinish = std::move(mOnFinishMotion);
        auto onStart = std::move(mOnStartMotion);
        auto group = mCurrentGroup;
        auto no = mCurrentMotionNo;
        mOnFinishMotion = nullptr;
        mOnStartMotion = nullptr;
        mCallbacksPending = false;

        if (onFinish)
            onFinish(group, no);
    }

    // Python suppresses eye-blink while a main motion is active
    if (!updated && mAutoBlink && mEyeBlink)
        mEyeBlink->updateParam(mModelContext.get());

    // Python skips expression update when no expressions exist
    if (!mExpressions.empty())
        mExpressionMgr->updateParam(mModelContext.get());

    // Drag-based parameter updates (match v2 Python)
    auto addParam = [&](const char* id, float value, float weight) {
        int idx = mModelContext->getParamIndex(&Id::getID(id));
        if (idx >= 0) {
            float cur = mModelContext->getParamFloat(idx);
            mModelContext->setParamFloat(idx, cur + value * weight);
        }
    };
    addParam("PARAM_ANGLE_X", mDragX * 30, 1);
    addParam("PARAM_ANGLE_Y", mDragY * 30, 1);
    addParam("PARAM_ANGLE_Z", mDragX * mDragY * -30, 1);
    addParam("PARAM_BODY_ANGLE_X", mDragX * 10, 1);
    addParam("PARAM_EYE_BALL_X", mDragX, 1);
    addParam("PARAM_EYE_BALL_Y", mDragY, 1);

    // Auto-breath animation (wall clock time, match v2 Python periods)
    if (mAutoBreath) {
        float t = (float)UtSystem::getUserTimeMSec() / 1000.0f;
        addParam("PARAM_ANGLE_X", 15.0f * sinf(t / 6.5345f), 0.5f);
        addParam("PARAM_ANGLE_Y", 8.0f * sinf(t / 3.5345f), 0.5f);
        addParam("PARAM_ANGLE_Z", 10.0f * sinf(t / 5.5345f), 0.5f);
        addParam("PARAM_BODY_ANGLE_X", 4.0f * sinf(t / 15.5345f), 0.5f);
        int breathIdx = mModelContext->getParamIndex(&Id::getID("PARAM_BREATH"));
        if (breathIdx >= 0)
            mModelContext->setParamFloat(breathIdx, 0.5f + 0.5f * sinf(t / 3.2345f));
    }

    if (mPhysics)
        mPhysics->updateParam(mModelContext.get());
    if (mPose)
        mPose->updateParam(mModelContext.get());
}
void LAppModel::draw()
{
    // Match v2 Python: process deformer chain in draw(), not update()
    // This allows SetParameterValue between Update() and Draw() to take effect
    mModelContext->update();

    auto mvp = mMatrixManager.getMvp(&mModelMatrix);

    if (!mRenderer) 
    {
        Error("Renderer not initialized");
        std::abort();
        return;
    }
    mRenderer->setMatrix(mvp.data());
    mRenderer->preDraw(mModelContext.get());
    mRenderer->draw(mModelContext.get());
}
bool LAppModel::hitTest(const std::string& area, float x, float y)
{
    // Convert screen pixels → scene coords (match Python screenToScene)
    float w = (float)mMatrixManager.getWidth();
    float h = (float)mMatrixManager.getHeight();
    float sx = (x - w * 0.5f) * 2.0f / h;
    float sy = (y - h * 0.5f) * -2.0f / h;
    return hitTestSimple(area, sx, sy);
}
void LAppModel::setExpression(const std::string& name)
{
    auto it = mExpressions.find(name);
    if (it != mExpressions.end()) {
        Info("Start expression: %s", name.c_str());
        mExpressionMgr->startMotion(it->second.get(), false);
    }
}
void LAppModel::setRandomExpression()
{
    if (!mExpressions.empty()) {
        auto it = mExpressions.begin();
        std::advance(it, rand() % mExpressions.size());
        Info("Start random expression: %s", it->first.c_str());
        mExpressionMgr->startMotion(it->second.get(), false);
    }
}
void LAppModel::startMotion(const std::string& group, int no, int priority, StartCallback onStart,
                            FinishCallback onFinish)
{
    mOnStartMotion = std::move(onStart);
    mOnFinishMotion = std::move(onFinish);
    auto it = mMotions.find(group);
    if (it != mMotions.end() && !it->second.empty()) {
        if (no < 0 || no >= (int)it->second.size())
            no = 0;

        // Priority check (match Python v2)
        if (priority == 3 /* FORCE */) {
            Info("Start motion (force): group=%s no=%d priority=%d", group.c_str(), no, priority);
            mMainMotionMgr->setReservePriority(priority);
        } else if (!mMainMotionMgr->reserveMotion(priority)) {
            // Lower priority than current motion — don't play
            Info("Start motion rejected (low priority): group=%s no=%d priority=%d current=%d",
                 group.c_str(),
                 no,
                 priority,
                 mMainMotionMgr->mCurrentPriority);
            if (mOnStartMotion)
                mOnStartMotion(group, no);
            if (mOnFinishMotion)
                mOnFinishMotion(group, no);
            mOnStartMotion = nullptr;
            mOnFinishMotion = nullptr;
            return;
        }

        mCallbacksPending = true;
        mCurrentGroup = group;
        mCurrentMotionNo = no;
        if (mOnStartMotion)
            mOnStartMotion(group, no);
        Info("Start motion: group=%s no=%d priority=%d", group.c_str(), no, priority);
        mMainMotionMgr->startMotionPrio(it->second[no].get(), priority);
    } else {
        Info("Start motion: group=%s not found or empty", group.c_str());
        if (mOnStartMotion)
            mOnStartMotion(group, no);
        if (mOnFinishMotion)
            mOnFinishMotion(group, no);
        mOnStartMotion = nullptr;
        mOnFinishMotion = nullptr;
    }
}
void LAppModel::startRandomMotion(const std::string& group, int priority, StartCallback onStart,
                                  FinishCallback onFinish)
{
    if (group.empty()) {
        if (mMotions.empty())
            return;
        int idx = rand() % (int)mMotions.size();
        auto it = mMotions.begin();
        std::advance(it, idx);
        int count = (int)it->second.size();
        int no = (count > 1) ? (rand() % count) : 0;
        startMotion(it->first, no, priority, std::move(onStart), std::move(onFinish));
    } else {
        auto it = mMotions.find(group);
        int count = (it != mMotions.end()) ? (int)it->second.size() : 1;
        int no = (count > 1) ? (rand() % count) : 0;
        startMotion(group, no, priority, std::move(onStart), std::move(onFinish));
    }
}
void LAppModel::clearMotions()
{
    mClearFlag = true;
}
void LAppModel::resetExpression()
{
    mExpressionMgr->stopAllMotions();
}
void LAppModel::resetPose()
{
    if (mPose) {
        for (auto& g : mPose->mMGroups)
            for (auto& p : g.parts) p.initIndex(mModelContext.get());
    }
}
void LAppModel::rotate(float deg)
{
    mMatrixManager.rotate(deg);
}
float LAppModel::getParameterValue(int index) const
{
    return mModelContext->getParamFloat(index);
}
float LAppModel::getParameterMin(int index) const
{
    return mModelContext->getParamMin(index);
}
float LAppModel::getParameterMax(int index) const
{
    return mModelContext->getParamMax(index);
}
float LAppModel::getParameterDefault(int index) const
{
    return mModelContext->getParamDefault(index);
}
std::string LAppModel::getParameterId(int index) const
{
    auto& ids = mModelContext->mParamIdList;
    if (index >= 0 && index < (int)ids.size() && ids[index])
        return ids[index]->str();
    return "";
}
void LAppModel::setPartScreenColor(int index, float r, float g, float b, float a)
{
    mModelContext->setPartScreenColor(index, r, g, b, a);
}
void LAppModel::setPartMultiplyColor(int index, float r, float g, float b, float a)
{
    mModelContext->setPartMultiplyColor(index, r, g, b, a);
}
std::vector<float> LAppModel::getPartScreenColor(int index) const
{
    auto* ctx = mModelContext->getPartsContext(index);
    return {ctx->mScreenColor[0], ctx->mScreenColor[1], ctx->mScreenColor[2], ctx->mScreenColor[3]};
}
std::vector<float> LAppModel::getPartMultiplyColor(int index) const
{
    auto* ctx = mModelContext->getPartsContext(index);
    return {ctx->mMultiplyColor[0],
            ctx->mMultiplyColor[1],
            ctx->mMultiplyColor[2],
            ctx->mMultiplyColor[3]};
}
static bool isInTriangle(float px, float py, float ax, float ay, float bx, float by, float cx,
                         float cy)
{
    float v0x = cx - ax, v0y = cy - ay;
    float v1x = bx - ax, v1y = by - ay;
    float v2x = px - ax, v2y = py - ay;
    float dot00 = v0x * v0x + v0y * v0y;
    float dot01 = v0x * v1x + v0y * v1y;
    float dot02 = v0x * v2x + v0y * v2y;
    float dot11 = v1x * v1x + v1y * v1y;
    float dot12 = v1x * v2x + v1y * v2y;
    float inv = dot00 * dot11 - dot01 * dot01;
    if (inv == 0.0f)
        return false;
    float u = (dot11 * dot02 - dot01 * dot12) / inv;
    float v = (dot00 * dot12 - dot01 * dot02) / inv;
    return (u >= 0) && (v >= 0) && (u + v <= 1);
}

std::vector<std::string> LAppModel::hitPart(float x, float y, bool topOnly)
{
    // Step 1: screen pixels → scene coords (match Python MatrixManager.screenToScene)
    float w = (float)mMatrixManager.getWidth();
    float h = (float)mMatrixManager.getHeight();
    float sx = (x - w * 0.5f) * 2.0f / h;
    float sy = (y - h * 0.5f) * -2.0f / h;

    // Step 2: scene → canvas via MVP inverse (match Python MatrixManager.invertTransform)
    auto mvp = mMatrixManager.getMvp(&mModelMatrix);
    float mx = (sx - mvp[12]) / mvp[0];
    float my = (sy - mvp[13]) / mvp[5];

    auto& drawCtxs = mModelContext->mDrawContextList;
    auto& nextList = mModelContext->mNextListDrawIndex;
    auto& firstList = mModelContext->mOrderListFirstDrawIndex;
    int range = (int)firstList.size();

    std::vector<std::string> result;

    // Iterate draw orders in reverse
    for (int order = range - 1; order >= 0; order--) {
        if ((int)firstList.size() <= order)
            continue;
        int idx = firstList[order];
        if (idx < 0 || idx >= (int)drawCtxs.size())
            continue;
        while (true) {
            auto* dctx = drawCtxs[idx].get();
            if (!dctx || !dctx->mAvailable) {
                if (nextList.size() > (size_t)idx)
                    idx = nextList[idx];
                else
                    break;
                if (idx < 0 || idx == 65535)
                    break;
                continue;
            }

            // Check part visibility
            auto* pctx = mModelContext->getPartsContext(dctx->mPartsIndex);
            if (pctx && pctx->mPartsData && !pctx->mPartsData->isVisible()) {
                if (nextList.size() > (size_t)idx)
                    idx = nextList[idx];
                else
                    break;
                if (idx < 0 || idx == 65535)
                    break;
                continue;
            }
            if (pctx && pctx->getPartsOpacity() < 0.1f) {
                if (nextList.size() > (size_t)idx)
                    idx = nextList[idx];
                else
                    break;
                if (idx < 0 || idx == 65535)
                    break;
                continue;
            }

            // Get part ID
            std::string partId;
            if (pctx && pctx->mPartsData && pctx->mPartsData->getId())
                partId = pctx->mPartsData->getId()->str();

            // Skip duplicate parts
            bool dup = false;
            for (auto& r : result)
                if (r == partId) {
                    dup = true;
                    break;
                }
            if (dup) {
                if (nextList.size() > (size_t)idx)
                    idx = nextList[idx];
                else
                    break;
                if (idx < 0 || idx == 65535)
                    break;
                continue;
            }

            // Triangle hit test against drawable vertices
            auto& verts = !dctx->mTransformedPoints.empty() ? dctx->mTransformedPoints
                                                            : dctx->mInterpolatedPoints;
            auto* dd = static_cast<Mesh*>(mModelContext->getDrawData(idx));
            if (dd && !verts.empty()) {
                auto& indices = dd->getIndexArray();
                for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                    int i0 = indices[i] * 2, i1 = indices[i + 1] * 2, i2 = indices[i + 2] * 2;
                    if (i0 < 0 || i1 < 0 || i2 < 0)
                        continue;
                    if ((size_t)i0 + 1 >= verts.size() || (size_t)i1 + 1 >= verts.size() ||
                        (size_t)i2 + 1 >= verts.size())
                        continue;
                    if (isInTriangle(mx,
                                     my,
                                     verts[i0],
                                     verts[i0 + 1],
                                     verts[i1],
                                     verts[i1 + 1],
                                     verts[i2],
                                     verts[i2 + 1])) {
                        result.push_back(partId);
                        if (topOnly)
                            return result;
                        break;
                    }
                }
            }

            if (nextList.size() > (size_t)idx)
                idx = nextList[idx];
            else
                break;
            if (idx < 0 || idx == 65535)
                break;
        }
    }
    return result;
}

void LAppModel::CreateRenderer()
{
    if (mRenderer) 
    {
        Warn("Renderer already exists, releasing it first");
        return;
    }
    mRenderer = std::make_unique<GLRenderer>(mModelContext.get(), (int)mTexturePaths.size());
    for (size_t i = 0; i < mTexturePaths.size(); i++) {
        std::string texPath = mModelHomeDir + mTexturePaths[i];
        int w, h, n;
        auto texData = readFile(texPath);
        unsigned char* pixels =
            texData.empty()
                ? nullptr
                : stbi_load_from_memory(texData.data(), (int)texData.size(), &w, &h, &n, 4);
        if (!pixels)
            continue;
        Info("Load texture[%zu/%zu]: %s", i + 1, mTexturePaths.size(), mTexturePaths[i].c_str());

        GLuint texId;
        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        mRenderer->setTexture((int)i, (int)texId);
        stbi_image_free(pixels);
    }
}
void LAppModel::ReleaseRenderer()
{
    if (!mRenderer)
    {
        Warn("Renderer not initialized, nothing to release");
        return;
    }
    mRenderer.reset();
}
}   // namespace live2d
