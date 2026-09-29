#include "V3/Model.hpp"
#include "IModel.hpp"
#include "Motion/ACubismMotion.hpp"

#include <CubismDefaultParameterId.hpp>
#include <CubismModelSettingJson.hpp>
#include <Id/CubismIdManager.hpp>
#include <Live2DCubismCore.hpp>
#include <Model/CubismMoc.hpp>
#include <Motion/CubismMotion.hpp>
#include <Rendering/OpenGL/CubismShader_OpenGLES2.hpp>
#include <Utils/CubismString.hpp>


#include <LAppPal.hpp>
#include <Log.hpp>


#include <algorithm>
#include <cstring>
#include <filesystem>
#include <functional>
#include <unordered_set>


using namespace Live2D::Cubism::Framework;
using namespace Live2D::Cubism::Framework::DefaultParameterId;
using namespace Live2D::Cubism::Core;
using namespace Live2D::Common::Log;

namespace Live2D {
namespace V3 {
namespace {
void LoadAssets(const std::string& filePath,
                const std::function<void(csmByte*, csmSizeInt)>& afterLoadCallback) {
    csmSizeInt bufferSize = 0;
    csmByte* buffer = nullptr;

    if (filePath.empty()) {
        return;
    }

    buffer = LAppPal::LoadFileAsBytes(filePath.c_str(), &bufferSize);

    afterLoadCallback(buffer, bufferSize);

    LAppPal::ReleaseBytes(buffer);
}

// root_path → 资源根目录（保证以 '/' 结尾；空串原样返回 = 相对当前工作目录）
std::string NormalizeModelHomeDir(const char* rootPath) {
    std::string dir(rootPath ? rootPath : "");
    if (!dir.empty() && dir.back() != '/' && dir.back() != '\\') {
        dir += '/';
    }
    return dir;
}

// homeDir 已含尾部 '/'；rel 为绝对路径（含 '/' '\' 开头）时原样返回
csmString JoinPath(const csmString& homeDir, const char* rel) {
    if (rel == nullptr || rel[0] == '\0')
        return csmString(rel ? rel : "");
    std::filesystem::path p = std::filesystem::u8path(rel);
    if (p.is_absolute() || rel[0] == '/' || rel[0] == '\\')
        return csmString(rel);
    return homeDir + rel;
}
}   // namespace

Model::Model()
    : mModelSetting(nullptr)
    , mMatrixManager()
    , mParameterCount(0)
    , mParameterDefaultValues(nullptr)
    , mParameterValues(nullptr)
    , mTmpOrderedDrawIndice(nullptr)
    , autoBlink(true)
    , autoBreath(true) {
    mProxy._mocConsistency = true;

    mIdParamAngleX = CubismFramework::GetIdManager()->GetId(ParamAngleX);
    mIdParamAngleY = CubismFramework::GetIdManager()->GetId(ParamAngleY);
    mIdParamAngleZ = CubismFramework::GetIdManager()->GetId(ParamAngleZ);
    mIdParamBodyAngleX = CubismFramework::GetIdManager()->GetId(ParamBodyAngleX);
    mIdParamEyeBallX = CubismFramework::GetIdManager()->GetId(ParamEyeBallX);
    mIdParamEyeBallY = CubismFramework::GetIdManager()->GetId(ParamEyeBallY);
}

Model::~Model() {
    mTextureManager.ReleaseTextures();

    ReleaseMotions();
    ReleaseExpressions();
    ReleaseExpressionManagers();

    if (mModelSetting == nullptr) {
        return;
    }

    delete mModelSetting;
}

void Model::LoadModelJson(const char* filePath, bool createRenderer) {
    std::filesystem::path p = std::filesystem::u8path(filePath);
    mModelHomeDir = p.parent_path().generic_u8string().c_str();
    mModelHomeDir += "/";

    LOGD("load modelSetting: %s", filePath);
    LoadAssets(filePath, [&](csmByte* buffer, csmSizeInt size) {
        mModelSetting = new CubismModelSettingJson(buffer, size);
    });

    SetupModel();

    if (createRenderer) {
        CreateRenderer(1);
    }
}

void Model::LoadFromJsonString(const char* jsonData, bool createRenderer, const char* rootPath) {
    if (mModelSetting != nullptr) {
        LOGE("model already loaded");
        return;
    }
    if (jsonData == nullptr || jsonData[0] == '\0') {
        LOGE("model json string is empty");
        return;
    }

    const std::string homeDir = NormalizeModelHomeDir(rootPath);
    mModelHomeDir = homeDir.c_str();   // csmString::operator= 深拷贝，homeDir 可安全析构
    LOGD("load modelSetting from json string (home: %s)", mModelHomeDir.GetRawString());

    auto* setting = new CubismModelSettingJson(reinterpret_cast<const csmByte*>(jsonData),
                                               static_cast<csmSizeInt>(strlen(jsonData)));
    if (!setting->IsValid()) {
        // 解析失败时 _jsonValue 为空，任何 accessor 都会越界 → 必须在这里拦下
        LOGE("Failed to parse model json string");
        delete setting;
        return;
    }
    mModelSetting = setting;

    SetupModel();

    if (createRenderer) {
        CreateRenderer(1);
    }
}

const char* Model::GetModelHomeDir() {
    return mModelHomeDir.GetRawString();
}
int Model::Version() const {
    return 3;
}

void Model::Update(float deltaSecs) {
    if (deltaSecs < 0.0f) {
        // 哨兵: 未传入 delta，内部自计时（clamp 0.1）
        auto now = std::chrono::steady_clock::now();
        if (mLastUpdatePoint.time_since_epoch().count() != 0) {
            deltaSecs =
                (float)std::min(std::chrono::duration<double>(now - mLastUpdatePoint).count(), 0.1);
        } else {
            deltaSecs = 0.016f;
        }
        mLastUpdatePoint = now;
    }

    mProxy._dragManager->Update(deltaSecs);
    mDragX = mProxy._dragManager->GetX();
    mDragY = mProxy._dragManager->GetY();

    bool motionUpdated = false;
    LoadParameters();
    if (!mProxy._motionManager->IsFinished()) {
        motionUpdated = mProxy._motionManager->UpdateMotion(mProxy.GetModel(), deltaSecs);
    }
    SaveParameters();

    mProxy.SetOpacity(mProxy->GetModelOpacity());

    if (!motionUpdated) {
        if (mProxy._eyeBlink != NULL && autoBlink) {
            mProxy._eyeBlink->UpdateParameters(mProxy.GetModel(), deltaSecs);
        }
    }

    UpdateExpression(deltaSecs);

    mProxy->AddParameterValue(mParamAngleXi, mDragX * 30);
    mProxy->AddParameterValue(mParamAngleYi, mDragY * 30);
    mProxy->AddParameterValue(mParamAngleZi, mDragX * mDragY * -30);

    mProxy->AddParameterValue(mParamBodyAngleXi, mDragX * 10);

    mProxy->AddParameterValue(mParamEyeBallXi, mDragX);
    mProxy->AddParameterValue(mParamEyeBallYi, mDragY);

    if (mProxy._breath != NULL && autoBreath) {
        mProxy._breath->UpdateParameters(mProxy.GetModel(), deltaSecs);
    }

    if (mProxy._physics != NULL) {
        mProxy._physics->Evaluate(mProxy.GetModel(), deltaSecs);
    }

    if (mProxy._pose != NULL) {
        mProxy._pose->UpdateParameters(mProxy.GetModel(), deltaSecs);
    }
}

void Model::SetupModel() {
    // moc3
    if (strcmp(mModelSetting->GetModelFileName(), "") != 0) {
        csmString path = mModelSetting->GetModelFileName();
        path = JoinPath(mModelHomeDir, path.GetRawString());

        LoadAssets(path.GetRawString(), [&](csmByte* buffer, csmSizeInt size) {
            mProxy.LoadModel(buffer, size, mProxy._mocConsistency);
        });
        LOGI("Load model: %s", path.GetRawString());
    }

    if (mProxy.GetModel() == nullptr) {
        LOGE("Failed to SetupModel()");
    }

    // exp3.json
    if (mModelSetting->GetExpressionCount() > 0) {
        const csmInt32 count = mModelSetting->GetExpressionCount();
        for (csmInt32 i = 0; i < count; i++) {
            csmString name = mModelSetting->GetExpressionName(i);
            csmString path = JoinPath(mModelHomeDir, mModelSetting->GetExpressionFileName(i));

            LoadAssets(path.GetRawString(), [&](csmByte* buffer, csmSizeInt size) {
                ACubismMotion* motion = mProxy.LoadExpression(buffer, size, name.GetRawString());
                if (motion) {
                    std::string key = name.GetRawString();
                    if (mExpressions[name] != nullptr) {
                        ACubismMotion::Delete(mExpressions[name]);
                        mExpressions[name] = nullptr;
                    }
                    if (mExpManagers[key] != nullptr) {
                        CSM_DELETE(mExpManagers[key]);
                        mExpManagers.erase(key);
                    }
                    mExpressions[name] = motion;
                    mExpManagers[key] = CSM_NEW CubismExpressionMotionManager();
                }
            });
        }
    }

    // physics3.json
    if (strcmp(mModelSetting->GetPhysicsFileName(), "") != 0) {
        csmString path = JoinPath(mModelHomeDir, mModelSetting->GetPhysicsFileName());

        LoadAssets(path.GetRawString(),
                   [&](csmByte* buffer, csmSizeInt size) { mProxy.LoadPhysics(buffer, size); });
    }

    // pose3.json
    if (strcmp(mModelSetting->GetPoseFileName(), "") != 0) {
        csmString path = JoinPath(mModelHomeDir, mModelSetting->GetPoseFileName());

        LoadAssets(path.GetRawString(),
                   [&](csmByte* buffer, csmSizeInt size) { mProxy.LoadPose(buffer, size); });
    }

    // EyeBlink
    if (mModelSetting->GetEyeBlinkParameterCount() > 0) {
        mProxy._eyeBlink = CubismEyeBlink::Create(mModelSetting);
    }

    // Breath
    {
        mProxy._breath = CubismBreath::Create();

        csmVector<CubismBreath::BreathParameterData> breathParameters;

        breathParameters.PushBack(
            CubismBreath::BreathParameterData(mIdParamAngleX, 0.0f, 15.0f, 6.5345f, 0.5f));
        breathParameters.PushBack(
            CubismBreath::BreathParameterData(mIdParamAngleY, 0.0f, 8.0f, 3.5345f, 0.5f));
        breathParameters.PushBack(
            CubismBreath::BreathParameterData(mIdParamAngleZ, 0.0f, 10.0f, 5.5345f, 0.5f));
        breathParameters.PushBack(
            CubismBreath::BreathParameterData(mIdParamBodyAngleX, 0.0f, 4.0f, 15.5345f, 0.5f));
        breathParameters.PushBack(CubismBreath::BreathParameterData(
            CubismFramework::GetIdManager()->GetId(ParamBreath), 0.5f, 0.5f, 3.2345f, 0.5f));

        mProxy._breath->SetParameters(breathParameters);
    }

    // UserData
    if (strcmp(mModelSetting->GetUserDataFile(), "") != 0) {
        csmString path = JoinPath(mModelHomeDir, mModelSetting->GetUserDataFile());
        LoadAssets(path.GetRawString(),
                   [&](csmByte* buffer, csmSizeInt size) { mProxy.LoadUserData(buffer, size); });
    }

    // EyeBlinkIds
    {
        csmInt32 eyeBlinkIdCount = mModelSetting->GetEyeBlinkParameterCount();
        for (csmInt32 i = 0; i < eyeBlinkIdCount; ++i) {
            mEyeBlinkIds.PushBack(mModelSetting->GetEyeBlinkParameterId(i));
        }
    }

    // LipSyncIds
    {
        csmInt32 lipSyncIdCount = mModelSetting->GetLipSyncParameterCount();
        for (csmInt32 i = 0; i < lipSyncIdCount; ++i) {
            mLipSyncIds.PushBack(mModelSetting->GetLipSyncParameterId(i));
        }
    }

    if (mModelSetting == nullptr || mProxy.GetModelMatrix() == nullptr) {
        LOGE("Failed to SetupModel()");
        return;
    }

    // Layout
    csmMap<csmString, csmFloat32> layout;
    mModelSetting->GetLayoutMap(layout);
    mProxy.GetModelMatrix()->SetupFromLayout(layout);

    // motion3.json
    mMotionGroupNames.clear();
    mMotionCounts.clear();
    for (csmInt32 i = 0; i < mModelSetting->GetMotionGroupCount(); i++) {
        const csmChar* group = mModelSetting->GetMotionGroupName(i);
        PreloadMotionGroup(group);
    }
    mProxy._motionManager->StopAllMotions();
    mMatrixManager.SetModelWH(mProxy->GetCanvasWidth(), mProxy->GetCanvasHeight());
    mParamAngleXi = mProxy->GetParameterIndex(mIdParamAngleX);
    mParamAngleYi = mProxy->GetParameterIndex(mIdParamAngleY);
    mParamAngleZi = mProxy->GetParameterIndex(mIdParamAngleZ);
    mParamBodyAngleXi = mProxy->GetParameterIndex(mIdParamBodyAngleX);
    mParamEyeBallXi = mProxy->GetParameterIndex(mIdParamEyeBallX);
    mParamEyeBallYi = mProxy->GetParameterIndex(mIdParamEyeBallY);
    mTmpOrderedDrawIndice = new int[mProxy->GetDrawableCount()];
    csmModel* model = mProxy->GetModel();
    mParameterDefaultValues = csmGetParameterDefaultValues(model);
    mParameterValues = csmGetParameterValues(model);
    mParameterCount = csmGetParameterCount(model);
    mSavedParameterValues.resize(mParameterCount);
    SaveParameters();
    LOGD("Model setup complete");
}

bool Model::UpdateMotion(float deltaSecs) {
    mProxy.SetOpacity(mProxy->GetModelOpacity());
    return !mProxy._motionManager->IsFinished() &&
           mProxy._motionManager->UpdateMotion(mProxy.GetModel(), deltaSecs);
}

void Model::UpdateDrag(float deltaSecs) {
    mProxy._dragManager->Update(deltaSecs);
    mDragX = mProxy._dragManager->GetX();
    mDragY = mProxy._dragManager->GetY();

    mProxy->AddParameterValue(mParamAngleXi, mDragX * 30);
    mProxy->AddParameterValue(mParamAngleYi, mDragY * 30);
    mProxy->AddParameterValue(mParamAngleZi, mDragX * mDragY * -30);

    mProxy->AddParameterValue(mParamBodyAngleXi, mDragX * 10);

    mProxy->AddParameterValue(mParamEyeBallXi, mDragX);
    mProxy->AddParameterValue(mParamEyeBallYi, mDragY);
}

void Model::UpdateBreath(float deltaSecs) {
    if (mProxy._breath == nullptr) {
        return;
    }
    mProxy._breath->UpdateParameters(mProxy.GetModel(), deltaSecs);
}

void Model::UpdateBlink(float deltaSecs) {
    if (mProxy._eyeBlink == nullptr) {
        return;
    }
    mProxy._eyeBlink->UpdateParameters(mProxy.GetModel(), deltaSecs);
}

void Model::UpdateExpression(float deltaSecs) {
    if (mProxy._expressionManager->IsFinished()) {
        for (auto& pair : mExpManagers) {
            pair.second->UpdateMotion(mProxy.GetModel(), deltaSecs);
        }
    } else {
        mProxy._expressionManager->UpdateMotion(mProxy.GetModel(), deltaSecs);
    }

    ResumeLastExpressionIfNeeded(deltaSecs);
}

void Model::UpdatePhysics(float deltaSecs) {
    if (mProxy._physics == nullptr) {
        return;
    }

    mProxy._physics->Evaluate(mProxy.GetModel(), deltaSecs);
}

void Model::UpdatePose(float deltaSecs) {
    if (mProxy._pose == nullptr) {
        return;
    }

    mProxy._pose->UpdateParameters(mProxy.GetModel(), deltaSecs);
}

int Model::GetParameterCount() {
    return mProxy->GetParameterCount();
}

void Model::GetParameterIds(void* collector, void (*collect)(void* collector, const char* id)) {
    for (csmInt32 i = 0; i < mParameterCount; ++i) {
        collect(collector, mProxy->GetParameterId(i)->GetString().GetRawString());
    }
}

const char* Model::GetParameterId(int index) {
    if (index < 0 || index >= mParameterCount) {
        return "";
    }
    return mProxy->GetParameterId(index)->GetString().GetRawString();
}

float Model::GetParameterValue(int index) {
    return mProxy->GetParameterValue(index);
}

float Model::GetParameterMaximumValue(int index) {
    return mProxy->GetParameterMaximumValue(index);
}

float Model::GetParameterMinimumValue(int index) {
    return mProxy->GetParameterMinimumValue(index);
}

float Model::GetParameterDefaultValue(int index) {
    return mProxy->GetParameterDefaultValue(index);
}

void Model::SetParameterValue(const char* id, float value, float weight) {
    const CubismId* handle = CubismFramework::GetIdManager()->GetId(id);
    mProxy->SetParameterValue(handle, value, weight);
}

void Model::SetParameterValue(int index, float value, float weight) {
    mProxy->SetParameterValue(index, value, weight);
}

void Model::AddParameterValue(const char* id, float value) {
    const CubismId* handle = CubismFramework::GetIdManager()->GetId(id);
    mProxy->AddParameterValue(handle, value);
}

void Model::AddParameterValue(int index, float value) {
    mProxy->AddParameterValue(index, value);
}

void Model::SetAndSaveParameterValue(const char* id, float value, float weight) {
    const CubismId* handle = CubismFramework::GetIdManager()->GetId(id);
    const int index = mProxy->GetParameterIndex(handle);
    mProxy->SetParameterValue(index, value, weight);
    if (index < mParameterCount) {
        mSavedParameterValues[index] = mParameterValues[index];
    }
}

void Model::SetAndSaveParameterValue(int index, float value, float weight) {
    mProxy->SetParameterValue(index, value, weight);
    if (index < mParameterCount) {
        mSavedParameterValues[index] = mParameterValues[index];
    }
}

void Model::AddAndSaveParameterValue(const char* id, float value) {
    const CubismId* handle = CubismFramework::GetIdManager()->GetId(id);
    const int index = mProxy->GetParameterIndex(handle);
    mProxy->AddParameterValue(index, value);
    if (index < mParameterCount) {
        mSavedParameterValues[index] = mParameterValues[index];
    }
}

void Model::AddAndSaveParameterValue(int index, float value) {
    mProxy->AddParameterValue(index, value);
    if (index < mParameterCount) {
        mSavedParameterValues[index] = mParameterValues[index];
    }
}

void Model::LoadParameters() {
    for (int i = 0; i < mParameterCount; ++i) {
        mProxy->SetParameterValue(i, mSavedParameterValues[i]);
    }
}

void Model::SaveParameters() {
    for (int i = 0; i < mParameterCount; ++i) {
        mSavedParameterValues[i] = mParameterValues[i];
    }
}

void Model::Resize(int width, int height) {
    mMatrixManager.UpdateScreenToScene(width, height);
    auto renderer = mProxy.GetRenderer<Rendering::CubismRenderer_OpenGLES2>();
    if (renderer) {
        renderer->SetRenderTargetSize(width, height);
    }
}

void Model::SetOffset(float x, float y) {
    mOffsetX = x;
    mOffsetY = y;
    mMatrixManager.SetOffset(x, y);
}

void Model::SetOffsetX(float x) {
    mOffsetX = x;
    mMatrixManager.SetOffset(mOffsetX, mOffsetY);
}

void Model::SetOffsetY(float y) {
    mOffsetY = y;
    mMatrixManager.SetOffset(mOffsetX, mOffsetY);
}

void Model::Rotate(float angle) {
    mMatrixManager.Rotate(angle);
}

void Model::SetScale(float scale) {
    mMatrixManager.SetScaleX(scale);
    mMatrixManager.SetScaleY(scale);
}

void Model::SetScaleX(float scale) {
    mMatrixManager.SetScaleX(scale);
}

void Model::SetScaleY(float scale) {
    mMatrixManager.SetScaleY(scale);
}

const float* Model::GetMvp() {
    return mMatrixManager.GetMvp().GetArray();
}

namespace {
// std::function 回调 <-> ACubismMotion void*/C 风格回调 的桥接
struct MotionCallbackData {
    IModel::MotionCallback onStart;
    IModel::MotionCallback onFinish;
    std::string group;
    int no;
};

void MotionBeganHandler(ACubismMotion* motion) {
    auto* data = static_cast<MotionCallbackData*>(motion->GetBeganMotionCustomData());
    if (data == nullptr) {
        return;
    }
    if (data->onStart) {
        data->onStart(data->group, data->no);
    }
}

void MotionFinishedHandler(ACubismMotion* motion) {
    auto* data = static_cast<MotionCallbackData*>(motion->GetFinishedMotionCustomData());
    if (data == nullptr) {
        return;
    }
    if (data->onFinish) {
        data->onFinish(data->group, data->no);
    }
    delete data;
}
}   // namespace

void Model::StartMotion(const std::string& group, int no, int priority, MotionCallback onStart,
                        MotionCallback onFinish) {
    if (priority == MotionPriority::Force) {
        mProxy._motionManager->SetReservePriority(priority);
    } else if (!mProxy._motionManager->ReserveMotion(priority)) {
        LOGI("motion priority is too low.");
        return;
    }

    // ex) idle_0
    csmString name = Utils::CubismString::GetFormatedString("%s_%d", group.c_str(), no);
    CubismMotion* motion = static_cast<CubismMotion*>(mMotions[name.GetRawString()]);
    csmBool autoDelete = false;

    csmBool hasMotion = true;

    if (motion == NULL) {
        // 加载临时 motion
        const csmString fileName = mModelSetting->GetMotionFileName(group.c_str(), no);
        if (fileName.GetLength() <= 0) {
            hasMotion = false;
            LOGI("motion(%s) has no file attached", name.GetRawString());
            goto handler_label;
        }

        csmString path = fileName;

        path = JoinPath(mModelHomeDir, path.GetRawString());

        LoadAssets(path.GetRawString(), [&](csmByte* buffer, csmSizeInt size) {
            motion = static_cast<CubismMotion*>(mProxy.LoadMotion(buffer, size, NULL));

            if (motion) {
                csmFloat32 fadeTime = mModelSetting->GetMotionFadeInTimeValue(group.c_str(), no);
                if (fadeTime >= 0.0f) {
                    motion->SetFadeInTime(fadeTime);
                }

                fadeTime = mModelSetting->GetMotionFadeOutTimeValue(group.c_str(), no);
                if (fadeTime >= 0.0f) {
                    motion->SetFadeOutTime(fadeTime);
                }
                motion->SetEffectIds(mEyeBlinkIds, mLipSyncIds);
                autoDelete = true;   // 終了時にメモリから削除
            }
        });
        LOGI("load tmp motion(%s)", name.GetRawString());
    }

    if (motion) {
        auto* data = new MotionCallbackData{std::move(onStart), std::move(onFinish), group, no};
        motion->SetBeganMotionCustomData(data);
        motion->SetFinishedMotionCustomData(data);
        motion->SetBeganMotionHandler(MotionBeganHandler);
        motion->SetFinishedMotionHandler(MotionFinishedHandler);
    }

handler_label:

    if (!hasMotion) {
        // 添加空指针判断，如果 motion 文件不存在，直接调用动作结束回调函数
        // 修复模型文件不存在时，导致崩溃
        if (onStart) {
            onStart(group, no);
        }
        if (onFinish) {
            onFinish(group, no);
        }
        mProxy._motionManager->SetReservePriority(MotionPriority::None);
    }

    mProxy._motionManager->StartMotionPriority(motion, autoDelete, priority);
}

void Model::StartRandomMotion(const std::string& group, int priority, MotionCallback onStart,
                              MotionCallback onFinish) {
    csmString g;
    int gindex = -1;
    if (group.empty()) {
        int gcnt = mMotionGroupNames.size();
        if (gcnt > 0) {
            gindex = rand() % gcnt;
            g = mMotionGroupNames[gindex];
        }
    } else {
        g = group.c_str();
        for (csmInt32 i = 0; i < mMotionGroupNames.size(); i++) {
            if (mMotionGroupNames[i] == g) {
                gindex = i;
                break;
            }
        }
    }

    if (gindex < 0) {
        LOGI("MotionGroup [%s] not found", g.GetRawString());
        return;
    }

    csmInt32 no = rand() % mMotionCounts[gindex];

    StartMotion(g.GetRawString(), no, priority, std::move(onStart), std::move(onFinish));
}

bool Model::IsMotionFinished() {
    return mProxy._motionManager->IsFinished();
}

int Model::LoadExtraMotion(const char* group, const char* motionJsonPath) {
    int no = -1;
    LoadAssets(motionJsonPath, [&](csmByte* buffer, csmSizeInt size) {
        int i = 0;
        bool found = false;
        for (auto& s : mMotionGroupNames) {
            if (s == group) {
                found = true;
                break;
            }
            i++;
        }
        no = found ? mMotionCounts[i] : 0;

        const csmString name = Utils::CubismString::GetFormatedString("%s_%d", group, no);

        CubismMotion* tmpMotion = static_cast<CubismMotion*>(mProxy.LoadMotion(
            buffer, size, name.GetRawString(), NULL, NULL, mModelSetting, group, no));

        if (tmpMotion) {
            tmpMotion->SetEffectIds(mEyeBlinkIds, mLipSyncIds);

            mMotions[name] = tmpMotion;

            LOGI("Load extra motion: %s => [%s]", motionJsonPath, name.GetRawString());

            if (!found) {
                mMotionGroupNames.push_back(group);
                mMotionCounts.push_back(1);
            } else {
                mMotionCounts[i]++;
            }
        } else {
            LOGW("Load extra motion failed: %s", motionJsonPath);
        }
    });

    return no;
}

int Model::GetMotionGroupCount() {
    return mModelSetting->GetMotionGroupCount();
}

int Model::GetMotionCount(const char* group) {
    return mModelSetting->GetMotionCount(group);
}

const char* Model::GetMotionSound(const char* group, int no) {
    const char* sound = mModelSetting->GetMotionSoundFileName(group, no);
    return sound ? sound : "";
}

void Model::GetMotions(void* collector, void (*collect)(void* collector, const char* group, int no,
                                                        const char* file, const char* sound)) {
    const int count = mModelSetting->GetMotionGroupCount();
    for (int i = 0; i < count; i++) {
        const char* group = mModelSetting->GetMotionGroupName(i);
        const int motionCount = mModelSetting->GetMotionCount(group);
        for (int j = 0; j < motionCount; j++) {
            const char* file = mModelSetting->GetMotionFileName(group, j);
            const char* sound = mModelSetting->GetMotionSoundFileName(group, j);
            collect(collector, group, j, file, sound);
        }
    }
}

static bool isInTriangle(const csmVector2 p0, const csmVector2 p1, const csmVector2 p2,
                         const csmVector2 p) {
    // https://github.com/Arkueid/live2d-py/issues/18
    // 情况1：
    //  要检测的三角形很多，说明模型很精细，那么一定程度上三角形的面积会很小，
    //  只有少量的三角形的范围检测会失败，增加的额外计算量不会太大，
    //  因此只需要简单判断范围即可回避大量浮点计算
    // 情况2：
    //  要检测的三角形比较少，说明模型很粗糙，那么总的计算量就会相对较少，
    //  增加几次范围检测理论上是可以接受的
    // 总结为：需要计算叉积的实际三角形其实不会很多，因此范围检测可以避免大部分计算

    // 范围检测
    if (p.X < std::min({p0.X, p1.X, p2.X})) {
        return false;
    }
    if (p.X > std::max({p0.X, p1.X, p2.X})) {
        return false;
    }
    if (p.Y < std::min({p0.Y, p1.Y, p2.Y})) {
        return false;
    }
    if (p.Y > std::max({p0.Y, p1.Y, p2.Y})) {
        return false;
    }

    // 叉积检测
    const float dX = p.X - p2.X;
    const float dY = p.Y - p2.Y;
    const float dX21 = p2.X - p1.X;
    const float dY12 = p1.Y - p2.Y;
    const float D = dY12 * (p0.X - p2.X) + dX21 * (p0.Y - p2.Y);
    const float s = dY12 * dX + dX21 * dY;
    const float t = (p2.Y - p0.Y) * dX + (p0.X - p2.X) * dY;
    if (D < 0)
        return s <= 0 && t <= 0 && s + t >= D;
    return s >= 0 && t >= 0 && s + t <= D;
}

void Model::HitPart(float x, float y, void* collector,
                    void (*collect)(void* collector, const char* id), bool topOnly) {
    mMatrixManager.ScreenToScene(&x, &y);
    mMatrixManager.InvertTransform(&x, &y);
    const csmInt32 drawableCount = mProxy->GetDrawableCount();
    const csmInt32* renderOrders = GetDrawableRenderOrders();
    for (csmInt32 i = 0; i < drawableCount; i++) {
        // 绘制顺序，先绘制的被后绘制的覆盖
        mTmpOrderedDrawIndice[drawableCount - 1 - renderOrders[i]] = i;
    }
    // 多个 part index 可能指向同一个 part id，所以用 part id set
    std::unordered_set<const char*> hitParts;
    bool topClicked = false;

    for (int i = 0; i < drawableCount; i++) {
        int drawableIndex = mTmpOrderedDrawIndice[i];
        if (mProxy->GetDrawableOpacity(drawableIndex) == 0.0f) {
            continue;
        }
        int partIndex = mProxy->GetDrawableParentPartIndex(drawableIndex);
        if (partIndex == -1) {
            // 绘制对象不属于 part
            continue;
        }
        const char* partId = mProxy->GetPartId(partIndex)->GetString().GetRawString();
        if (mProxy->GetPartOpacity(partIndex) == 0.0f) {
            continue;
        }
        // 已经点击过的部件
        if (hitParts.find(partId) != hitParts.end()) {
            continue;
        }
        // 顶点连线个数，3个顶点一个三角形，一定是3的整数倍
        const int indexCount = mProxy->GetDrawableVertexIndexCount(drawableIndex);
        // 顶点坐标
        const csmVector2* vertices = mProxy->GetDrawableVertexPositions(drawableIndex);
        // 三角形顶点索引
        const csmUint16* indices = mProxy->GetDrawableVertexIndices(drawableIndex);
        const int triangleCount = indexCount / 3;

        for (int j = 0; j < triangleCount; j++) {
            if (!isInTriangle(vertices[indices[j * 3]],
                              vertices[indices[j * 3 + 1]],
                              vertices[indices[j * 3 + 2]],
                              {x, y})) {
                continue;
            }
            collect(collector, partId);
            hitParts.emplace(partId);
            topClicked = true;
            break;
        }

        if (topOnly && topClicked) {
            break;
        }
    }
}

void Model::HitDrawable(float x, float y, void* collector,
                        void (*collect)(void* collector, const char* id), bool topOnly) {
    mMatrixManager.ScreenToScene(&x, &y);
    mMatrixManager.InvertTransform(&x, &y);

    const csmInt32 drawableCount = mProxy->GetDrawableCount();
    const csmInt32* renderOrders = GetDrawableRenderOrders();
    for (csmInt32 i = 0; i < drawableCount; i++) {
        // 绘制顺序，先绘制的被后绘制的覆盖
        mTmpOrderedDrawIndice[drawableCount - 1 - renderOrders[i]] = i;
    }
    bool topClicked = false;

    for (int i = 0; i < drawableCount; i++) {
        int drawableIndex = mTmpOrderedDrawIndice[i];
        if (mProxy->GetDrawableOpacity(drawableIndex) == 0.0f) {
            continue;
        }
        const char* drawableId = mProxy->GetDrawableId(drawableIndex)->GetString().GetRawString();

        // 顶点连线个数，3个顶点一个三角形，一定是3的整数倍
        const int indexCount = mProxy->GetDrawableVertexIndexCount(drawableIndex);
        // 顶点坐标
        const csmVector2* vertices = mProxy->GetDrawableVertexPositions(drawableIndex);
        // 三角形顶点索引
        const csmUint16* indices = mProxy->GetDrawableVertexIndices(drawableIndex);
        const int triangleCount = indexCount / 3;

        for (int j = 0; j < triangleCount; j++) {
            if (!isInTriangle(vertices[indices[j * 3]],
                              vertices[indices[j * 3 + 1]],
                              vertices[indices[j * 3 + 2]],
                              {x, y})) {
                continue;
            }
            collect(collector, drawableId);
            topClicked = true;
            break;
        }

        if (topOnly && topClicked) {
            break;
        }
    }
}

bool Model::IsAreaHit(const char* areaName, float x, float y) {
    mMatrixManager.ScreenToScene(&x, &y);
    mMatrixManager.InvertTransform(&x, &y);

    if (mProxy.GetOpacity() < 1) {
        return false;
    }
    const csmInt32 count = mModelSetting->GetHitAreasCount();
    for (csmInt32 i = 0; i < count; i++) {
        if (strcmp(mModelSetting->GetHitAreaName(i), areaName) == 0) {
            const CubismIdHandle drawID = mModelSetting->GetHitAreaId(i);
            return mProxy.IsHit(drawID, x, y);
        }
    }
    return false;
}

bool Model::IsPartHit(int index, float x, float y) {
    mMatrixManager.ScreenToScene(&x, &y);
    mMatrixManager.InvertTransform(&x, &y);

    if (mProxy->GetPartOpacity(index) == 0.0f) {
        return false;
    }

    const csmInt32 drawableCount = mProxy->GetDrawableCount();
    const csmInt32* renderOrders = GetDrawableRenderOrders();
    for (csmInt32 i = 0; i < drawableCount; i++) {
        // 绘制顺序，先绘制的被后绘制的覆盖
        mTmpOrderedDrawIndice[drawableCount - 1 - renderOrders[i]] = i;
    }

    for (int i = 0; i < drawableCount; i++) {
        int drawableIndex = mTmpOrderedDrawIndice[i];
        if (mProxy->GetDrawableOpacity(drawableIndex) == 0.0f) {
            continue;
        }
        int partIndex = mProxy->GetDrawableParentPartIndex(drawableIndex);
        if (partIndex != index)   // 不是该 part 的 drawable
        {
            continue;
        }
        const char* partId = mProxy->GetPartId(partIndex)->GetString().GetRawString();

        // 顶点连线个数，3个顶点一个三角形，一定是3的整数倍
        const int indexCount = mProxy->GetDrawableVertexIndexCount(drawableIndex);
        // 顶点坐标
        const csmVector2* vertices = mProxy->GetDrawableVertexPositions(drawableIndex);
        // 三角形顶点索引
        const csmUint16* indices = mProxy->GetDrawableVertexIndices(drawableIndex);
        const int triangleCount = indexCount / 3;

        for (int j = 0; j < triangleCount; j++) {
            if (!isInTriangle(vertices[indices[j * 3]],
                              vertices[indices[j * 3 + 1]],
                              vertices[indices[j * 3 + 2]],
                              {x, y})) {
                continue;
            }
            return true;
        }
    }
    return false;
}

bool Model::IsDrawableHit(int index, float x, float y) {
    mMatrixManager.ScreenToScene(&x, &y);     // 屏幕到OpenGL坐标系
    mMatrixManager.InvertTransform(&x, &y);   // OpenGL坐标系到模型坐标系

    // 顶点连线个数，3个顶点一个三角形，一定是3的整数倍
    const int indexCount = mProxy->GetDrawableVertexIndexCount(index);
    // 顶点坐标
    const csmVector2* vertices = mProxy->GetDrawableVertexPositions(index);
    // 三角形顶点索引
    const csmUint16* indices = mProxy->GetDrawableVertexIndices(index);
    const int triangleCount = indexCount / 3;

    for (int j = 0; j < triangleCount; j++) {
        if (!isInTriangle(vertices[indices[j * 3]],
                          vertices[indices[j * 3 + 1]],
                          vertices[indices[j * 3 + 2]],
                          {x, y})) {
            continue;
        }
        return true;
        break;
    }
    return false;
}

void Model::Drag(float x, float y) {
    mMatrixManager.ScreenToScene(&x, &y);
    mProxy.SetDragging(x, y);
}

void Model::CreateRenderer(int maskBufferCount) {
    if (mProxy.GetRenderer<Rendering::CubismRenderer_OpenGLES2>()) {
        LOGW("Renderer already exists, skipping creation");
        return;
    }
    mTextureManager.ReleaseTextures();
    mProxy.CreateRenderer(mMatrixManager.GetWidth(), mMatrixManager.GetHeight(), maskBufferCount);
    SetupTextures();
}

void Model::DestroyRenderer() {
    mTextureManager.ReleaseTextures();
    mProxy.DeleteRenderer();
}

void Model::Draw() {
    if (mProxy.GetModel() == nullptr) {
        return;
    }

    mProxy->Update();

    CubismMatrix44& matrix = mMatrixManager.GetMvp();
    Rendering::CubismRenderer_OpenGLES2* renderer =
        mProxy.GetRenderer<Rendering::CubismRenderer_OpenGLES2>();

    renderer->SetMvpMatrix(&matrix);

    renderer->DrawModel();
}

int Model::GetPartCount() const {
    return mProxy->GetPartCount();
}

void Model::GetPartIds(void* collector, void (*collect)(void* collector, const char* id)) const {
    for (csmInt32 i = 0; i < mProxy->GetPartCount(); i++) {
        collect(collector, mProxy->GetPartId(i)->GetString().GetRawString());
    }
}

const char* Model::GetPartId(int index) const {
    if (index < 0 || index >= mProxy->GetPartCount()) {
        return "";
    }
    return mProxy->GetPartId(index)->GetString().GetRawString();
}

void Model::SetPartOpacity(int index, float opacity) {
    mProxy->SetPartOpacity(index, opacity);
}

void Model::SetPartScreenColor(int index, float r, float g, float b, float a) {
    auto& overrideColors = mProxy->GetOverrideMultiplyAndScreenColor();
    overrideColors.SetPartScreenColor(index, r, g, b, a);
    if (!overrideColors.GetPartScreenColorEnabled(index)) {
        overrideColors.SetPartMultiplyColorEnabled(index, true);
    }
}

void Model::SetPartMultiplyColor(int index, float r, float g, float b, float a) {
    auto& overrideColors = mProxy->GetOverrideMultiplyAndScreenColor();
    overrideColors.SetPartMultiplyColor(index, r, g, b, a);
    if (!overrideColors.GetPartMultiplyColorEnabled(index)) {
        overrideColors.SetPartMultiplyColorEnabled(index, true);
    }
}

void Model::GetPartScreenColor(int index, float& r, float& g, float& b, float& a) const {
    const auto& overrideColors = mProxy->GetOverrideMultiplyAndScreenColor();
    const auto c = overrideColors.GetPartScreenColor(index);
    r = c.R;
    g = c.G;
    b = c.B;
    a = c.A;
}

void Model::GetPartMultiplyColor(int index, float& r, float& g, float& b, float& a) const {
    const auto& overrideColors = mProxy->GetOverrideMultiplyAndScreenColor();
    const auto c = overrideColors.GetPartMultiplyColor(index);
    r = c.R;
    g = c.G;
    b = c.B;
    a = c.A;
}

int Model::GetDrawableCount() {
    return mProxy->GetDrawableCount();
}

void Model::GetDrawableIds(void* collector, void (*collect)(void* collector, const char* id)) {
    const int count = mProxy->GetDrawableCount();
    for (int i = 0; i < count; i++) {
        collect(collector, mProxy->GetDrawableId(i)->GetString().GetRawString());
    }
}

const float* Model::GetDrawableVertices(int index) {
    return mProxy->GetDrawableVertices(index);
}

int Model::GetDrawableVertexCount(int index) {
    return mProxy->GetDrawableVertexCount(index);
}

int Model::GetDrawableVertexIndexCount(int index) {
    return mProxy->GetDrawableVertexIndexCount(index);
}

const unsigned short* Model::GetDrawableIndices(int index) {
    return mProxy->GetDrawableVertexIndices(index);
}

void Model::SetDrawableMultiColor(int index, float r, float g, float b, float a) {
    const int count = mProxy->GetDrawableVertexCount(index);
    auto& overrideColors = mProxy->GetOverrideMultiplyAndScreenColor();
    overrideColors.SetDrawableMultiplyColor(index, r, g, b, a);
}

void Model::SetDrawableScreenColor(int index, float r, float g, float b, float a) {
    const int count = mProxy->GetDrawableVertexCount(index);
    auto& overrideColors = mProxy->GetOverrideMultiplyAndScreenColor();
    overrideColors.SetDrawableScreenColor(index, r, g, b, a);
}

void Model::AddExpression(const char* expressionId) {
    ACubismMotion* motion = mExpressions[expressionId];

    if (motion != nullptr) {
        LOGI("Add expression: [%s]", expressionId);
        mExpManagers[expressionId]->StartMotion(motion, false);
    } else {
        LOGW("expression[%s] is null ", expressionId);
    }
}

void Model::RemoveExpression(const char* expressionId) {
    if (mExpManagers.find(expressionId) == mExpManagers.end()) {
        return;
    }
    mExpManagers[expressionId]->StopAllMotions();

    LOGI("remove expression: [%s]", expressionId);
}

void Model::SetExpression(const char* expressionId, float fadeoutMs) {
    ACubismMotion* motion = mExpressions[expressionId];

    if (motion != nullptr) {
        LOGI("Set expression: [%s]", expressionId);
        mProxy._expressionManager->StartMotion(motion, false);
    } else {
        LOGW("expression[%s] is null ", expressionId);
    }

    // fadeout 语义: >=0 为临时表情（到时恢复上一个持久表情），<0 为持久表情
    if (fadeoutMs >= 0.0f) {
        mFadeoutMs = fadeoutMs;
        mFadeoutElapsedMs = 0;
    } else {
        mFadeoutMs = -1.0f;
        mLastExpression = expressionId;
    }
}

const char* Model::SetRandomExpression(float fadeoutMs) {
    const int size = mExpressions.GetSize();
    if (size == 0) {
        return nullptr;
    }
    csmInt32 no = rand() % size;
    csmMap<csmString, ACubismMotion*>::const_iterator map_ite;
    csmInt32 i = 0;
    for (map_ite = mExpressions.Begin(); map_ite != mExpressions.End(); map_ite++) {
        if (i == no) {
            csmString name = (*map_ite).First;
            SetExpression(name.GetRawString(), fadeoutMs);
            return name.GetRawString();
        }
        i++;
    }
    return nullptr;
}

void Model::ResetExpressions() {
    for (auto& [id, expMgr] : mExpManagers) {
        expMgr->StopAllMotions();
    }
    mProxy._expressionManager->StopAllMotions();

    mFadeoutMs = -1.0f;
    mFadeoutElapsedMs = 0;
    mLastExpression.clear();

    LOGI("Clear all expressions");
}

void Model::ResetExpression() {
    mFadeoutMs = -1.0f;
    mFadeoutElapsedMs = 0;
    mLastExpression.clear();
    mProxy._expressionManager->StopAllMotions();
    LOGI("Reset expression");
}

int Model::GetExpressionCount() {
    return mModelSetting->GetExpressionCount();
}

void Model::GetExpressions(void* collector,
                           void (*collect)(void* collector, const char* id, const char* file)) {
    const int count = mModelSetting->GetExpressionCount();
    for (int i = 0; i < count; i++) {
        const char* file = mModelSetting->GetExpressionFileName(i);
        const char* id = mModelSetting->GetExpressionName(i);
        collect(collector, id, file);
    }
}

void Model::LoadExtraExpression(const char* expressionId, const char* expressionFilePath) {
    LoadAssets(expressionFilePath, [&](csmByte* buffer, csmSizeInt size) {
        ACubismMotion* expression = mProxy.LoadExpression(buffer, size, expressionId);
        if (expression) {
            const std::string key = expressionId;
            if (mExpressions[expressionId] != nullptr) {
                LOGW("Expression has been overwritten: %s", expressionId);
                ACubismMotion::Delete(mExpressions[expressionId]);
                mExpressions[expressionId] = nullptr;
            }
            if (mExpManagers[key] != nullptr) {
                CSM_DELETE(mExpManagers[key]);
                mExpManagers.erase(key);
            }
            mExpressions[expressionId] = expression;
            mExpManagers[key] = CSM_NEW CubismExpressionMotionManager();
            LOGI("Load extra expression: %s => [%s]", expressionFilePath, expressionId);
        } else {
            LOGW("Failed to load motion: %s", expressionFilePath);
        }
    });
}

void Model::StopAllMotions() {
    mProxy._motionManager->StopAllMotions();
}

void Model::ResetAllParameters() {
    for (int i = 0; i < mParameterCount; i++) {
        mParameterValues[i] = mParameterDefaultValues[i];
        mSavedParameterValues[i] = mParameterDefaultValues[i];
    }
}

void Model::ResetPose() {
    if (mProxy._pose != nullptr) {
        mProxy._pose->Reset(mProxy.GetModel());
    }
}

void Model::GetCanvasSize(float& w, float& h) {
    w = mProxy->GetCanvasWidth();
    h = mProxy->GetCanvasHeight();
}

void Model::GetCanvasSizePixel(float& w, float& h) {
    w = mProxy->GetCanvasWidthPixel();
    h = mProxy->GetCanvasHeightPixel();
}

float Model::GetPixelsPerUnit() {
    return mProxy->GetPixelsPerUnit();
}

void Model::SetAutoBlink(bool on) {
    autoBlink = on;
}

void Model::SetAutoBreath(bool on) {
    autoBreath = on;
}

bool Model::AutoBreathEnabled() const {
    return autoBreath;
}

bool Model::AutoBlinkEnabled() const {
    return autoBlink;
}

bool Model::HasMocConsistencyFromFile(const char* mocFileName) {
    if (!mocFileName || !*mocFileName)
        return false;
    csmString path = JoinPath(mModelHomeDir, mocFileName);
    csmSizeInt size;
    csmByte* buffer = LAppPal::LoadFileAsBytes(path.GetRawString(), &size);
    if (!buffer)
        return false;
    bool ok = CubismMoc::HasMocConsistencyFromUnrevivedMoc(buffer, size);
    LAppPal::ReleaseBytes(buffer);
    return ok;
}

void Model::ReleaseMotions() {
    for (csmMap<csmString, ACubismMotion*>::const_iterator iter = mMotions.Begin();
         iter != mMotions.End();
         ++iter) {
        ACubismMotion::Delete(iter->Second);
    }

    mMotions.Clear();
}

void Model::ReleaseExpressions() {
    for (csmMap<csmString, ACubismMotion*>::const_iterator iter = mExpressions.Begin();
         iter != mExpressions.End();
         ++iter) {
        ACubismMotion::Delete(iter->Second);
    }

    mExpressions.Clear();
}

void Model::ReleaseExpressionManagers() {
    for (auto& [id, expMgr] : mExpManagers) {
        delete expMgr;
    }
    mExpManagers.clear();
}

void Model::SetupTextures() {
    for (csmInt32 modelTextureNumber = 0; modelTextureNumber < mModelSetting->GetTextureCount();
         modelTextureNumber++) {
        if (strcmp(mModelSetting->GetTextureFileName(modelTextureNumber), "") == 0) {
            continue;
        }

        csmString texturePath = mModelSetting->GetTextureFileName(modelTextureNumber);
        texturePath = JoinPath(mModelHomeDir, texturePath.GetRawString());

        // 已经加载过的纹理会直接复用
        LAppTextureManager::TextureInfo* texture =
            mTextureManager.CreateTextureFromPngFile(texturePath.GetRawString());
        const csmInt32 glTextueNumber = texture->id;

        // OpenGL
        mProxy.GetRenderer<Rendering::CubismRenderer_OpenGLES2>()->BindTexture(modelTextureNumber,
                                                                               glTextueNumber);
    }

#ifdef PREMULTIPLIED_ALPHA_ENABLE
    mProxy.GetRenderer<Rendering::CubismRenderer_OpenGLES2>()->IsPremultipliedAlpha(true);
#else
    mProxy.GetRenderer<Rendering::CubismRenderer_OpenGLES2>()->IsPremultipliedAlpha(false);
#endif
}

void Model::PreloadMotionGroup(const csmChar* group) {
    const csmInt32 count = mModelSetting->GetMotionCount(group);

    if (count > 0) {
        mMotionGroupNames.push_back(group);
        mMotionCounts.push_back(count);
    }

    for (csmInt32 i = 0; i < count; i++) {
        // ex) idle_0
        csmString name = Utils::CubismString::GetFormatedString("%s_%d", group, i);
        csmString path = mModelSetting->GetMotionFileName(group, i);
        path = JoinPath(mModelHomeDir, path.GetRawString());

        LOGI("load motion: %s => [%s_%d] ", path.GetRawString(), group, i);

        LoadAssets(path.GetRawString(), [&](csmByte* buffer, csmInt32 size) {
            CubismMotion* tmpMotion = static_cast<CubismMotion*>(mProxy.LoadMotion(
                buffer, size, name.GetRawString(), NULL, NULL, mModelSetting, group, i));
            if (tmpMotion) {
                tmpMotion->SetEffectIds(mEyeBlinkIds, mLipSyncIds);

                if (mMotions[name] != NULL) {
                    ACubismMotion::Delete(mMotions[name]);
                }
                mMotions[name] = tmpMotion;
            }
        });
    }
}

const int* Model::GetDrawableRenderOrders() const {
    return mProxy->GetRenderOrders();
}

void Model::ResumeLastExpressionIfNeeded(float deltaSecs) {
    // 表情 fadeout: 到期恢复上一个持久表情
    if (mFadeoutMs >= 0.0f) {
        mFadeoutElapsedMs += deltaSecs * 1000.0f;
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
}   // namespace V3
}   // namespace Live2D