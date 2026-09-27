#pragma once
#include "L2DBaseModel.hpp"
#include "L2DTargetPoint.hpp"
#include "MatrixManagerV2.hpp"
#include "GLRenderer.hpp"
#include <IModel.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>


namespace Live2D {
namespace V2 {
class Model : public L2DBaseModel {
public:
    using StartCallback = std::function<void(const std::string&, int)>;
    using FinishCallback = std::function<void(const std::string&, int)>;

    Model();
    ~Model() override;

    // ---- IModel ----
    void LoadModelJson(const char* path, bool createRenderer = true) override;
    const char* GetModelHomeDir() override;

    // 版本
    int Version() const override;

    void Update(float deltaSecs = 0.016f) override;
    bool UpdateMotion(float deltaSecs) override;
    void UpdateDrag(float deltaSecs) override;
    void UpdateBreath(float deltaSecs) override;
    void UpdateBlink(float deltaSecs) override;
    void UpdateExpression(float deltaSecs) override;
    void UpdatePhysics(float deltaSecs) override;
    void UpdatePose(float deltaSecs) override;

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

    void Resize(int w, int h) override;
    void SetOffset(float dx, float dy) override;
    void Rotate(float deg) override;
    void SetScale(float s) override;
    void SetScaleX(float scaleX) override;
    void SetScaleY(float scaleY) override;
    const float* GetMvp() override;

    void StartMotion(const std::string& group, int no, int priority = 3,
                     MotionCallback onStart = nullptr,
                     MotionCallback onFinish = nullptr) override;
    void StartRandomMotion(const std::string& group = "", int priority = 3,
                           MotionCallback onStart = nullptr,
                           MotionCallback onFinish = nullptr) override;
    bool IsMotionFinished() override;
    int LoadExtraMotion(const char* group, const char* motionJsonPath) override;
    int GetMotionGroupCount() override;
    int GetMotionCount(const char* group) override;
    void GetMotions(void* collector,
                    void (*collect)(void* collector, const char* group, int no, const char* file,
                                    const char* sound)) override;
    void StopAllMotions() override;
    void ResetAllParameters() override;
    void ResetPose() override;

    void HitPart(float x, float y, void* collector, void (*collect)(void* collector, const char* id),
                 bool topOnly = false) override;
    void HitDrawable(float x, float y, void* collector,
                     void (*collect)(void* collector, const char* id),
                     bool topOnly = false) override;
    void Drag(float x, float y) override;
    bool IsAreaHit(const char* area, float x, float y) override;
    bool IsPartHit(int index, float x, float y) override;
    bool IsDrawableHit(int index, float x, float y) override;

    void CreateRenderer(int maskBufferCount = 1) override;
    void DestroyRenderer() override;
    void Draw() override;

    int GetPartCount() const override;
    void GetPartIds(void* collector, void (*collect)(void* collector, const char* id)) const override;
    const char* GetPartId(int index) const override;
    void SetPartOpacity(int index, float val) override;
    void SetPartScreenColor(int index, float r, float g, float b, float a) override;
    void SetPartMultiplyColor(int index, float r, float g, float b, float a) override;
    void GetPartScreenColor(int index, float& r, float& g, float& b, float& a) const override;
    void GetPartMultiplyColor(int index, float& r, float& g, float& b, float& a) const override;

    int GetDrawableCount() override;
    void GetDrawableIds(void* collector, void (*collect)(void* collector, const char* id)) override;
    const float* GetDrawableVertices(int index) override;
    int GetDrawableVertexCount(int index) override;
    int GetDrawableVertexIndexCount(int index) override;
    const unsigned short* GetDrawableIndices(int index) override;
    void SetDrawableMultiColor(int index, float r, float g, float b, float a) override;
    void SetDrawableScreenColor(int index, float r, float g, float b, float a) override;

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

    void GetCanvasSize(float& w, float& h) override;
    void GetCanvasSizePixel(float& w, float& h) override;
    float GetPixelsPerUnit() override;

    void SetAutoBreath(bool v) override;
    void SetAutoBlink(bool v) override;
    bool AutoBreathEnabled() const override;
    bool AutoBlinkEnabled() const override;

    bool HasMocConsistencyFromFile(const char* mocFileName) override;

private:
    struct MotionInfo {
        std::string file;
        std::string sound;
    };
    // wantDrawableId=false 返回 part id，true 返回 drawData id
    std::vector<std::string> hitIds(float x, float y, bool topOnly, bool wantDrawableId);

    L2DTargetPoint mDragMgr;
    MatrixManagerV2 mMatrixManager;
    bool mAutoBreath = true, mAutoBlink = true;
    bool mClearFlag = false;
    std::string mModelHomeDir;

    StartCallback mOnStartMotion;
    FinishCallback mOnFinishMotion;
    bool mCallbacksPending = false;
    std::string mCurrentGroup;
    std::vector<std::string> mTexturePaths;
    std::unordered_map<std::string, std::vector<MotionInfo>> mMotionInfos;
    std::unordered_map<std::string, std::string> mExpressionFiles;
    int mCurrentMotionNo = 0;
    std::unique_ptr<GLRenderer> mRenderer;
    float mMvpCache[16] = {};
    std::vector<float> mDrawableVertexCache;
    std::vector<unsigned short> mDrawableIndexCache;
};
}   // namespace V2
}   // namespace Live2D
