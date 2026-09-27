#include "Model.hpp"
#include "Motion/ACubismMotion.hpp"

#include <CubismDefaultParameterId.hpp>
#include <CubismModelSettingJson.hpp>
#include <Id/CubismIdManager.hpp>
#include <Live2DCubismCore.hpp>
#include <Model/CubismMoc.hpp>
#include <Motion/CubismMotion.hpp>
#include <Rendering/OpenGL/CubismShader_OpenGLES2.hpp>
#include <Utils/CubismString.hpp>


#include <LAppDefine.hpp>
#include <LAppPal.hpp>
#include <Log.hpp>


#include <algorithm>
#include <filesystem>
#include <functional>
#include <unordered_set>


using namespace Live2D::Cubism::Framework;
using namespace LAppDefine;
using namespace Live2D::Cubism::Framework::DefaultParameterId;
using namespace Live2D::Cubism::Core;
using namespace Live2D::Common::Log;

namespace Live2D {
namespace V3 {
namespace {
class FakeMotion : public ACubismMotion {
protected:
    void DoUpdateParameters(CubismModel* model, csmFloat32 userTimeSeconds, csmFloat32 weight,
                            CubismMotionQueueEntry* motionQueueEntry) override {}

public:
    FakeMotion() = default;
};

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
}   // namespace

Model::Model()
    : CubismUserModel()
    , mModelSetting(nullptr)
    , mMatrixManager()
    , mParameterCount(0)
    , mParameterDefaultValues(nullptr)
    , mParameterValues(nullptr)
    , mTmpOrderedDrawIndice(nullptr)
    , autoBlink(true)
    , autoBreath(true) {
    _mocConsistency = true;

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

void Model::LoadModelJson(const char* filePath) {
    std::filesystem::path p = std::filesystem::u8path(filePath);
    mModelHomeDir = p.parent_path().generic_u8string().c_str();
    mModelHomeDir += "/";

    LOGI("load modelSetting: %s", filePath);
    LoadAssets(filePath, [&](csmByte* buffer, csmSizeInt size) {
        mModelSetting = new CubismModelSettingJson(buffer, size);
    });

    SetupModel();
}

ACubismMotion* Model::LoadMotion(const csmByte* buffer, csmSizeInt size, const csmChar* name,
                                 ACubismMotion::FinishedMotionCallback onFinished,
                                 ACubismMotion::BeganMotionCallback onBegan,
                                 ICubismModelSetting* modelSetting, const csmChar* group,
                                 csmInt32 index, csmBool shouldCheckMotionConsistency) {
    std::string fixed(reinterpret_cast<const char*>(buffer), size);
    LAppPal::FixMotionJson(fixed);
    return CubismUserModel::LoadMotion(reinterpret_cast<const csmByte*>(fixed.data()),
                                       static_cast<csmSizeInt>(fixed.size()),
                                       name,
                                       onFinished,
                                       onBegan,
                                       modelSetting,
                                       group,
                                       index,
                                       shouldCheckMotionConsistency);
}

const char* Model::GetModelHomeDir() {
    return mModelHomeDir.GetRawString();
}

void Model::Update(float deltaSecs) {
    _dragManager->Update(deltaSecs);
    mDragX = _dragManager->GetX();
    mDragY = _dragManager->GetY();

    bool motionUpdated = false;
    LoadParameters();
    if (!_motionManager->IsFinished()) {
        motionUpdated = _motionManager->UpdateMotion(_model, deltaSecs);
    }
    SaveParameters();

    _opacity = _model->GetModelOpacity();

    if (!motionUpdated) {
        if (_eyeBlink != NULL && autoBlink) {
            _eyeBlink->UpdateParameters(_model, deltaSecs);
        }
    }

    UpdateExpression(deltaSecs);

    _model->AddParameterValue(mParamAngleXi, mDragX * 30);
    _model->AddParameterValue(mParamAngleYi, mDragY * 30);
    _model->AddParameterValue(mParamAngleZi, mDragX * mDragY * -30);

    _model->AddParameterValue(mParamBodyAngleXi, mDragX * 10);

    _model->AddParameterValue(mParamEyeBallXi, mDragX);
    _model->AddParameterValue(mParamEyeBallYi, mDragY);

    if (_breath != NULL && autoBreath) {
        _breath->UpdateParameters(_model, deltaSecs);
    }

    if (_physics != NULL) {
        _physics->Evaluate(_model, deltaSecs);
    }

    if (_pose != NULL) {
        _pose->UpdateParameters(_model, deltaSecs);
    }
}

void Model::SetupModel() {
    // moc3
    if (strcmp(mModelSetting->GetModelFileName(), "") != 0) {
        csmString path = mModelSetting->GetModelFileName();
        path = mModelHomeDir + path;

        LOGI("create model: %s", mModelSetting->GetModelFileName());

        LoadAssets(path.GetRawString(), [&](csmByte* buffer, csmSizeInt size) {
            LoadModel(buffer, size, _mocConsistency);
        });
    }

    if (_model == nullptr) {
        LOGE("Failed to SetupModel()");
    }

    // exp3.json
    if (mModelSetting->GetExpressionCount() > 0) {
        const csmInt32 count = mModelSetting->GetExpressionCount();
        for (csmInt32 i = 0; i < count; i++) {
            csmString name = mModelSetting->GetExpressionName(i);
            csmString path = mModelHomeDir + mModelSetting->GetExpressionFileName(i);

            LoadAssets(path.GetRawString(), [&](csmByte* buffer, csmSizeInt size) {
                ACubismMotion* motion = LoadExpression(buffer, size, name.GetRawString());
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
        csmString path = mModelHomeDir + mModelSetting->GetPhysicsFileName();

        LoadAssets(path.GetRawString(),
                   [&](csmByte* buffer, csmSizeInt size) { LoadPhysics(buffer, size); });
    }

    // pose3.json
    if (strcmp(mModelSetting->GetPoseFileName(), "") != 0) {
        csmString path = mModelHomeDir + mModelSetting->GetPoseFileName();

        LoadAssets(path.GetRawString(),
                   [&](csmByte* buffer, csmSizeInt size) { LoadPose(buffer, size); });
    }

    // EyeBlink
    if (mModelSetting->GetEyeBlinkParameterCount() > 0) {
        _eyeBlink = CubismEyeBlink::Create(mModelSetting);
    }

    // Breath
    {
        _breath = CubismBreath::Create();

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

        _breath->SetParameters(breathParameters);
    }

    // UserData
    if (strcmp(mModelSetting->GetUserDataFile(), "") != 0) {
        csmString path = mModelHomeDir + mModelSetting->GetUserDataFile();
        LoadAssets(path.GetRawString(),
                   [&](csmByte* buffer, csmSizeInt size) { LoadUserData(buffer, size); });
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

    if (mModelSetting == nullptr || _modelMatrix == nullptr) {
        LOGE("Failed to SetupModel()");
        return;
    }

    // Layout
    csmMap<csmString, csmFloat32> layout;
    mModelSetting->GetLayoutMap(layout);
    _modelMatrix->SetupFromLayout(layout);

    // motion3.json
    mMotionGroupNames.clear();
    mMotionCounts.clear();
    for (csmInt32 i = 0; i < mModelSetting->GetMotionGroupCount(); i++) {
        const csmChar* group = mModelSetting->GetMotionGroupName(i);
        PreloadMotionGroup(group);
    }
    _motionManager->StopAllMotions();
    mMatrixManager.SetModelWH(_model->GetCanvasWidth(), _model->GetCanvasHeight());
    mParamAngleXi = _model->GetParameterIndex(mIdParamAngleX);
    mParamAngleYi = _model->GetParameterIndex(mIdParamAngleY);
    mParamAngleZi = _model->GetParameterIndex(mIdParamAngleZ);
    mParamBodyAngleXi = _model->GetParameterIndex(mIdParamBodyAngleX);
    mParamEyeBallXi = _model->GetParameterIndex(mIdParamEyeBallX);
    mParamEyeBallYi = _model->GetParameterIndex(mIdParamEyeBallY);
    mTmpOrderedDrawIndice = new int[_model->GetDrawableCount()];
    csmModel* model = _model->GetModel();
    mParameterDefaultValues = csmGetParameterDefaultValues(model);
    mParameterValues = csmGetParameterValues(model);
    mParameterCount = csmGetParameterCount(model);
    mSavedParameterValues.resize(mParameterCount);
    SaveParameters();
    LOGD("Model setup complete");
}

bool Model::IsHit(CubismIdHandle drawableId, csmFloat32 pointX, csmFloat32 pointY) {
    const csmInt32 drawIndex = _model->GetDrawableIndex(drawableId);

    if (drawIndex < 0) {
        return false;   // 存在しない場合はfalse
    }

    const csmInt32 count = _model->GetDrawableVertexCount(drawIndex);
    const csmFloat32* vertices = _model->GetDrawableVertices(drawIndex);

    csmFloat32 left = vertices[0];
    csmFloat32 right = vertices[0];
    csmFloat32 top = vertices[1];
    csmFloat32 bottom = vertices[1];

    for (csmInt32 j = 1; j < count; ++j) {
        csmFloat32 x = vertices[Constant::VertexOffset + j * Constant::VertexStep];
        csmFloat32 y = vertices[Constant::VertexOffset + j * Constant::VertexStep + 1];

        if (x < left) {
            left = x;   // Min x
        }

        if (x > right) {
            right = x;   // Max x
        }

        if (y < top) {
            top = y;   // Min y
        }

        if (y > bottom) {
            bottom = y;   // Max y
        }
    }

    return ((left <= pointX) && (pointX <= right) && (top <= pointY) && (pointY <= bottom));
}

bool Model::UpdateMotion(float deltaSecs) {
    _opacity = _model->GetModelOpacity();
    return !_motionManager->IsFinished() && _motionManager->UpdateMotion(_model, deltaSecs);
}

void Model::UpdateDrag(float deltaSecs) {
    _dragManager->Update(deltaSecs);
    mDragX = _dragManager->GetX();
    mDragY = _dragManager->GetY();

    _model->AddParameterValue(mParamAngleXi, mDragX * 30);
    _model->AddParameterValue(mParamAngleYi, mDragY * 30);
    _model->AddParameterValue(mParamAngleZi, mDragX * mDragY * -30);

    _model->AddParameterValue(mParamBodyAngleXi, mDragX * 10);

    _model->AddParameterValue(mParamEyeBallXi, mDragX);
    _model->AddParameterValue(mParamEyeBallYi, mDragY);
}

void Model::UpdateBreath(float deltaSecs) {
    if (_breath == nullptr) {
        return;
    }
    _breath->UpdateParameters(_model, deltaSecs);
}

void Model::UpdateBlink(float deltaSecs) {
    if (_eyeBlink == nullptr) {
        return;
    }
    _eyeBlink->UpdateParameters(_model, deltaSecs);
}

void Model::UpdateExpression(float deltaSecs) {
    if (_expressionManager->IsFinished()) {
        for (auto& pair : mExpManagers) {
            pair.second->UpdateMotion(_model, deltaSecs);
        }
    } else {
        _expressionManager->UpdateMotion(_model, deltaSecs);
    }
}

void Model::UpdatePhysics(float deltaSecs) {
    if (_physics == nullptr) {
        return;
    }

    _physics->Evaluate(_model, deltaSecs);
}

void Model::UpdatePose(float deltaSecs) {
    if (_pose == nullptr) {
        return;
    }

    _pose->UpdateParameters(_model, deltaSecs);
}

int Model::GetParameterCount() {
    return _model->GetParameterCount();
}

void Model::GetParameterIds(void* collector, void (*collect)(void* collector, const char* id)) {
    for (csmInt32 i = 0; i < mParameterCount; ++i) {
        collect(collector, _model->GetParameterId(i)->GetString().GetRawString());
    }
}

float Model::GetParameterValue(int index) {
    return _model->GetParameterValue(index);
}

float Model::GetParameterMaximumValue(int index) {
    return _model->GetParameterMaximumValue(index);
}

float Model::GetParameterMinimumValue(int index) {
    return _model->GetParameterMinimumValue(index);
}

float Model::GetParameterDefaultValue(int index) {
    return _model->GetParameterDefaultValue(index);
}

void Model::SetParameterValue(const char* id, float value, float weight) {
    const CubismId* handle = CubismFramework::GetIdManager()->GetId(id);
    _model->SetParameterValue(handle, value, weight);
}

void Model::SetParameterValue(int index, float value, float weight) {
    _model->SetParameterValue(index, value, weight);
}

void Model::AddParameterValue(const char* id, float value) {
    const CubismId* handle = CubismFramework::GetIdManager()->GetId(id);
    _model->AddParameterValue(handle, value);
}

void Model::AddParameterValue(int index, float value) {
    _model->AddParameterValue(index, value);
}

void Model::SetAndSaveParameterValue(const char* id, float value, float weight) {
    const CubismId* handle = CubismFramework::GetIdManager()->GetId(id);
    const int index = _model->GetParameterIndex(handle);
    _model->SetParameterValue(index, value, weight);
    if (index < mParameterCount) {
        mSavedParameterValues[index] = mParameterValues[index];
    }
}

void Model::SetAndSaveParameterValue(int index, float value, float weight) {
    _model->SetParameterValue(index, value, weight);
    if (index < mParameterCount) {
        mSavedParameterValues[index] = mParameterValues[index];
    }
}

void Model::AddAndSaveParameterValue(const char* id, float value) {
    const CubismId* handle = CubismFramework::GetIdManager()->GetId(id);
    const int index = _model->GetParameterIndex(handle);
    _model->AddParameterValue(index, value);
    if (index < mParameterCount) {
        mSavedParameterValues[index] = mParameterValues[index];
    }
}

void Model::AddAndSaveParameterValue(int index, float value) {
    _model->AddParameterValue(index, value);
    if (index < mParameterCount) {
        mSavedParameterValues[index] = mParameterValues[index];
    }
}

void Model::LoadParameters() {
    for (int i = 0; i < mParameterCount; ++i) {
        _model->SetParameterValue(i, mSavedParameterValues[i]);
    }
}

void Model::SaveParameters() {
    for (int i = 0; i < mParameterCount; ++i) {
        mSavedParameterValues[i] = mParameterValues[i];
    }
}

void Model::Resize(int width, int height) {
    mMatrixManager.UpdateScreenToScene(width, height);
    auto renderer = GetRenderer<Rendering::CubismRenderer_OpenGLES2>();
    if (renderer) {
        renderer->SetRenderTargetSize(width, height);
    }
}

void Model::SetOffset(float x, float y) {
    mMatrixManager.SetOffset(x, y);
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

void Model::StartMotion(const char* group, int no, int priority, void* startCallee,
                        ACubismMotion::BeganMotionCallback onStartMotionHandler, void* finishCallee,
                        ACubismMotion::FinishedMotionCallback onFinishMotionHandler) {
    if (priority == PriorityForce) {
        _motionManager->SetReservePriority(priority);
    } else if (!_motionManager->ReserveMotion(priority)) {
        LOGI("motion priority is too low.");
        return;
    }

    // ex) idle_0
    csmString name = Utils::CubismString::GetFormatedString("%s_%d", group, no);
    CubismMotion* motion = static_cast<CubismMotion*>(mMotions[name.GetRawString()]);
    csmBool autoDelete = false;

    csmBool hasMotion = true;

    if (motion == NULL) {
        // 加载临时 motion
        const csmString fileName = mModelSetting->GetMotionFileName(group, no);
        if (fileName.GetLength() <= 0) {
            hasMotion = false;
            LOGI("motion(%s) has no file attached", name.GetRawString());
            goto handler_label;
        }

        csmString path = fileName;

        path = mModelHomeDir + path;

        LoadAssets(path.GetRawString(), [&](csmByte* buffer, csmSizeInt size) {
            motion = static_cast<CubismMotion*>(LoadMotion(buffer, size, NULL));

            if (motion) {
                csmFloat32 fadeTime = mModelSetting->GetMotionFadeInTimeValue(group, no);
                if (fadeTime >= 0.0f) {
                    motion->SetFadeInTime(fadeTime);
                }

                fadeTime = mModelSetting->GetMotionFadeOutTimeValue(group, no);
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
        motion->group = group;
        motion->no = no;
        motion->SetBeganMotionCustomData(startCallee);
        motion->SetFinishedMotionCustomData(finishCallee);
        motion->SetBeganMotionHandler(onStartMotionHandler);
        motion->SetFinishedMotionHandler(onFinishMotionHandler);
    }

handler_label:

    if (!hasMotion) {
        // 添加空指针判断，如果 motion 文件不存在，直接调用动作结束回调函数
        // 修复模型文件不存在时，导致崩溃
        FakeMotion fakeMotion;
        fakeMotion.group = group;
        fakeMotion.no = no;
        fakeMotion.SetBeganMotionCustomData(startCallee);
        fakeMotion.SetFinishedMotionCustomData(finishCallee);
        if (onStartMotionHandler) {
            onStartMotionHandler(&fakeMotion);
        }
        if (onFinishMotionHandler) {
            onFinishMotionHandler(&fakeMotion);
        }
        _motionManager->SetReservePriority(PriorityNone);
    }

    _motionManager->StartMotionPriority(motion, autoDelete, priority);
}

void Model::StartRandomMotion(const char* group, int priority, void* startCallee,
                              ACubismMotion::BeganMotionCallback startCalleeHandler,
                              void* finishCallee,
                              ACubismMotion::FinishedMotionCallback finishCalleeHandler) {
    csmString g;
    int gindex = -1;
    if (group == nullptr) {
        int gcnt = mMotionGroupNames.size();
        if (gcnt > 0) {
            gindex = rand() % gcnt;
            g = mMotionGroupNames[gindex];
        }
    } else {
        g = group;
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

    StartMotion(g.GetRawString(),
                no,
                priority,
                startCallee,
                startCalleeHandler,
                finishCallee,
                finishCalleeHandler);
}

bool Model::IsMotionFinished() {
    return _motionManager->IsFinished();
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

        CubismMotion* tmpMotion = static_cast<CubismMotion*>(
            LoadMotion(buffer, size, name.GetRawString(), NULL, NULL, mModelSetting, group, no));

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
    const csmInt32 drawableCount = _model->GetDrawableCount();
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
        if (_model->GetDrawableOpacity(drawableIndex) == 0.0f) {
            continue;
        }
        int partIndex = _model->GetDrawableParentPartIndex(drawableIndex);
        if (partIndex == -1) {
            // 绘制对象不属于 part
            continue;
        }
        const char* partId = _model->GetPartId(partIndex)->GetString().GetRawString();
        if (_model->GetPartOpacity(partIndex) == 0.0f) {
            continue;
        }
        // 已经点击过的部件
        if (hitParts.find(partId) != hitParts.end()) {
            continue;
        }
        // 顶点连线个数，3个顶点一个三角形，一定是3的整数倍
        const int indexCount = _model->GetDrawableVertexIndexCount(drawableIndex);
        // 顶点坐标
        const csmVector2* vertices = _model->GetDrawableVertexPositions(drawableIndex);
        // 三角形顶点索引
        const csmUint16* indices = _model->GetDrawableVertexIndices(drawableIndex);
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

    const csmInt32 drawableCount = _model->GetDrawableCount();
    const csmInt32* renderOrders = GetDrawableRenderOrders();
    for (csmInt32 i = 0; i < drawableCount; i++) {
        // 绘制顺序，先绘制的被后绘制的覆盖
        mTmpOrderedDrawIndice[drawableCount - 1 - renderOrders[i]] = i;
    }
    bool topClicked = false;

    for (int i = 0; i < drawableCount; i++) {
        int drawableIndex = mTmpOrderedDrawIndice[i];
        if (_model->GetDrawableOpacity(drawableIndex) == 0.0f) {
            continue;
        }
        const char* drawableId = _model->GetDrawableId(drawableIndex)->GetString().GetRawString();

        // 顶点连线个数，3个顶点一个三角形，一定是3的整数倍
        const int indexCount = _model->GetDrawableVertexIndexCount(drawableIndex);
        // 顶点坐标
        const csmVector2* vertices = _model->GetDrawableVertexPositions(drawableIndex);
        // 三角形顶点索引
        const csmUint16* indices = _model->GetDrawableVertexIndices(drawableIndex);
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

    if (_opacity < 1) {
        return false;
    }
    const csmInt32 count = mModelSetting->GetHitAreasCount();
    for (csmInt32 i = 0; i < count; i++) {
        if (strcmp(mModelSetting->GetHitAreaName(i), areaName) == 0) {
            const CubismIdHandle drawID = mModelSetting->GetHitAreaId(i);
            return IsHit(drawID, x, y);
        }
    }
    return false;
}

bool Model::IsPartHit(int index, float x, float y) {
    mMatrixManager.ScreenToScene(&x, &y);
    mMatrixManager.InvertTransform(&x, &y);

    if (_model->GetPartOpacity(index) == 0.0f) {
        return false;
    }

    const csmInt32 drawableCount = _model->GetDrawableCount();
    const csmInt32* renderOrders = GetDrawableRenderOrders();
    for (csmInt32 i = 0; i < drawableCount; i++) {
        // 绘制顺序，先绘制的被后绘制的覆盖
        mTmpOrderedDrawIndice[drawableCount - 1 - renderOrders[i]] = i;
    }

    for (int i = 0; i < drawableCount; i++) {
        int drawableIndex = mTmpOrderedDrawIndice[i];
        if (_model->GetDrawableOpacity(drawableIndex) == 0.0f) {
            continue;
        }
        int partIndex = _model->GetDrawableParentPartIndex(drawableIndex);
        if (partIndex != index)   // 不是该 part 的 drawable
        {
            continue;
        }
        const char* partId = _model->GetPartId(partIndex)->GetString().GetRawString();

        // 顶点连线个数，3个顶点一个三角形，一定是3的整数倍
        const int indexCount = _model->GetDrawableVertexIndexCount(drawableIndex);
        // 顶点坐标
        const csmVector2* vertices = _model->GetDrawableVertexPositions(drawableIndex);
        // 三角形顶点索引
        const csmUint16* indices = _model->GetDrawableVertexIndices(drawableIndex);
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
    const int indexCount = _model->GetDrawableVertexIndexCount(index);
    // 顶点坐标
    const csmVector2* vertices = _model->GetDrawableVertexPositions(index);
    // 三角形顶点索引
    const csmUint16* indices = _model->GetDrawableVertexIndices(index);
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
    SetDragging(x, y);
}

void Model::CreateRenderer(int maskBufferCount) {
    mTextureManager.ReleaseTextures();
    CubismUserModel::CreateRenderer(
        mMatrixManager.GetWidth(), mMatrixManager.GetHeight(), maskBufferCount);
    SetupTextures();
}

void Model::DestroyRenderer() {
    mTextureManager.ReleaseTextures();
    CubismUserModel::DeleteRenderer();
}

void Model::Draw() {
    if (_model == nullptr) {
        return;
    }

    _model->Update();

    CubismMatrix44& matrix = mMatrixManager.GetMvp();
    Rendering::CubismRenderer_OpenGLES2* renderer =
        GetRenderer<Rendering::CubismRenderer_OpenGLES2>();

    renderer->SetMvpMatrix(&matrix);

    renderer->DrawModel();
}

const int Model::GetPartCount() const {
    return _model->GetPartCount();
}

void Model::GetPartIds(void* collector, void (*collect)(void* collector, const char* id)) const {
    for (csmInt32 i = 0; i < _model->GetPartCount(); i++) {
        collect(collector, _model->GetPartId(i)->GetString().GetRawString());
    }
}

void Model::SetPartOpacity(int index, float opacity) {
    _model->SetPartOpacity(index, opacity);
}

void Model::SetPartScreenColor(int index, float r, float g, float b, float a) {
    auto& overrideColors = _model->GetOverrideMultiplyAndScreenColor();
    overrideColors.SetPartScreenColor(index, r, g, b, a);
    if (!overrideColors.GetPartScreenColorEnabled(index)) {
        overrideColors.SetPartMultiplyColorEnabled(index, true);
    }
}

void Model::SetPartMultiplyColor(int index, float r, float g, float b, float a) {
    auto& overrideColors = _model->GetOverrideMultiplyAndScreenColor();
    overrideColors.SetPartMultiplyColor(index, r, g, b, a);
    if (!overrideColors.GetPartMultiplyColorEnabled(index)) {
        overrideColors.SetPartMultiplyColorEnabled(index, true);
    }
}

void Model::GetPartScreenColor(int index, float& r, float& g, float& b, float& a) const {
    const auto& overrideColors = _model->GetOverrideMultiplyAndScreenColor();
    const auto c = overrideColors.GetPartScreenColor(index);
    r = c.R;
    g = c.G;
    b = c.B;
    a = c.A;
}

void Model::GetPartMultiplyColor(int index, float& r, float& g, float& b, float& a) const {
    const auto& overrideColors = _model->GetOverrideMultiplyAndScreenColor();
    const auto c = overrideColors.GetPartMultiplyColor(index);
    r = c.R;
    g = c.G;
    b = c.B;
    a = c.A;
}

int Model::GetDrawableCount() {
    return _model->GetDrawableCount();
}

void Model::GetDrawableIds(void* collector, void (*collect)(void* collector, const char* id)) {
    const int count = _model->GetDrawableCount();
    for (int i = 0; i < count; i++) {
        collect(collector, _model->GetDrawableId(i)->GetString().GetRawString());
    }
}

const float* Model::GetDrawableVertices(int index) {
    return _model->GetDrawableVertices(index);
}

const int Model::GetDrawableVertexCount(int index) {
    return _model->GetDrawableVertexCount(index);
}

const int Model::GetDrawableVertexIndexCount(int index) {
    return _model->GetDrawableVertexIndexCount(index);
}

const unsigned short* Model::GetDrawableIndices(int index) {
    return _model->GetDrawableVertexIndices(index);
}

void Model::SetDrawableMultiColor(int index, float r, float g, float b, float a) {
    const int count = _model->GetDrawableVertexCount(index);
    auto& overrideColors = _model->GetOverrideMultiplyAndScreenColor();
    overrideColors.SetDrawableMultiplyColor(index, r, g, b, a);
}

void Model::SetDrawableScreenColor(int index, float r, float g, float b, float a) {
    const int count = _model->GetDrawableVertexCount(index);
    auto& overrideColors = _model->GetOverrideMultiplyAndScreenColor();
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

void Model::SetExpression(const char* expressionId) {
    ACubismMotion* motion = mExpressions[expressionId];

    LOGI("Set expression: [%s]", expressionId);

    if (motion != nullptr) {
        _expressionManager->StartMotion(motion, false);
    } else {
        LOGW("expression[%s] is null ", expressionId);
    }
}

const char* Model::SetRandomExpression() {
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
            SetExpression(name.GetRawString());
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
    _expressionManager->StopAllMotions();

    LOGI("Clear all expressions");
}

void Model::ResetExpression() {
    _expressionManager->StopAllMotions();
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
        ACubismMotion* expression = LoadExpression(buffer, size, expressionId);
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
    _motionManager->StopAllMotions();
}

void Model::ResetAllParameters() {
    for (int i = 0; i < mParameterCount; i++) {
        mParameterValues[i] = mParameterDefaultValues[i];
        mSavedParameterValues[i] = mParameterDefaultValues[i];
    }
}

void Model::ResetPose() {
    if (_pose != nullptr) {
        _pose->Reset(_model);
    }
}

void Model::GetCanvasSize(float& w, float& h) {
    w = _model->GetCanvasWidth();
    h = _model->GetCanvasHeight();
}

void Model::GetCanvasSizePixel(float& w, float& h) {
    w = _model->GetCanvasWidthPixel();
    h = _model->GetCanvasHeightPixel();
}

float Model::GetPixelsPerUnit() {
    return _model->GetPixelsPerUnit();
}

void Model::SetAutoBlink(bool on) {
    autoBlink = on;
}

void Model::SetAutoBreath(bool on) {
    autoBreath = on;
}

bool Model::HasMocConsistencyFromFile(const char* mocFileName) {
    if (!mocFileName || !*mocFileName)
        return false;
    csmString path = mModelHomeDir + mocFileName;
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
        texturePath = mModelHomeDir + texturePath;

        // 已经加载过的纹理会直接复用
        LAppTextureManager::TextureInfo* texture =
            mTextureManager.CreateTextureFromPngFile(texturePath.GetRawString());
        const csmInt32 glTextueNumber = texture->id;

        // OpenGL
        GetRenderer<Rendering::CubismRenderer_OpenGLES2>()->BindTexture(modelTextureNumber,
                                                                        glTextueNumber);
    }

#ifdef PREMULTIPLIED_ALPHA_ENABLE
    GetRenderer<Rendering::CubismRenderer_OpenGLES2>()->IsPremultipliedAlpha(true);
#else
    GetRenderer<Rendering::CubismRenderer_OpenGLES2>()->IsPremultipliedAlpha(false);
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
        path = mModelHomeDir + path;

        LOGI("load motion: %s => [%s_%d] ", path.GetRawString(), group, i);

        LoadAssets(path.GetRawString(), [&](csmByte* buffer, csmInt32 size) {
            CubismMotion* tmpMotion = static_cast<CubismMotion*>(
                LoadMotion(buffer, size, name.GetRawString(), NULL, NULL, mModelSetting, group, i));
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
    return _model->GetRenderOrders();
}
}   // namespace V3
}   // namespace Live2D