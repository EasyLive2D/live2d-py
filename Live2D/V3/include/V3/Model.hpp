/**
 * @brief Model.hpp
 * @author Arkueid
 * @date 2025/04/06
 * @note 更细粒度、更独立的 live2d 模型管理类
 */

#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>


#include <IModel.hpp>

#include <CubismUserModelProxy.hpp>
#include <Motion/ACubismMotion.hpp>

#include <LAppTextureManager.hpp>
#include <MatrixManagerV3.hpp>

using namespace Csm;

namespace Live2D {
namespace V3 {

class Model : public IModel {
public:
    Model();
    ~Model() override;

    /**
     * @brief
     * @param filePath model3.json path
     */
    void LoadModelJson(const char* filePath, bool createRenderer = true) override;

    const char* GetModelHomeDir() override;

    // 版本
    int Version() const override;

    // update

    void Update(float deltaSecs) override;

    /**
     * @brief
     * @param deltaSecs time elapsed since last frame
     * @return true if motion is not finished and motion is updated
     */
    bool UpdateMotion(float deltaSecs) override;

    void UpdateDrag(float deltaSecs) override;

    void UpdateBreath(float deltaSecs) override;

    void UpdateBlink(float deltaSecs) override;

    void UpdateExpression(float deltaSecs) override;

    void UpdatePhysics(float deltaSecs) override;

    void UpdatePose(float deltaSecs) override;

    // param
    int GetParameterCount() override;

    void GetParameterIds(void* collector, void (*collect)(void* collector, const char* id)) override;

    const char* GetParameterId(int index) override;

    float GetParameterValue(int index) override;

    float GetParameterMaximumValue(int index) override;

    float GetParameterMinimumValue(int index) override;

    float GetParameterDefaultValue(int index) override;

    void SetParameterValue(const char* id, float value, float weight = 1.0f) override;

    void SetParameterValue(int index, float value, float weight = 1.0f) override;

    void AddParameterValue(const char* id, float value) override;

    void AddParameterValue(int index, float value) override;

    void SetAndSaveParameterValue(const char* id, float value, float weight = 1.0f) override;

    void SetAndSaveParameterValue(int index, float value, float weight = 1.0f) override;

    void AddAndSaveParameterValue(const char* id, float value) override;

    void AddAndSaveParameterValue(int index, float value) override;

    void LoadParameters() override;

    void SaveParameters() override;

    // transform
    void Resize(int width, int height) override;

    void SetOffset(float x, float y) override;

    void Rotate(float angle) override;

    void SetScale(float scale) override;

    void SetScaleX(float scaleX) override;

    void SetScaleY(float scaleY) override;

    const float* GetMvp() override;

    // motion
    void StartMotion(const std::string& group, int no, int priority = 3,
                     MotionCallback onStart = nullptr, MotionCallback onFinish = nullptr) override;

    void StartRandomMotion(const std::string& group = "", int priority = 3,
                           MotionCallback onStart = nullptr,
                           MotionCallback onFinish = nullptr) override;

    bool IsMotionFinished() override;

    int LoadExtraMotion(const char* group, const char* motionJsonPath) override;

    int GetMotionGroupCount() override;

    int GetMotionCount(const char* group) override;

    void GetMotions(void* collector, void (*collect)(void* collector, const char* group, int no,
                                                     const char* file, const char* sound)) override;

    // reset motions
    void StopAllMotions() override;

    void ResetAllParameters() override;

    void ResetPose() override;

    // mouse interaction
    void HitPart(float x, float y, void* collector,
                 void (*collect)(void* collector, const char* id), bool topOnly = false) override;

    void HitDrawable(float x, float y, void* collector,
                     void (*collect)(void* collector, const char* id),
                     bool topOnly = false) override;

    void Drag(float x, float y) override;

    bool IsAreaHit(const char* areaName, float x, float y) override;

    bool IsPartHit(int index, float x, float y) override;

    bool IsDrawableHit(int index, float x, float y) override;

    // rendering
    void CreateRenderer(int maskBufferCount = 1) override;

    void DestroyRenderer() override;

    void Draw() override;

    // part
    int GetPartCount() const override;
    void GetPartIds(void* collector, void (*collect)(void* collector, const char* id)) const override;
    const char* GetPartId(int index) const override;
    void SetPartOpacity(int index, float opacity) override;
    void SetPartScreenColor(int index, float r, float g, float b, float a) override;
    void SetPartMultiplyColor(int index, float r, float g, float b, float a) override;
    void GetPartScreenColor(int index, float& r, float& g, float& b, float& a) const override;
    void GetPartMultiplyColor(int index, float& r, float& g, float& b, float& a) const override;

    // drawable
    int GetDrawableCount() override;
    void GetDrawableIds(void* collector, void (*collect)(void* collector, const char* id)) override;

    const float* GetDrawableVertices(int index) override;
    int GetDrawableVertexCount(int index) override;
    int GetDrawableVertexIndexCount(int index) override;
    const unsigned short* GetDrawableIndices(int index) override;

    void SetDrawableMultiColor(int index, float r, float g, float b, float a) override;
    void SetDrawableScreenColor(int index, float r, float g, float b, float a) override;

    // expression
    void AddExpression(const char* expressionId) override;

    void RemoveExpression(const char* expressionId) override;

    void SetExpression(const char* expressionId) override;

    const char* SetRandomExpression() override;

    void ResetExpressions() override;

    void ResetExpression() override;

    int GetExpressionCount() override;

    void GetExpressions(void* collector,
                        void (*collect)(void* collector, const char* id, const char* file)) override;

    void LoadExtraExpression(const char* expressionId, const char* expressionJsonPath) override;

    // sizes
    void GetCanvasSize(float& w, float& h) override;

    void GetCanvasSizePixel(float& w, float& h) override;

    float GetPixelsPerUnit() override;

    void SetAutoBlink(bool on) override;

    void SetAutoBreath(bool on) override;

    bool AutoBreathEnabled() const override;

    bool AutoBlinkEnabled() const override;

    bool HasMocConsistencyFromFile(const char* mocFileName) override;

private:
    void ReleaseMotions();

    void ReleaseExpressions();

    void ReleaseExpressionManagers();

    void SetupTextures();

    void PreloadMotionGroup(const csmChar* group);

    void SetupModel();

    const int* GetDrawableRenderOrders() const;

private:
    CubismUserModelProxy mProxy;   // 必须声明在其它成员之前（先构造、后析构，等价原基类顺序）
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

    // Update() 哨兵路径（deltaSecs<0）的内部自计时时间戳
    std::chrono::steady_clock::time_point mLastUpdatePoint{};
};
}   // namespace V3
}   // namespace Live2D
