/**
 * @brief Model.hpp
 * @author Arkueid
 * @date 2025/04/06
 * @note 更细粒度、更独立的 live2d 模型管理类
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>


#include <Model/CubismUserModel.hpp>
#include <Motion/ACubismMotion.hpp>

#include <LAppTextureManager.hpp>
#include <MatrixManagerV3.hpp>

using namespace Csm;

namespace Live2D {
namespace V3 {

class Model : public Csm::CubismUserModel {
public:
    Model();
    ~Model() override;

    /**
     * @brief
     * @param filePath model3.json path
     */
    void LoadModelJson(const char* filePath);

    const char* GetModelHomeDir();

    // Hook motion loading to auto-fix meta counts
    Csm::ACubismMotion* LoadMotion(const Csm::csmByte* buffer, Csm::csmSizeInt size,
                                   const Csm::csmChar* name,
                                   Csm::ACubismMotion::FinishedMotionCallback onFinished = NULL,
                                   Csm::ACubismMotion::BeganMotionCallback onBegan = NULL,
                                   Csm::ICubismModelSetting* modelSetting = NULL,
                                   const Csm::csmChar* group = NULL, Csm::csmInt32 index = -1,
                                   csmBool shouldCheckMotionConsistency = false) override;

    // update

    void Update(float deltaSecs);

    /**
     * @brief
     * @param deltaSecs time elapsed since last frame
     * @return true if motion is not finished and motion is updated
     */
    bool UpdateMotion(float deltaSecs);

    void UpdateDrag(float deltaSecs);

    void UpdateBreath(float deltaSecs);

    void UpdateBlink(float deltaSecs);

    void UpdateExpression(float deltaSecs);

    void UpdatePhysics(float deltaSecs);

    void UpdatePose(float deltaSecs);

    // param
    int GetParameterCount();

    void GetParameterIds(void* collector, void (*collect)(void* collector, const char* id));

    float GetParameterValue(int index);

    float GetParameterMaximumValue(int index);

    float GetParameterMinimumValue(int index);

    float GetParameterDefaultValue(int index);

    void SetParameterValue(const char* id, float value, float weight = 1.0f);

    void SetParameterValue(int index, float value, float weight = 1.0f);

    void AddParameterValue(const char* id, float value);

    void AddParameterValue(int index, float value);

    void SetAndSaveParameterValue(const char* id, float value, float weight = 1.0f);

    void SetAndSaveParameterValue(int index, float value, float weight = 1.0f);

    void AddAndSaveParameterValue(const char* id, float value);

    void AddAndSaveParameterValue(int index, float value);

    void LoadParameters();

    void SaveParameters();

    // transform
    void Resize(int width, int height);

    void SetOffset(float x, float y);

    void Rotate(float angle);

    void SetScale(float scale);

    void SetScaleX(float scaleX);

    void SetScaleY(float scaleY);

    const float* GetMvp();

    // motion
    void StartMotion(const char* group, int no, int priority = 3, void* startCallee = nullptr,
                     ACubismMotion::BeganMotionCallback startCalleeHandler = nullptr,
                     void* finishCallee = nullptr,
                     ACubismMotion::FinishedMotionCallback finishCalleeHandler = nullptr);

    void StartRandomMotion(const char* group = nullptr, int priority = 3,
                           void* startCallee = nullptr,
                           ACubismMotion::BeganMotionCallback startCalleeHandler = nullptr,
                           void* finishCallee = nullptr,
                           ACubismMotion::FinishedMotionCallback finishCalleeHandler = nullptr);

    bool IsMotionFinished();

    int LoadExtraMotion(const char* group, const char* motionJsonPath);

    int GetMotionGroupCount();

    int GetMotionCount(const char* group);

    void GetMotions(void* collector, void (*collect)(void* collector, const char* group, int no,
                                                     const char* file, const char* sound));

    // reset motions
    void StopAllMotions();

    void ResetAllParameters();

    void ResetPose();

    // mouse interaction
    void HitPart(float x, float y, void* collector,
                 void (*collect)(void* collector, const char* id), bool topOnly = false);

    void HitDrawable(float x, float y, void* collector,
                     void (*collect)(void* collector, const char* id), bool topOnly = false);

    void Drag(float x, float y);

    bool IsAreaHit(const char* areaName, float x, float y);

    bool IsPartHit(int index, float x, float y);

    bool IsDrawableHit(int index, float x, float y);

    // rendering
    void CreateRenderer(int maskBufferCount = 1);

    void DestroyRenderer();

    void Draw();

    // part
    const int GetPartCount() const;
    void GetPartIds(void* collector, void (*collect)(void* collector, const char* id)) const;
    void SetPartOpacity(int index, float opacity);
    void SetPartScreenColor(int index, float r, float g, float b, float a);
    void SetPartMultiplyColor(int index, float r, float g, float b, float a);
    void GetPartScreenColor(int index, float& r, float& g, float& b, float& a) const;
    void GetPartMultiplyColor(int index, float& r, float& g, float& b, float& a) const;

    // drawable
    int GetDrawableCount();
    void GetDrawableIds(void* collector, void (*collect)(void* collector, const char* id));

    const float* GetDrawableVertices(int index);
    const int GetDrawableVertexCount(int index);
    const int GetDrawableVertexIndexCount(int index);
    const unsigned short* GetDrawableIndices(int index);

    void SetDrawableMultiColor(int index, float r, float g, float b, float a);
    void SetDrawableScreenColor(int index, float r, float g, float b, float a);

    // expression
    void AddExpression(const char* expressionId);

    void RemoveExpression(const char* expressionId);

    void SetExpression(const char* expressionId);

    const char* SetRandomExpression();

    void ResetExpressions();

    void ResetExpression();

    int GetExpressionCount();

    void GetExpressions(void* collector,
                        void (*collect)(void* collector, const char* id, const char* file));

    void LoadExtraExpression(const char* expressionId, const char* expressionJsonPath);

    // sizes
    void GetCanvasSize(float& w, float& h);

    void GetCanvasSizePixel(float& w, float& h);

    float GetPixelsPerUnit();

    void SetAutoBlink(bool on);

    void SetAutoBreath(bool on);

    bool HasMocConsistencyFromFile(const char* mocFileName);

private:
    void ReleaseMotions();

    void ReleaseExpressions();

    void ReleaseExpressionManagers();

    void SetupTextures();

    void PreloadMotionGroup(const csmChar* group);

    void SetupModel();

    bool IsHit(CubismIdHandle drawableId, csmFloat32 pointX, csmFloat32 pointY) override;

    const int* GetDrawableRenderOrders() const;

private:
    ICubismModelSetting* mModelSetting;
    csmVector<CubismIdHandle> mEyeBlinkIds;
    csmVector<CubismIdHandle> mLipSyncIds;

    csmString mModelHomeDir;
    csmMap<Csm::csmString, ACubismMotion*> mMotions;
    csmMap<Csm::csmString, ACubismMotion*> mExpressions;
    std::unordered_map<std::string, CubismExpressionMotionManager*> mExpManagers;


    const Csm::CubismId* mIdParamAngleX;
    const Csm::CubismId* mIdParamAngleY;
    const Csm::CubismId* mIdParamAngleZ;
    const Csm::CubismId* mIdParamBodyAngleX;
    const Csm::CubismId* mIdParamEyeBallX;
    const Csm::CubismId* mIdParamEyeBallY;

    int mParamAngleXi;
    int mParamAngleYi;
    int mParamAngleZi;
    int mParamBodyAngleXi;
    int mParamEyeBallXi;
    int mParamEyeBallYi;

    LAppTextureManager mTextureManager;

    MatrixManagerV3 mMatrixManager;

    csmFloat32 mDragX;
    csmFloat32 mDragY;

    int* mTmpOrderedDrawIndice;
    const float* mParameterDefaultValues;
    float* mParameterValues;
    int mParameterCount;

    std::vector<csmString> mMotionGroupNames;
    std::vector<int> mMotionCounts;

    std::vector<float> mSavedParameterValues;

    bool autoBreath;
    bool autoBlink;
};
}   // namespace V3
}   // namespace Live2D