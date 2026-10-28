#include "V2/Model.hpp"
#include "Drawable.hpp"
#include "ExpressionMotion.hpp"
#include "EyeBlink.hpp"
#include "FileUtils.hpp"
#include "GraphicsBackend.hpp"
#include "IModel.hpp"
#include "Id.hpp"
#include "MotionManager.hpp"
#include "Part.hpp"
#include "Physics.hpp"
#include "Pose.hpp"
// for linux: memcpy
#include <memory>
#include <cstddef>
#ifdef CSM_TARGET_VULKAN
#include "VkRenderer.hpp"
#include "VkUtil.hpp"
#include "VulkanManager.hpp"
#endif
#include "Motion.hpp"
#include "Log.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif
#include "Moc.hpp"
#include "nlohmann/json.hpp"
#ifdef __ANDROID__
#include <GLES/gl.h>
#include <GLES3/gl3.h>
#endif

#if defined(CSM_TARGET_WIN_GL) || defined(CSM_TARGET_LINUX_GL) || defined(CSM_TARGET_MAC_GL)
#include <GL/glew.h>
#include "GLRenderer.hpp"
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
using namespace Live2D::Common;

// homeDir 已含尾部分隔符（可为空串）；rel 为绝对路径（含 '/' '\'
// 开头）时原样返回
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

Model::Model() : BaseModel(), mRenderer(nullptr) {}

Model::~Model() = default;

void Model::LoadModelJson(const char* path, bool createRenderer) {
    const std::string pathStr(path ? path : "");

    // 复用 readFile（u8path + 缺失文件返回空 vector 而不是抛异常）
    auto bytes = FileUtils::LoadFileAsBytes(pathStr);
    if (bytes.empty()) {
        LOGE("Failed to read model json: %s", pathStr.c_str());
        return;
    }
    const std::string jsonText(reinterpret_cast<char*>(bytes.data()), bytes.size());

    // 资源根目录 = json 所在目录（含尾部分隔符）
    LoadModelJsonImpl(jsonText, pathStr.substr(0, pathStr.find_last_of("/\\") + 1), createRenderer);
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
    LOGD("Model home directory: %s", mModelHomeDir.c_str());

    auto modelIt = data.find("model");
    if (modelIt == data.end() || !modelIt->is_string()) {
        LOGE("model json has no \"model\" field");
        return;
    }
    std::string mocPath = JoinPath(mModelHomeDir, modelIt->get<std::string>());

    {
        auto mocData = FileUtils::LoadFileAsBytes(mocPath);
        if (mocData.empty()) {
            LOGE("Failed to read .moc file: %s", mocPath.c_str());
            return;
        }
        LoadModelData(mocData);
    }
    // 画布适配参数在模型生命周期内不变，加载时设置一次
    mMatrixManager.SetFit(
        {(float)mMoc->GetCanvasWidth(), (float)mMoc->GetCanvasHeight(), true, true});
    LOGI("Load model: %s", mocPath.c_str());

    // Load texture paths from JSON
    parseTexturePaths(data, mTexturePaths);

    // Load physics (Python: getPhysicsFile() is not None — lapp_model.py:63-64)
    {
        auto physics = data.find("physics");
        if (physics != data.end() && physics->is_string()) {
            auto phyFile = physics->get<std::string>();
            auto phyData = FileUtils::LoadFileAsBytes(JoinPath(mModelHomeDir, phyFile));
            if (!phyData.empty()) {
                LOGD("Load physics: %s", phyFile.c_str());
                LoadPhysics(phyData);
            }
        }
    }

    // Load pose file (Python: getPoseFile() is None for both missing key and
    // explicit null — e.g. Resources/v2/托尔/model0.json has "pose": null)
    {
        auto pose = data.find("pose");
        if (pose != data.end() && pose->is_string()) {
            auto poseFile = pose->get<std::string>();
            auto posePath = JoinPath(mModelHomeDir, poseFile);
            auto poseData = FileUtils::LoadFileAsBytes(posePath);
            if (!poseData.empty()) {
                LOGI("Load pose: %s", poseFile.c_str());
                mPose.reset(Pose::Load(poseData, mIdPool));
                // Initialize part/param indices once (PartData::initIndex uses the
                // "VISIBLE:" prefix; a bare-id pre-pass here used to append phantom
                // parameters via getParamIndex's extend-on-miss).
                mPose->InitParam(mMoc.get());
            }
        }
    }

    {   // Load motion files
        auto motions = data.find("motions");
        if (motions != data.end()) {
            for (auto& [groupName, motionArray] : motions->items()) {
                auto& motVec = mMotions[groupName];
                for (auto& motionEntry : motionArray) {
                    auto motFile = motionEntry["file"].get<std::string>();
                    auto motPath = JoinPath(mModelHomeDir, motFile);
                    auto motData = FileUtils::LoadFileAsBytes(motPath);
                    if (!motData.empty()) {
                        auto* motion = Motion::Load(motData, mIdPool);
                        LOGD("Load motion: %s", motFile.c_str());
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
    }

    {
        // Load expression files
        auto expressions = data.find("expressions");
        if (expressions != data.end()) {
            for (auto& [index, expEntry] : expressions->items()) {
                auto expFile = expEntry["file"].get<std::string>();
                auto expPath = JoinPath(mModelHomeDir, expFile);
                auto expData = FileUtils::LoadFileAsBytes(expPath);
                auto expName = expEntry["name"].get<std::string>();
                if (!expData.empty()) {
                    auto* expr = ExpressionMotion::Load(expData, mIdPool);
                    LOGD("Load expression: %s", expName.c_str());
                    mExpressions[expName].reset(expr);
                    mExpressionFiles[expName] = expFile;
                }
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
    mMatrixManager.OnResize(w, h);
}
void Model::Drag(float x, float y) {
    // Convert screen coords to scene coords (match Python
    // MatrixManager.screenToScene)
    float w = (float)mMatrixManager.GetWidth();
    float h = (float)mMatrixManager.GetHeight();
    float sx = (x - w * 0.5f) * 2.0f / h;
    float sy = (y - h * 0.5f) * -2.0f / h;
    mDragMgr.Set(sx, sy);
}
bool Model::IsMotionFinished() {
    return mMainMotionMgr->IsFinished();
}
void Model::SetOffset(float dx, float dy) {
    mOffsetX = dx;
    mOffsetY = dy;
    mMatrixManager.SetOffset(dx, dy);
}
void Model::SetOffsetX(float x) {
    mOffsetX = x;
    mMatrixManager.SetOffset(mOffsetX, mOffsetY);
}
void Model::SetOffsetY(float y) {
    mOffsetY = y;
    mMatrixManager.SetOffset(mOffsetX, mOffsetY);
}
void Model::SetScale(float s) {
    mMatrixManager.SetScale(s);
}
void Model::SetParameterValue(const char* id, float val, float weight) {
    int idx = mMoc->GetParamIndex(id);
    mMoc->SetParamValue(idx, val, weight);
    // Match Python: also update savedParamValues so LoadParam() doesn't revert
    if (idx >= 0 && idx < (int)mMoc->mSavedParamValues.size())
        mMoc->mSavedParamValues[idx] =
            mMoc->mSavedParamValues[idx] * (1.0f - weight) + val * weight;
}
void Model::SetParameterValue(int index, float val, float weight) {
    SetParameterValue(GetParameterId(index), val, weight);
}
void Model::AddParameterValue(const char* id, float val) {
    int idx = mMoc->GetParamIndex(id);
    mMoc->SetParamValue(idx, mMoc->GetParamValue(idx) + val);
}
void Model::AddParameterValue(int index, float val) {
    AddParameterValue(GetParameterId(index), val);
}
int Model::GetParameterCount() {
    return (int)mMoc->mParamValues.size();
}
void Model::GetParameterIds(void* collector, void (*collect)(void* collector, const char* id)) {
    const int count = GetParameterCount();
    for (int i = 0; i < count; i++) {
        collect(collector, GetParameterId(i));
    }
}
const char* Model::GetParameterId(int index) {
    auto& ids = mMoc->mParamIds;
    if (index >= 0 && index < (int)ids.size() && ids[index])
        return ids[index]->Str().c_str();
    return "";
}
float Model::GetParameterValue(int index) {
    return mMoc->GetParamValue(index);
}
float Model::GetParameterMinimumValue(int index) {
    return mMoc->GetParamMin(index);
}
float Model::GetParameterMaximumValue(int index) {
    return mMoc->GetParamMax(index);
}
float Model::GetParameterDefaultValue(int index) {
    return mMoc->GetParamDefault(index);
}
int Model::GetPartCount() const {
    return (int)mMoc->mParts.size();
}
void Model::GetPartIds(void* collector, void (*collect)(void* collector, const char* id)) const {
    auto& parts = mMoc->mParts;
    const int count = (int)parts.size();
    for (int i = 0; i < count; i++) {
        if (parts[i]->GetId())
            collect(collector, parts[i]->GetId()->Str().c_str());
    }
}
const char* Model::GetPartId(int index) const {
    auto& parts = mMoc->mParts;
    if (index >= 0 && index < (int)parts.size() && parts[index]->GetId())
        return parts[index]->GetId()->Str().c_str();
    return "";
}
void Model::SetPartOpacity(int index, float val) {
    mMoc->SetPartsOpacity(index, val);
}
void Model::Update(float deltaSecs) {
#if defined(TRACY_ENABLE)
    ZoneScopedN("v2/Update");
#endif
    // Model 是唯一读墙钟的地方，且只做一件事: 每帧算一次 dt
    float dt;
    if (deltaSecs < 0.0f) {
        // 墙钟路径（Python v2 1:1）
        float now = (float)std::chrono::duration<double, std::milli>(
                        std::chrono::steady_clock::now().time_since_epoch())
                        .count();
        dt = (mLastFrameTimeMs != 0.0f) ? (now - mLastFrameTimeMs) / 1000.0f : 0.0f;
        mLastFrameTimeMs = now;
    } else {
        // delta 路径
        dt = deltaSecs;
    }
    float dtMs = dt * 1000.0f;
    mBreathTimeMs += dtMs;

    {
#if defined(TRACY_ENABLE)
        ZoneScopedN("v2/UpdateDrag");
#endif
        mDragMgr.Update(dt);
        SetDrag(mDragMgr.GetX(), mDragMgr.GetY());
    }

    // Match v2 Python Update() flow
    bool updated = false;
    {
#if defined(TRACY_ENABLE)
        ZoneScopedN("v2/UpdateMotion");
#endif
        if (mClearFlag) {
            mMainMotionMgr->StopAllMotions();
            mClearFlag = false;
        } else {
            mMoc->LoadParam();
            updated = mMainMotionMgr->UpdateParam(mMoc.get(), dtMs);
        }
        mMoc->SaveParam();
    }

    {
#if defined(TRACY_ENABLE)
        ZoneScopedN("v2/UpdateBlink");
#endif
        // Python suppresses eye-blink while a main motion is active
        if (!updated && mAutoBlink && mEyeBlink)
            mEyeBlink->UpdateParam(mMoc.get(), dtMs);
    }

    {
#if defined(TRACY_ENABLE)
        ZoneScopedN("v2/UpdateExpression");
#endif
        UpdateExpression(dt);
    }

    // Drag-based parameter updates (match v2 Python)
    auto addParam = [&](const char* id, float value, float weight) {
        int idx = mMoc->GetParamIndex(id);
        if (idx >= 0) {
            float cur = mMoc->GetParamValue(idx);
            mMoc->SetParamValue(idx, cur + value * weight);
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
        int breathIdx = mMoc->GetParamIndex("PARAM_BREATH");
        if (breathIdx >= 0)
            mMoc->SetParamValue(breathIdx, 0.5f + 0.5f * sinf(t / 3.2345f));
    }

    {
#if defined(TRACY_ENABLE)
        ZoneScopedN("v2/UpdatePhysics");
#endif
        if (mPhysics)
            mPhysics->UpdateParam(mMoc.get(), (long long)dtMs);
    }
    {
#if defined(TRACY_ENABLE)
        ZoneScopedN("v2/UpdatePose");
#endif
        if (mPose)
            mPose->UpdateParam(mMoc.get(), dt);
    }

#ifdef CSM_TARGET_VULKAN
    // 本帧参数发生过变化，裁剪遮罩内容需重渲
    if (mRenderer && mRenderer->IsVulkan())
        mRenderer->SetMaskDirty();
#endif
}
void Model::Draw() {
#if defined(TRACY_ENABLE)
    ZoneScopedN("v2/Draw");
#endif
    // Match v2 Python: process deformer chain in Draw(), not Update()
    // This allows SetParameterValue between Update() and Draw() to take effect
    {
#if defined(TRACY_ENABLE)
        ZoneScopedN("v2/ModelUpdate");
#endif
        mMoc->Update();
        // 解析渲染派生数据（opacity/可见性/渲染序列），供 Moc 数据面 getter 使用
        mMoc->Resolve();
    }

    if (!mRenderer) {
        LOGE("Renderer not initialized");
        return;
    }
    mRenderer->SetMatrix(mMatrixManager.GetMvp());
#ifdef CSM_TARGET_VULKAN
    if (mRenderer->IsVulkan()) {
        // 帧解耦：acquire/clear/present 由外部 vkBeginFrame/vkEndFrame 驱动，
        // Draw 仅向帧命令缓冲录制；mask 区间由渲染器自管（挂起/恢复）
        VulkanManager* vkMgr = VulkanManager::GetInstance();
        if (!vkMgr->IsFrameActive())
            return;
        vkMgr->InvalidateSizeIfNeeded(mMatrixManager.GetWidth(), mMatrixManager.GetHeight());
    }
#endif
    {
#if defined(TRACY_ENABLE)
        ZoneScopedN("v2/DrawModel");
#endif
        mRenderer->PreDraw(mMoc.get());
        mRenderer->Draw(mMoc.get());
    }
}
void Model::screenToModel(float* x, float* y) {
    // 像素 → ndc → 完整 mvp 逆变换（含旋转/缩放/平移）
    float w = (float)mMatrixManager.GetWidth();
    float h = (float)mMatrixManager.GetHeight();
    *x = *x / w * 2.0f - 1.0f;
    *y = 1.0f - *y / h * 2.0f;
    Common::MatrixManager::InvertTransform(x, y, mMatrixManager.GetMvp());
}

bool Model::IsAreaHit(const char* area, float x, float y) {
    float mx = x, my = y;
    screenToModel(&mx, &my);
    return HitTestSimple(area, mx, my);
}
void Model::SetExpression(const char* name, float fadeoutMs) {
    auto it = mExpressions.find(name);
    if (it == mExpressions.end())
        return;
    LOGI("Set expression: %s", name);
    mExpressionMgr->StartMotion(it->second.get(), false);

    // fadeout 语义: >=0 为临时表情（到时恢复上一个持久表情），<0 为持久表情
    if (fadeoutMs >= 0.0f) {
        mFadeoutMs = fadeoutMs;
        mFadeoutElapsedMs = 0;
    } else {
        mFadeoutMs = -1.0f;
        mLastExpression = name;
    }
}
std::string Model::SetRandomExpression(float fadeoutMs) {
    if (!mExpressions.empty()) {
        auto it = mExpressions.begin();
        std::advance(it, rand() % mExpressions.size());
        LOGI("Start random expression: %s", it->first.c_str());
        mExpressionMgr->StartMotion(it->second.get(), false);
        if (fadeoutMs >= 0.0f) {
            mFadeoutMs = fadeoutMs;
            mFadeoutElapsedMs = 0;
        } else {
            mFadeoutMs = -1.0f;
            mLastExpression = it->first;
        }
        return it->first.c_str();
    }
    return {};   // 无表情模型：返回空串（nullptr 构造 std::string 是 UB）
}
void Model::StartMotion(const std::string& group, int no, int priority, MotionCallback onStart,
                        MotionCallback onFinish) {
    auto it = mMotions.find(group);
    if (it != mMotions.end() && !it->second.empty()) {
        if (no < 0 || no >= (int)it->second.size())
            no = 0;

        // Priority check (match Python v2)
        if (priority == MotionPriority::MotionPrioNone) {
            LOGI("Start motion (force): group=%s no=%d priority=%d", group.c_str(), no, priority);
            mMainMotionMgr->SetReservePriority(priority);
        } else if (!mMainMotionMgr->ReserveMotion(priority)) {
            // Lower priority than current motion — don't play
            LOGI("Start motion rejected (low priority): group=%s no=%d priority=%d "
                 "current=%d",
                 group.c_str(),
                 no,
                 priority,
                 mMainMotionMgr->mCurrentPriority);
            // 对齐 V3：优先级拒绝路径静默返回，不触发回调
            return;
        }

        // per-motion handler（对齐 V3）：回调挂载到 motion 对象，group/no 由
        // lambda 捕获；motion 复用时整体覆盖，避免残留旧回调
        auto* motion = it->second[no].get();
        if (onStart)
            motion->mOnStart = [cb = std::move(onStart), group, no]() { cb(group, no); };
        else
            motion->mOnStart = nullptr;
        if (onFinish)
            motion->mOnFinish = [cb = std::move(onFinish), group, no]() { cb(group, no); };
        else
            motion->mOnFinish = nullptr;
        LOGD("Start motion: group=%s no=%d priority=%d", group.c_str(), no, priority);
        mMainMotionMgr->StartMotionPrio(motion, priority);
    } else {
        LOGD("Start motion: group=%s not found or empty", group.c_str());
        if (onStart)
            onStart(group, no);
        if (onFinish)
            onFinish(group, no);
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
    mExpressionMgr->StopAllMotions();
    LOGI("Reset expression");
}
void Model::ResetPose() {
    // 索引在 (pose, moc) 生命周期内稳定（参数表 append-only、parts 表静态、
    // moc 不热替换），无需重解析；保留 API 占位对齐 IModel 接口。
}
void Model::Rotate(float deg) {
    mMatrixManager.Rotate(deg);
}
void Model::SetPartScreenColor(int index, float r, float g, float b, float a) {
    mMoc->SetPartScreenColor(index, r, g, b, a);
}
void Model::SetPartMultiplyColor(int index, float r, float g, float b, float a) {
    mMoc->SetPartMultiplyColor(index, r, g, b, a);
}
void Model::GetPartScreenColor(int index, float& r, float& g, float& b, float& a) const {
    auto* part = mMoc->GetPart(index);
    if (!part)
        return;
    r = part->mScreenColor[0];
    g = part->mScreenColor[1];
    b = part->mScreenColor[2];
    a = part->mScreenColor[3];
}
void Model::GetPartMultiplyColor(int index, float& r, float& g, float& b, float& a) const {
    auto* part = mMoc->GetPart(index);
    if (!part)
        return;
    r = part->mMultiplyColor[0];
    g = part->mMultiplyColor[1];
    b = part->mMultiplyColor[2];
    a = part->mMultiplyColor[3];
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

std::vector<std::string> Model::HitIds(float x, float y, bool topOnly, bool wantDrawableId) {
    // 像素 → NDC → 完整 mvp 逆变换 → canvas 顶点坐标（含旋转/缩放/平移）
    float mx = x, my = y;
    screenToModel(&mx, &my);

    auto& drawList = mMoc->mDrawables;
    auto& nextList = mMoc->mNextListDrawIndex;
    auto& firstList = mMoc->mOrderListFirstDrawIndex;
    int range = (int)firstList.size();

    std::vector<std::string> result;

    // Iterate draw orders in reverse
    for (int order = range - 1; order >= 0; order--) {
        if ((int)firstList.size() <= order)
            continue;
        int idx = firstList[order];
        if (idx < 0 || idx >= (int)drawList.size())
            continue;
        while (true) {
            auto* dd = drawList[idx];
            if (!dd || !dd->mAvailable) {
                if (nextList.size() > (size_t)idx)
                    idx = nextList[idx];
                else
                    break;
                if (idx < 0 || idx == 65535)
                    break;
                continue;
            }

            // Check part visibility
            auto* part = mMoc->GetPart(dd->mPartsIndex);
            if (part && !part->IsVisible()) {
                if (nextList.size() > (size_t)idx)
                    idx = nextList[idx];
                else
                    break;
                if (idx < 0 || idx == 65535)
                    break;
                continue;
            }
            if (part && part->GetPartsOpacity() < 0.1f) {
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
                if (dd && dd->GetId())
                    hitId = dd->GetId()->Str();
            } else {
                if (part && part->GetId())
                    hitId = part->GetId()->Str();
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
            auto& verts =
                !dd->mTransformedPoints.empty() ? dd->mTransformedPoints : dd->mInterpolatedPoints;
            if (dd && !verts.empty()) {
                auto& indices = dd->GetIndexArray();
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
    auto ids = HitIds(x, y, topOnly, false);
    for (auto& id : ids)
        collect(collector, id.c_str());
}

void Model::CreateRenderer(int maskBufferCount) {
    (void)maskBufferCount;   // v2 固定一个裁剪缓冲
    if (mRenderer) {
        LOGW("Renderer already exists, skipping creation");
        return;
    }
#ifdef CSM_TARGET_VULKAN
    if (Common::Backend::IsVulkan()) {
        // 与 V3 一致：Vulkan 已初始化时创建 Vulkan
        // 渲染器；未初始化则跳过（无头模式安全）
        if (VulkanManager::GetInstance()->IsInitialized()) {
            mRenderer = std::make_unique<VkRenderer>(mMoc.get(), (int)mTexturePaths.size());
            SetupTextures();
            return;
        }
        LOGW("Vulkan is not initialized, skipping renderer creation");
        return;
    }
#endif
#if defined(CSM_TARGET_WIN_GL) || defined(CSM_TARGET_LINUX_GL) || defined(CSM_TARGET_MAC_GL)
    if (Common::Backend::IsOpenGL()) {
        mRenderer = std::make_unique<GLRenderer>(mMoc.get(), (int)mTexturePaths.size());
        SetupTextures();
        return;
    }
#endif
    LOGE("Graphics backend not set, skipping renderer creation");
}
void Model::DestroyRenderer() {
    if (!mRenderer) {
        LOGW("Renderer not initialized, nothing to release");
        return;
    }
    mRenderer.reset();
}

void Model::GetCanvasSize(float& w, float& h) {
    w = (float)mMoc->GetCanvasWidth();
    h = (float)mMoc->GetCanvasHeight();
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
    return mMatrixManager.GetMvp();
}

// ---- 细化更新（dt 由外部传入，与 Update(deltaSecs) 的 delta 路径同构）----

bool Model::UpdateMotion(float deltaSecs) {
    mMoc->LoadParam();
    bool updated = mMainMotionMgr->UpdateParam(mMoc.get(), deltaSecs * 1000.0f);
    mMoc->SaveParam();
    return updated;
}
void Model::UpdateDrag(float deltaSecs) {
    mDragMgr.Update(deltaSecs);
    SetDrag(mDragMgr.GetX(), mDragMgr.GetY());

    // Drag-based parameter updates (match v2 Python)
    auto addParam = [&](const char* id, float value, float weight) {
        int idx = mMoc->GetParamIndex(id);
        if (idx >= 0) {
            float cur = mMoc->GetParamValue(idx);
            mMoc->SetParamValue(idx, cur + value * weight);
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
        int idx = mMoc->GetParamIndex(id);
        if (idx >= 0) {
            float cur = mMoc->GetParamValue(idx);
            mMoc->SetParamValue(idx, cur + value * weight);
        }
    };
    float t = mBreathTimeMs / 1000.0f;
    addParam("PARAM_ANGLE_X", 15.0f * sinf(t / 6.5345f), 0.5f);
    addParam("PARAM_ANGLE_Y", 8.0f * sinf(t / 3.5345f), 0.5f);
    addParam("PARAM_ANGLE_Z", 10.0f * sinf(t / 5.5345f), 0.5f);
    addParam("PARAM_BODY_ANGLE_X", 4.0f * sinf(t / 15.5345f), 0.5f);
    int breathIdx = mMoc->GetParamIndex("PARAM_BREATH");
    if (breathIdx >= 0)
        mMoc->SetParamValue(breathIdx, 0.5f + 0.5f * sinf(t / 3.2345f));
}
void Model::UpdateBlink(float deltaSecs) {
    if (mEyeBlink)
        mEyeBlink->UpdateParam(mMoc.get(), deltaSecs * 1000.0f);
}
void Model::UpdateExpression(float deltaSecs) {
    if (!mExpressions.empty())
        mExpressionMgr->UpdateParam(mMoc.get(), deltaSecs * 1000.0f);
    // 表情 fadeout: 到期恢复上一个持久表情
    if (mFadeoutMs >= 0.0f) {
        mFadeoutElapsedMs += deltaSecs * 1000;
        if (mFadeoutElapsedMs >= mFadeoutMs) {
            mFadeoutMs = -1.0f;
            if (mLastExpression.empty()) {
                ResetExpression();
            } else {
                SetExpression(mLastExpression.c_str());
            }
        }
    }
}
void Model::UpdatePhysics(float deltaSecs) {
    if (mPhysics)
        mPhysics->UpdateParam(mMoc.get(), (long long)(deltaSecs * 1000.0f));
}
void Model::UpdatePose(float deltaSecs) {
    if (mPose)
        mPose->UpdateParam(mMoc.get(), deltaSecs);
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
    int idx = mMoc->GetParamIndex(id);
    if (idx >= 0 && idx < (int)mMoc->mSavedParamValues.size())
        mMoc->mSavedParamValues[idx] = mMoc->GetParamValue(idx);
}
void Model::AddAndSaveParameterValue(int index, float value) {
    AddParameterValue(index, value);
    if (index >= 0 && index < (int)mMoc->mSavedParamValues.size())
        mMoc->mSavedParamValues[index] = mMoc->GetParamValue(index);
}
void Model::LoadParameters() {
    mMoc->LoadParam();
}
void Model::SaveParameters() {
    mMoc->SaveParam();
}
void Model::ResetAllParameters() {
    const int count = GetParameterCount();
    for (int i = 0; i < count; i++) {
        float def = mMoc->GetParamDefault(i);
        mMoc->SetParamValue(i, def);
        if (i < (int)mMoc->mSavedParamValues.size())
            mMoc->mSavedParamValues[i] = def;
    }
}

// ---- 变换 ----

void Model::SetScaleX(float scaleX) {
    mMatrixManager.SetScaleX(scaleX);
}
void Model::SetScaleY(float scaleY) {
    mMatrixManager.SetScaleY(scaleY);
}

// ---- 动作组 ----

int Model::LoadExtraMotion(const char* group, const char* motionJsonPath) {
    auto motData = FileUtils::LoadFileAsBytes(motionJsonPath);
    if (motData.empty()) {
        LOGE("Failed to read motion file: %s", motionJsonPath);
        return -1;
    }
    auto* motion = Motion::Load(motData, mIdPool);
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
void Model::GetMotions(void* collector, void (*collect)(void* collector, const char* group, int no,
                                                        const char* file, const char* sound)) {
    for (auto& [group, infos] : mMotionInfos) {
        for (int no = 0; no < (int)infos.size(); no++) {
            collect(collector, group.c_str(), no, infos[no].file.c_str(), infos[no].sound.c_str());
        }
    }
}

// ---- 命中测试 ----

void Model::HitDrawable(float x, float y, void* collector,
                        void (*collect)(void* collector, const char* id), bool topOnly) {
    auto ids = HitIds(x, y, topOnly, true);
    for (auto& id : ids)
        collect(collector, id.c_str());
}
bool Model::IsPartHit(int index, float x, float y) {
    const char* id = GetPartId(index);
    if (id[0] == '\0')
        return false;
    auto ids = HitIds(x, y, false, false);
    for (auto& hit : ids)
        if (hit == id)
            return true;
    return false;
}
bool Model::IsDrawableHit(int index, float x, float y) {
    auto* dd = mMoc->GetDrawable(index);
    if (!dd || !dd->GetId())
        return false;
    std::string id = dd->GetId()->Str();
    auto ids = HitIds(x, y, false, true);
    for (auto& hit : ids)
        if (hit == id)
            return true;
    return false;
}

// ---- drawable ----

int Model::GetDrawableCount() {
    return (int)mMoc->mDrawables.size();
}
void Model::GetDrawableIds(void* collector, void (*collect)(void* collector, const char* id)) {
    const int count = GetDrawableCount();
    for (int i = 0; i < count; i++) {
        auto* dd = mMoc->GetDrawable(i);
        if (dd && dd->GetId())
            collect(collector, dd->GetId()->Str().c_str());
    }
}
const float* Model::GetDrawableVertices(int index) {
    auto* dd = mMoc->GetDrawable(index);
    if (!dd) {
        mDrawableVertexCache.clear();
        return mDrawableVertexCache.data();
    }
    auto& verts =
        !dd->mTransformedPoints.empty() ? dd->mTransformedPoints : dd->mInterpolatedPoints;
    mDrawableVertexCache = verts;
    return mDrawableVertexCache.data();
}
int Model::GetDrawableVertexCount(int index) {
    GetDrawableVertices(index);
    return (int)mDrawableVertexCache.size() / 2;
}
int Model::GetDrawableVertexIndexCount(int index) {
    auto* dd = mMoc->GetDrawable(index);
    if (!dd)
        return 0;
    return (int)dd->GetIndexArray().size();
}
const unsigned short* Model::GetDrawableIndices(int index) {
    auto* dd = mMoc->GetDrawable(index);
    if (!dd) {
        mDrawableIndexCache.clear();
        return mDrawableIndexCache.data();
    }
    auto& indices = dd->GetIndexArray();
    mDrawableIndexCache.assign(indices.begin(), indices.end());
    return mDrawableIndexCache.data();
}
void Model::SetDrawableMultiColor(int index, float r, float g, float b, float a) {
    // drawable 级颜色覆盖，仅影响本 drawable（语义对齐 V3 per-drawable override）
    auto* dd = mMoc->GetDrawable(index);
    if (!dd)
        return;
    dd->SetOverrideMultiplyColor(r, g, b, a);
}
void Model::SetDrawableScreenColor(int index, float r, float g, float b, float a) {
    // drawable 级颜色覆盖，仅影响本 drawable（语义对齐 V3 per-drawable override）
    auto* dd = mMoc->GetDrawable(index);
    if (!dd)
        return;
    dd->SetOverrideScreenColor(r, g, b, a);
}

// ---- 表情 ----

void Model::AddExpression(const char* expressionId) {
    // v2 只有一个表情管理器（无叠加），等价于 SetExpression
    SetExpression(expressionId);
}
void Model::RemoveExpression(const char* expressionId) {
    (void)expressionId;
    // v2 只有一个表情管理器，无法移除单个表情，停止全部
    mExpressionMgr->StopAllMotions();
}
void Model::ResetExpressions() {
    mFadeoutMs = -1.0f;
    mFadeoutElapsedMs = 0;
    mLastExpression.clear();
    mExpressionMgr->StopAllMotions();
}
void Model::GetExpressions(void* collector,
                           void (*collect)(void* collector, const char* id, const char* file)) {
    for (auto& [id, file] : mExpressionFiles) {
        collect(collector, id.c_str(), file.c_str());
    }
}
void Model::LoadExtraExpression(const char* expressionId, const char* expressionJsonPath) {
    auto expData = FileUtils::LoadFileAsBytes(expressionJsonPath);
    if (expData.empty()) {
        LOGE("Failed to read expression file: %s", expressionJsonPath);
        return;
    }
    auto* expr = ExpressionMotion::Load(expData, mIdPool);
    if (!expr) {
        LOGE("Failed to load expression: %s", expressionJsonPath);
        return;
    }
    mExpressions[expressionId].reset(expr);
    mExpressionFiles[expressionId] = expressionJsonPath;
    LOGI("Load extra expression: %s => [%s]", expressionJsonPath, expressionId);
}

bool Model::HasMocConsistencyFromFile(const char*) {
    // v2 的 .moc 二进制格式没有 moc3 的一致性检查概念
    return false;
}
void Model::SetupTextures() {
#ifdef CSM_TARGET_VULKAN
    if (Common::Backend::IsVulkan()) {
        auto vkRenderer = mRenderer->GetVkRenderer();
        for (size_t i = 0; i < mTexturePaths.size(); i++) {
            std::string texPath = JoinPath(mModelHomeDir, mTexturePaths[i]);
            int w, h, n;
            auto texData = FileUtils::LoadFileAsBytes(texPath);
            unsigned char* pixels =
                texData.empty()
                    ? nullptr
                    : stbi_load_from_memory(texData.data(), (int)texData.size(), &w, &h, &n, 4);
            if (!pixels)
                continue;
            LOGD(
                "Load texture[%zu/%zu]: %s", i + 1, mTexturePaths.size(), mTexturePaths[i].c_str());

            VkImage image = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            VkImageView view = VK_NULL_HANDLE;
            VkSampler sampler = VK_NULL_HANDLE;
            if (VkUtil::CreateTextureFromPixels(VulkanManager::GetInstance()->GetDevice(),
                                                VulkanManager::GetInstance()->GetPhysicalDevice(),
                                                pixels,
                                                w,
                                                h,
                                                &image,
                                                &memory,
                                                &view,
                                                &sampler,
                                                1.0f)) {
                vkRenderer->SetTexture((int)i, image, memory, view, sampler);
            } else {
                LOGE("Failed to create Vulkan texture: %s", texPath.c_str());
            }
            stbi_image_free(pixels);
        }
        return;
    }
#endif

#if defined(CSM_TARGET_WIN_GL) || defined(CSM_TARGET_LINUX_GL) || defined(CSM_TARGET_MAC_GL)
    if (Common::Backend::IsOpenGL()) {
        for (size_t i = 0; i < mTexturePaths.size(); i++) {
            std::string texPath = JoinPath(mModelHomeDir, mTexturePaths[i]);
            int w, h, n;
            auto texData = FileUtils::LoadFileAsBytes(texPath);
            unsigned char* pixels =
                texData.empty()
                    ? nullptr
                    : stbi_load_from_memory(texData.data(), (int)texData.size(), &w, &h, &n, 4);
            if (!pixels)
                continue;
            LOGD(
                "Load texture[%zu/%zu]: %s", i + 1, mTexturePaths.size(), mTexturePaths[i].c_str());

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
            static_cast<GLRenderer*>(mRenderer.get())->SetTexture((int)i, (int)texId);
            stbi_image_free(pixels);
        }
    }
#endif
}
}   // namespace V2
}   // namespace Live2D
