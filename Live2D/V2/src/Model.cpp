#include "V2/Model.hpp"
#include "IModel.hpp"
#include "Id.hpp"
#include "PartsData.hpp"
#include "PartsDataContext.hpp"
#include "Mesh.hpp"
#include "MeshContext.hpp"
#include "L2DExpressionMotion.hpp"
#include "L2DEyeBlink.hpp"
#include "L2DMotionManager.hpp"
#include "L2DPhysics.hpp"
#include "L2DPose.hpp"
#include "GLRenderer.hpp"
#include "Log.hpp"
#include "ModelContext.hpp"
#include "Live2DMotion.hpp"
#include "UtSystem.hpp"
#include "nlohmann/json.hpp"
#ifdef __ANDROID__
#include <GLES/gl.h>
#include <GLES3/gl3.h>
#else
#include <GL/glew.h>
#endif
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stb_image.h>
#include <string>
#include <vector>

namespace Live2D {
namespace V2 {

using json = nlohmann::json;
using namespace Live2D::Common::Log;

// Helper: read entire file using std::filesystem::u8path for Unicode path support
static std::vector<uint8_t> readFile(const std::string& path) {
    std::filesystem::path fp = std::filesystem::u8path(path);
    std::ifstream f(fp, std::ios::binary | std::ios::ate);
    if (!f)
        return {};   // missing file: tellg() would be -1 -> vector((size_t)-1) throws
    auto sz = f.tellg();
    f.seekg(0);
    std::vector<uint8_t> data((size_t)sz);
    f.read((char*)data.data(), sz);
    f.close();
    return data;
}

// homeDir 已含尾部分隔符（可为空串）；rel 为绝对路径（含 '/' '\' 开头）时原样返回
static std::string JoinPath(const std::string& homeDir, const std::string& rel) {
    if (rel.empty())
        return rel;
    std::filesystem::path p = std::filesystem::u8path(rel);
    if (p.is_absolute() || rel[0] == '/' || rel[0] == '\\')
        return rel;
    return homeDir + rel;
}

// Simple JSON texture path extractor
static void parseTexturePaths(const json& data, std::vector<std::string>& texPaths) {
    auto textures = data.find("textures");
    if (textures != data.end()) {
        texPaths = textures->get<std::vector<std::string>>();
    }
}

Model::Model()
    : mRenderer(nullptr) {}
Model::~Model() = default;

void Model::LoadModelJson(const char* path, bool createRenderer) {
    const std::string pathStr(path ? path : "");

    // 复用 readFile（u8path + 缺失文件返回空 vector 而不是抛异常）
    auto bytes = readFile(pathStr);
    if (bytes.empty()) {
        LOGE("Failed to read model json: %s", pathStr.c_str());
        return;
    }
    const std::string jsonText(reinterpret_cast<const char*>(bytes.data()), bytes.size());

    // 资源根目录 = json 所在目录（含尾部分隔符）
    LoadModelJsonImpl(jsonText, pathStr.substr(0, pathStr.find_last_of("/\\") + 1), createRenderer);
}

void Model::LoadFromJsonString(const char* jsonData, bool createRenderer, const char* rootPath) {
    if (jsonData == nullptr) {
        LOGE("model json string is null");
        return;
    }

    // root_path → 资源根目录（补尾部分隔符；空串 = 相对当前工作目录）
    std::string homeDir(rootPath ? rootPath : "");
    if (!homeDir.empty() && homeDir.back() != '/' && homeDir.back() != '\\') {
        homeDir += '/';
    }

    LoadModelJsonImpl(std::string(jsonData), homeDir, createRenderer);
}

void Model::LoadModelJsonImpl(const std::string& jsonText, const std::string& homeDir,
                              bool createRenderer) {
    // Read JSON（不抛异常；畸形输入走 discarded）
    json data = json::parse(jsonText, nullptr, false);
    if (data.is_discarded() || !data.is_object()) {
        LOGE("invalid model json");
        return;
    }

    // Get base directory and load .moc from JSON model field
    mModelHomeDir = homeDir;
    LOGI("Model home directory: %s", mModelHomeDir.c_str());

    auto modelIt = data.find("model");
    if (modelIt == data.end() || !modelIt->is_string()) {
        LOGE("model json has no \"model\" field");
        return;
    }
    std::string mocPath = JoinPath(mModelHomeDir, modelIt->get<std::string>());

    auto mocData = readFile(mocPath);
    if (mocData.empty()) {
        LOGE("Failed to read .moc file: %s", mocPath.c_str());
        return;
    }
    loadModelData(mocData, 0);
    mModelMatrix.mWidth = (float)mModelImpl->getCanvasWidth();
    mModelMatrix.mHeight = (float)mModelImpl->getCanvasHeight();
    LOGI("Load model: %s", mocPath.c_str());

    // Load texture paths from JSON
    parseTexturePaths(data, mTexturePaths);

    // Load physics (Python: getPhysicsFile() is not None — lapp_model.py:63-64)
    auto physics = data.find("physics");
    if (physics != data.end() && physics->is_string()) {
        auto phyFile = physics->get<std::string>();
        auto phyData = readFile(JoinPath(mModelHomeDir, phyFile));
        if (!phyData.empty()) {
            LOGI("Load physics: %s", phyFile.c_str());
            loadPhysics(phyData);
        }
    }

    // Load pose file (Python: getPoseFile() is None for both missing key and
    // explicit null — e.g. Resources/v2/托尔/model0.json has "pose": null)
    auto pose = data.find("pose");
    if (pose != data.end() && pose->is_string()) {
        auto poseFile = pose->get<std::string>();
        auto posePath = JoinPath(mModelHomeDir, poseFile);
        auto poseData = readFile(posePath);
        if (!poseData.empty()) {
            LOGI("Load pose: %s", poseFile.c_str());
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
                auto motPath = JoinPath(mModelHomeDir, motFile);
                auto motData = readFile(motPath);
                if (!motData.empty()) {
                    auto* motion = Live2DMotion::load(motData);
                    LOGI("Load motion: %s", motFile.c_str());
                    motVec.emplace_back(motion);

                    std::string sound;
                    auto soundIt = motionEntry.find("sound");
                    if (soundIt != motionEntry.end() && soundIt->is_string())
                        sound = soundIt->get<std::string>();
                    mMotionInfos[groupName].push_back({motFile, sound});
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
            auto expPath = JoinPath(mModelHomeDir, expFile);
            auto expData = readFile(expPath);
            auto expName = expEntry["name"].get<std::string>();
            if (!expData.empty()) {
                auto* expr = L2DExpressionMotion::load(expData);
                LOGI("Load expression: %s", expName.c_str());
                mExpressions[expName].reset(expr);
                mExpressionFiles[expName] = expFile;
            }
        }
    }

    if (createRenderer) {
        CreateRenderer();
    }
}

const char* Model::GetModelHomeDir() {
    return mModelHomeDir.c_str();
}
int Model::Version() const {
    return 2;
}

void Model::Resize(int w, int h) {
    mMatrixManager.onResize(w, h);
}
void Model::Drag(float x, float y) {
    // Convert screen coords to scene coords (match Python MatrixManager.screenToScene)
    float w = (float)mMatrixManager.getWidth();
    float h = (float)mMatrixManager.getHeight();
    float sx = (x - w * 0.5f) * 2.0f / h;
    float sy = (y - h * 0.5f) * -2.0f / h;
    mDragMgr.set(sx, sy);
}
bool Model::IsMotionFinished() {
    return mMainMotionMgr->isFinished();
}
void Model::SetOffset(float dx, float dy) {
    mOffsetX = dx;
    mOffsetY = dy;
    mMatrixManager.setOffset(dx, dy);
}
void Model::SetOffsetX(float x) {
    mOffsetX = x;
    mMatrixManager.setOffset(mOffsetX, mOffsetY);
}
void Model::SetOffsetY(float y) {
    mOffsetY = y;
    mMatrixManager.setOffset(mOffsetX, mOffsetY);
}
void Model::SetScale(float s) {
    mMatrixManager.setScale(s);
}
void Model::SetParameterValue(const char* id, float val, float weight) {
    int idx = mModelContext->getParamIndex(&Id::getID(id));
    mModelContext->setParamFloat(idx, val, weight);
    // Match Python: also update savedParamValues so loadParam() doesn't revert
    if (idx >= 0 && idx < (int)mModelContext->mSavedParamValues.size())
        mModelContext->mSavedParamValues[idx] =
            mModelContext->mSavedParamValues[idx] * (1.0f - weight) + val * weight;
}
void Model::SetParameterValue(int index, float val, float weight) {
    SetParameterValue(GetParameterId(index), val, weight);
}
void Model::AddParameterValue(const char* id, float val) {
    int idx = mModelContext->getParamIndex(&Id::getID(id));
    mModelContext->setParamFloat(idx, mModelContext->getParamFloat(idx) + val);
}
void Model::AddParameterValue(int index, float val) {
    AddParameterValue(GetParameterId(index), val);
}
int Model::GetParameterCount() {
    return (int)mModelContext->mParamValues.size();
}
void Model::GetParameterIds(void* collector, void (*collect)(void* collector, const char* id)) {
    const int count = GetParameterCount();
    for (int i = 0; i < count; i++) {
        collect(collector, GetParameterId(i));
    }
}
const char* Model::GetParameterId(int index) {
    auto& ids = mModelContext->mParamIdList;
    if (index >= 0 && index < (int)ids.size() && ids[index])
        return ids[index]->str().c_str();
    return "";
}
float Model::GetParameterValue(int index) {
    return mModelContext->getParamFloat(index);
}
float Model::GetParameterMinimumValue(int index) {
    return mModelContext->getParamMin(index);
}
float Model::GetParameterMaximumValue(int index) {
    return mModelContext->getParamMax(index);
}
float Model::GetParameterDefaultValue(int index) {
    return mModelContext->getParamDefault(index);
}
int Model::GetPartCount() const {
    return (int)mModelContext->mPartsDataList.size();
}
void Model::GetPartIds(void* collector, void (*collect)(void* collector, const char* id)) const {
    auto& parts = mModelContext->mPartsDataList;
    const int count = (int)parts.size();
    for (int i = 0; i < count; i++) {
        if (parts[i]->getId())
            collect(collector, parts[i]->getId()->str().c_str());
    }
}
const char* Model::GetPartId(int index) const {
    auto& parts = mModelContext->mPartsDataList;
    if (index >= 0 && index < (int)parts.size() && parts[index]->getId())
        return parts[index]->getId()->str().c_str();
    return "";
}
void Model::SetPartOpacity(int index, float val) {
    mModelContext->setPartsOpacity(index, val);
}
void Model::Update(float deltaSecs) {
    // Model 是唯一读墙钟的地方，且只做一件事: 每帧算一次 dt
    float dt;
    if (deltaSecs < 0.0f) {
        // 墙钟路径（Python v2 1:1）
        float now = (float)UtSystem::getUserTimeMSec();
        dt = (mLastFrameTimeMs != 0.0f) ? (now - mLastFrameTimeMs) / 1000.0f : 0.0f;
        mLastFrameTimeMs = now;
    } else {
        // delta 路径
        dt = deltaSecs;
    }
    float dtMs = dt * 1000.0f;
    mBreathTimeMs += dtMs;

    // 表情 fadeout: 到期恢复上一个持久表情
    if (mFadeoutMs >= 0.0f) {
        mFadeoutElapsedMs += dtMs;
        if (mFadeoutElapsedMs >= mFadeoutMs) {
            mFadeoutMs = -1.0f;
            SetExpression(mLastExpression.c_str());
        }
    }

    mDragMgr.update(dt);
    setDrag(mDragMgr.getX(), mDragMgr.getY());

    // Match v2 Python update() flow
    bool updated = false;
    if (mClearFlag) {
        mMainMotionMgr->stopAllMotions();
        if (mPose) {
            for (auto& g : mPose->mMGroups)
                for (auto& p : g.parts)
                    p.initIndex(mModelContext.get());
        }
        mClearFlag = false;
    } else {
        mModelContext->loadParam();
        updated = mMainMotionMgr->updateParam(mModelContext.get(), dtMs);
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
        mEyeBlink->updateParam(mModelContext.get(), dtMs);

    // Python skips expression update when no expressions exist
    if (!mExpressions.empty())
        mExpressionMgr->updateParam(mModelContext.get(), dtMs);

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

    // Auto-breath animation (match v2 Python periods)
    if (mAutoBreath) {
        float t = mBreathTimeMs / 1000.0f;
        addParam("PARAM_ANGLE_X", 15.0f * sinf(t / 6.5345f), 0.5f);
        addParam("PARAM_ANGLE_Y", 8.0f * sinf(t / 3.5345f), 0.5f);
        addParam("PARAM_ANGLE_Z", 10.0f * sinf(t / 5.5345f), 0.5f);
        addParam("PARAM_BODY_ANGLE_X", 4.0f * sinf(t / 15.5345f), 0.5f);
        int breathIdx = mModelContext->getParamIndex(&Id::getID("PARAM_BREATH"));
        if (breathIdx >= 0)
            mModelContext->setParamFloat(breathIdx, 0.5f + 0.5f * sinf(t / 3.2345f));
    }

    if (mPhysics)
        mPhysics->updateParam(mModelContext.get(), (long long)dtMs);
    if (mPose)
        mPose->updateParam(mModelContext.get(), dt);
}
void Model::Draw() {
    // Match v2 Python: process deformer chain in draw(), not update()
    // This allows SetParameterValue between Update() and Draw() to take effect
    mModelContext->update();

    auto mvp = mMatrixManager.getMvp(&mModelMatrix);

    if (!mRenderer) {
        LOGE("Renderer not initialized");
        return;
    }
    mRenderer->setMatrix(mvp.data());
    mRenderer->preDraw(mModelContext.get());
    mRenderer->draw(mModelContext.get());
}
bool Model::IsAreaHit(const char* area, float x, float y) {
    // Convert screen pixels → scene coords (match Python screenToScene)
    float w = (float)mMatrixManager.getWidth();
    float h = (float)mMatrixManager.getHeight();
    float sx = (x - w * 0.5f) * 2.0f / h;
    float sy = (y - h * 0.5f) * -2.0f / h;
    return hitTestSimple(area, sx, sy);
}
void Model::SetExpression(const char* name, float fadeoutMs) {
    auto it = mExpressions.find(name);
    if (it == mExpressions.end())
        return;
    LOGI("Set expression: %s", name);
    mExpressionMgr->startMotion(it->second.get(), false);

    // fadeout 语义: >=0 为临时表情（到时恢复上一个持久表情），<0 为持久表情
    if (fadeoutMs >= 0.0f) {
        mFadeoutMs = fadeoutMs;
        mFadeoutElapsedMs = 0;
    } else {
        mFadeoutMs = -1.0f;
        mLastExpression = name;
    }
}
const char* Model::SetRandomExpression(float fadeoutMs) {
    if (!mExpressions.empty()) {
        auto it = mExpressions.begin();
        std::advance(it, rand() % mExpressions.size());
        LOGI("Start random expression: %s", it->first.c_str());
        mExpressionMgr->startMotion(it->second.get(), false);
        if (fadeoutMs >= 0.0f) {
            mFadeoutMs = fadeoutMs;
            mFadeoutElapsedMs = 0;
        } else {
            mFadeoutMs = -1.0f;
            mLastExpression = it->first;
        }
        return it->first.c_str();
    }
    return nullptr;
}
void Model::StartMotion(const std::string& group, int no, int priority, MotionCallback onStart,
                        MotionCallback onFinish) {
    mOnStartMotion = std::move(onStart);
    mOnFinishMotion = std::move(onFinish);
    auto it = mMotions.find(group);
    if (it != mMotions.end() && !it->second.empty()) {
        if (no < 0 || no >= (int)it->second.size())
            no = 0;

        // Priority check (match Python v2)
        if (priority == MotionPriority::Force) {
            LOGI("Start motion (force): group=%s no=%d priority=%d", group.c_str(), no, priority);
            mMainMotionMgr->setReservePriority(priority);
        } else if (!mMainMotionMgr->reserveMotion(priority)) {
            // Lower priority than current motion — don't play
            LOGI("Start motion rejected (low priority): group=%s no=%d priority=%d current=%d",
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
        LOGD("Start motion: group=%s no=%d priority=%d", group.c_str(), no, priority);
        mMainMotionMgr->startMotionPrio(it->second[no].get(), priority);
    } else {
        LOGD("Start motion: group=%s not found or empty", group.c_str());
        if (mOnStartMotion)
            mOnStartMotion(group, no);
        if (mOnFinishMotion)
            mOnFinishMotion(group, no);
        mOnStartMotion = nullptr;
        mOnFinishMotion = nullptr;
    }
}
void Model::StartRandomMotion(const std::string& group, int priority, MotionCallback onStart,
                              MotionCallback onFinish) {
    if (group.empty()) {
        if (mMotions.empty())
            return;
        int idx = rand() % (int)mMotions.size();
        auto it = mMotions.begin();
        std::advance(it, idx);
        int count = (int)it->second.size();
        int no = (count > 1) ? (rand() % count) : 0;
        StartMotion(it->first, no, priority, std::move(onStart), std::move(onFinish));
    } else {
        auto it = mMotions.find(group);
        int count = (it != mMotions.end()) ? (int)it->second.size() : 1;
        int no = (count > 1) ? (rand() % count) : 0;
        StartMotion(group, no, priority, std::move(onStart), std::move(onFinish));
    }
}
void Model::StopAllMotions() {
    mClearFlag = true;
}
void Model::ResetExpression() {
    mFadeoutMs = -1.0f;
    mFadeoutElapsedMs = 0;
    mLastExpression.clear();
    mExpressionMgr->stopAllMotions();
}
void Model::ResetPose() {
    if (mPose) {
        for (auto& g : mPose->mMGroups)
            for (auto& p : g.parts)
                p.initIndex(mModelContext.get());
    }
}
void Model::Rotate(float deg) {
    mMatrixManager.rotate(deg);
}
void Model::SetPartScreenColor(int index, float r, float g, float b, float a) {
    mModelContext->setPartScreenColor(index, r, g, b, a);
}
void Model::SetPartMultiplyColor(int index, float r, float g, float b, float a) {
    mModelContext->setPartMultiplyColor(index, r, g, b, a);
}
void Model::GetPartScreenColor(int index, float& r, float& g, float& b, float& a) const {
    auto* ctx = mModelContext->getPartsContext(index);
    r = ctx->mScreenColor[0];
    g = ctx->mScreenColor[1];
    b = ctx->mScreenColor[2];
    a = ctx->mScreenColor[3];
}
void Model::GetPartMultiplyColor(int index, float& r, float& g, float& b, float& a) const {
    auto* ctx = mModelContext->getPartsContext(index);
    r = ctx->mMultiplyColor[0];
    g = ctx->mMultiplyColor[1];
    b = ctx->mMultiplyColor[2];
    a = ctx->mMultiplyColor[3];
}
static bool isInTriangle(float px, float py, float ax, float ay, float bx, float by, float cx,
                         float cy) {
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

std::vector<std::string> Model::hitIds(float x, float y, bool topOnly, bool wantDrawableId) {
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

            // Get hit id: part id (default) or drawData id
            std::string hitId;
            if (wantDrawableId) {
                auto* dd0 = static_cast<Mesh*>(mModelContext->getDrawData(idx));
                if (dd0 && dd0->getId())
                    hitId = dd0->getId()->str();
            } else {
                if (pctx && pctx->mPartsData && pctx->mPartsData->getId())
                    hitId = pctx->mPartsData->getId()->str();
            }

            // Skip duplicate ids
            bool dup = false;
            for (auto& r : result)
                if (r == hitId) {
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
                        result.push_back(hitId);
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

void Model::HitPart(float x, float y, void* collector,
                    void (*collect)(void* collector, const char* id), bool topOnly) {
    auto ids = hitIds(x, y, topOnly, false);
    for (auto& id : ids)
        collect(collector, id.c_str());
}

void Model::CreateRenderer(int maskBufferCount) {
    (void)maskBufferCount;   // v2 固定一个裁剪缓冲
    if (mRenderer) {
        LOGW("Renderer already exists, skipping creation");
        return;
    }
    mRenderer = std::make_unique<GLRenderer>(mModelContext.get(), (int)mTexturePaths.size());
    for (size_t i = 0; i < mTexturePaths.size(); i++) {
        std::string texPath = JoinPath(mModelHomeDir, mTexturePaths[i]);
        int w, h, n;
        auto texData = readFile(texPath);
        unsigned char* pixels =
            texData.empty()
                ? nullptr
                : stbi_load_from_memory(texData.data(), (int)texData.size(), &w, &h, &n, 4);
        if (!pixels)
            continue;
        LOGI("Load texture[%zu/%zu]: %s", i + 1, mTexturePaths.size(), mTexturePaths[i].c_str());

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
void Model::DestroyRenderer() {
    if (!mRenderer) {
        LOGW("Renderer not initialized, nothing to release");
        return;
    }
    mRenderer.reset();
}

void Model::GetCanvasSize(float& w, float& h) {
    w = (float)mModelImpl->getCanvasWidth();
    h = (float)mModelImpl->getCanvasHeight();
}
void Model::GetCanvasSizePixel(float& w, float& h) {
    GetCanvasSize(w, h);   // v2 pixelsPerUnit == 1
}
float Model::GetPixelsPerUnit() {
    return 1.0f;
}
void Model::SetAutoBreath(bool v) {
    mAutoBreath = v;
}
void Model::SetAutoBlink(bool v) {
    mAutoBlink = v;
}
bool Model::AutoBreathEnabled() const {
    return mAutoBreath;
}
bool Model::AutoBlinkEnabled() const {
    return mAutoBlink;
}
int Model::GetExpressionCount() {
    return (int)mExpressions.size();
}
const float* Model::GetMvp() {
    auto mvp = mMatrixManager.getMvp(&mModelMatrix);
    for (int i = 0; i < 16; i++)
        mMvpCache[i] = mvp[i];
    return mMvpCache;
}

// ---- 细化更新（dt 由外部传入，与 Update(deltaSecs) 的 delta 路径同构）----

bool Model::UpdateMotion(float deltaSecs) {
    mModelContext->loadParam();
    bool updated = mMainMotionMgr->updateParam(mModelContext.get(), deltaSecs * 1000.0f);
    mModelContext->saveParam();
    return updated;
}
void Model::UpdateDrag(float deltaSecs) {
    mDragMgr.update(deltaSecs);
    setDrag(mDragMgr.getX(), mDragMgr.getY());

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
}
void Model::UpdateBreath(float deltaSecs) {
    mBreathTimeMs += deltaSecs * 1000.0f;
    // Auto-breath animation (match v2 Python periods)
    auto addParam = [&](const char* id, float value, float weight) {
        int idx = mModelContext->getParamIndex(&Id::getID(id));
        if (idx >= 0) {
            float cur = mModelContext->getParamFloat(idx);
            mModelContext->setParamFloat(idx, cur + value * weight);
        }
    };
    float t = mBreathTimeMs / 1000.0f;
    addParam("PARAM_ANGLE_X", 15.0f * sinf(t / 6.5345f), 0.5f);
    addParam("PARAM_ANGLE_Y", 8.0f * sinf(t / 3.5345f), 0.5f);
    addParam("PARAM_ANGLE_Z", 10.0f * sinf(t / 5.5345f), 0.5f);
    addParam("PARAM_BODY_ANGLE_X", 4.0f * sinf(t / 15.5345f), 0.5f);
    int breathIdx = mModelContext->getParamIndex(&Id::getID("PARAM_BREATH"));
    if (breathIdx >= 0)
        mModelContext->setParamFloat(breathIdx, 0.5f + 0.5f * sinf(t / 3.2345f));
}
void Model::UpdateBlink(float deltaSecs) {
    if (mEyeBlink)
        mEyeBlink->updateParam(mModelContext.get(), deltaSecs * 1000.0f);
}
void Model::UpdateExpression(float deltaSecs) {
    if (!mExpressions.empty())
        mExpressionMgr->updateParam(mModelContext.get(), deltaSecs * 1000.0f);
}
void Model::UpdatePhysics(float deltaSecs) {
    if (mPhysics)
        mPhysics->updateParam(mModelContext.get(), (long long)(deltaSecs * 1000.0f));
}
void Model::UpdatePose(float deltaSecs) {
    if (mPose)
        mPose->updateParam(mModelContext.get(), deltaSecs);
}

// ---- 参数保存/恢复 ----

void Model::SetAndSaveParameterValue(const char* id, float value, float weight) {
    // v2 的 SetParameterValue 本身就会同步 saved 值（1:1 Python 行为）
    SetParameterValue(id, value, weight);
}
void Model::SetAndSaveParameterValue(int index, float value, float weight) {
    SetParameterValue(index, value, weight);
}
void Model::AddAndSaveParameterValue(const char* id, float value) {
    AddParameterValue(id, value);
    // 只保存被加的参数（等价 V3 的逐参数保存，不做全量快照）
    int idx = mModelContext->getParamIndex(&Id::getID(id));
    if (idx >= 0 && idx < (int)mModelContext->mSavedParamValues.size())
        mModelContext->mSavedParamValues[idx] = mModelContext->getParamFloat(idx);
}
void Model::AddAndSaveParameterValue(int index, float value) {
    AddParameterValue(index, value);
    if (index >= 0 && index < (int)mModelContext->mSavedParamValues.size())
        mModelContext->mSavedParamValues[index] = mModelContext->getParamFloat(index);
}
void Model::LoadParameters() {
    mModelContext->loadParam();
}
void Model::SaveParameters() {
    mModelContext->saveParam();
}
void Model::ResetAllParameters() {
    const int count = GetParameterCount();
    for (int i = 0; i < count; i++) {
        float def = mModelContext->getParamDefault(i);
        mModelContext->setParamFloat(i, def);
        if (i < (int)mModelContext->mSavedParamValues.size())
            mModelContext->mSavedParamValues[i] = def;
    }
}

// ---- 变换 ----

void Model::SetScaleX(float scaleX) {
    mMatrixManager.setScaleX(scaleX);
}
void Model::SetScaleY(float scaleY) {
    mMatrixManager.setScaleY(scaleY);
}

// ---- 动作组 ----

int Model::LoadExtraMotion(const char* group, const char* motionJsonPath) {
    auto motData = readFile(motionJsonPath);
    if (motData.empty()) {
        LOGE("Failed to read motion file: %s", motionJsonPath);
        return -1;
    }
    auto* motion = Live2DMotion::load(motData);
    if (!motion) {
        LOGE("Failed to load motion: %s", motionJsonPath);
        return -1;
    }
    auto& motVec = mMotions[group];
    motVec.emplace_back(motion);
    mMotionInfos[group].push_back({motionJsonPath, ""});
    LOGI("Load extra motion: %s => [%s_%zu]", motionJsonPath, group, motVec.size() - 1);
    return (int)motVec.size() - 1;
}
int Model::GetMotionGroupCount() {
    return (int)mMotions.size();
}
int Model::GetMotionCount(const char* group) {
    auto it = mMotions.find(group);
    return (it != mMotions.end()) ? (int)it->second.size() : 0;
}
const char* Model::GetMotionSound(const char* group, int no) {
    auto it = mMotionInfos.find(group);
    if (it != mMotionInfos.end() && no >= 0 && no < (int)it->second.size())
        return it->second[no].sound.c_str();
    return "";
}
void Model::GetMotions(void* collector,
                       void (*collect)(void* collector, const char* group, int no, const char* file,
                                       const char* sound)) {
    for (auto& [group, infos] : mMotionInfos) {
        for (int no = 0; no < (int)infos.size(); no++) {
            collect(collector, group.c_str(), no, infos[no].file.c_str(), infos[no].sound.c_str());
        }
    }
}

// ---- 命中测试 ----

void Model::HitDrawable(float x, float y, void* collector,
                        void (*collect)(void* collector, const char* id), bool topOnly) {
    auto ids = hitIds(x, y, topOnly, true);
    for (auto& id : ids)
        collect(collector, id.c_str());
}
bool Model::IsPartHit(int index, float x, float y) {
    const char* id = GetPartId(index);
    if (id[0] == '\0')
        return false;
    auto ids = hitIds(x, y, false, false);
    for (auto& hit : ids)
        if (hit == id)
            return true;
    return false;
}
bool Model::IsDrawableHit(int index, float x, float y) {
    auto* dd = static_cast<Mesh*>(mModelContext->getDrawData(index));
    if (!dd || !dd->getId())
        return false;
    std::string id = dd->getId()->str();
    auto ids = hitIds(x, y, false, true);
    for (auto& hit : ids)
        if (hit == id)
            return true;
    return false;
}

// ---- drawable ----

int Model::GetDrawableCount() {
    return (int)mModelContext->mDrawContextList.size();
}
void Model::GetDrawableIds(void* collector, void (*collect)(void* collector, const char* id)) {
    const int count = GetDrawableCount();
    for (int i = 0; i < count; i++) {
        auto* dd = mModelContext->getDrawData(i);
        if (dd && dd->getId())
            collect(collector, dd->getId()->str().c_str());
    }
}
const float* Model::GetDrawableVertices(int index) {
    auto* ctx = mModelContext->getDrawContext(index);
    if (!ctx) {
        mDrawableVertexCache.clear();
        return mDrawableVertexCache.data();
    }
    auto& verts = !ctx->mTransformedPoints.empty() ? ctx->mTransformedPoints
                                                   : ctx->mInterpolatedPoints;
    mDrawableVertexCache = verts;
    return mDrawableVertexCache.data();
}
int Model::GetDrawableVertexCount(int index) {
    GetDrawableVertices(index);
    return (int)mDrawableVertexCache.size() / 2;
}
int Model::GetDrawableVertexIndexCount(int index) {
    auto* dd = static_cast<Mesh*>(mModelContext->getDrawData(index));
    if (!dd)
        return 0;
    return (int)dd->getIndexArray().size();
}
const unsigned short* Model::GetDrawableIndices(int index) {
    auto* dd = static_cast<Mesh*>(mModelContext->getDrawData(index));
    if (!dd) {
        mDrawableIndexCache.clear();
        return mDrawableIndexCache.data();
    }
    auto& indices = dd->getIndexArray();
    mDrawableIndexCache.assign(indices.begin(), indices.end());
    return mDrawableIndexCache.data();
}
void Model::SetDrawableMultiColor(int index, float r, float g, float b, float a) {
    // v2 没有 drawable 级颜色，作用于该 drawable 所属的 part
    auto* ctx = mModelContext->getDrawContext(index);
    if (!ctx)
        return;
    mModelContext->setPartMultiplyColor(ctx->mPartsIndex, r, g, b, a);
}
void Model::SetDrawableScreenColor(int index, float r, float g, float b, float a) {
    // v2 没有 drawable 级颜色，作用于该 drawable 所属的 part
    auto* ctx = mModelContext->getDrawContext(index);
    if (!ctx)
        return;
    mModelContext->setPartScreenColor(ctx->mPartsIndex, r, g, b, a);
}

// ---- 表情 ----

void Model::AddExpression(const char* expressionId) {
    // v2 只有一个表情管理器（无叠加），等价于 SetExpression
    SetExpression(expressionId);
}
void Model::RemoveExpression(const char* expressionId) {
    (void)expressionId;
    // v2 只有一个表情管理器，无法移除单个表情，停止全部
    mExpressionMgr->stopAllMotions();
}
void Model::ResetExpressions() {
    mFadeoutMs = -1.0f;
    mFadeoutElapsedMs = 0;
    mLastExpression.clear();
    mExpressionMgr->stopAllMotions();
}
void Model::GetExpressions(void* collector,
                           void (*collect)(void* collector, const char* id, const char* file)) {
    for (auto& [id, file] : mExpressionFiles) {
        collect(collector, id.c_str(), file.c_str());
    }
}
void Model::LoadExtraExpression(const char* expressionId, const char* expressionJsonPath) {
    auto expData = readFile(expressionJsonPath);
    if (expData.empty()) {
        LOGE("Failed to read expression file: %s", expressionJsonPath);
        return;
    }
    auto* expr = L2DExpressionMotion::load(expData);
    if (!expr) {
        LOGE("Failed to load expression: %s", expressionJsonPath);
        return;
    }
    mExpressions[expressionId].reset(expr);
    mExpressionFiles[expressionId] = expressionJsonPath;
    LOGI("Load extra expression: %s => [%s]", expressionJsonPath, expressionId);
}

bool Model::HasMocConsistencyFromFile(const char* mocFileName) {
    (void)mocFileName;
    // v2 的 .moc 二进制格式没有 moc3 的一致性检查概念
    return false;
}
}   // namespace V2
}   // namespace Live2D
