/**
 * @brief IModel.hpp
 * @author Arkueid
 * @date 2026/09/27
 * @note 统一模型接口
 */

#pragma once

#include <functional>
#include <string>

namespace Live2D {

/**
 * @brief V2 / V3 模型统一接口（以 V3 暴露给 Python 的 API 为基准）。
 *
 * 命名采用 PascalCase；字符串参数为 const char*；批量枚举用
 * collector 回调；动作回调为 std::function。V2 暂未实现的部分
 * 以 LOGE + abort 的 stub 提供。
 */
class IModel {
public:
    virtual ~IModel() = default;

    // 动作回调: group / no
    using MotionCallback = std::function<void(const std::string& group, int no)>;

    // ---- 加载 ----
    virtual void LoadModelJson(const char* filePath, bool createRenderer = true) = 0;
    virtual const char* GetModelHomeDir() = 0;

    // ---- 更新 ----
    // deltaSecs 为哨兵: <0（默认，不传）走墙钟自适配（V2 Python 1:1 墙钟路径 / V3 内部自计时），
    // >=0 全 delta 驱动（可暂停/变速/确定性测试）
    virtual void Update(float deltaSecs = -1.0f) = 0;
    virtual bool UpdateMotion(float deltaSecs) = 0;
    virtual void UpdateDrag(float deltaSecs) = 0;
    virtual void UpdateBreath(float deltaSecs) = 0;
    virtual void UpdateBlink(float deltaSecs) = 0;
    virtual void UpdateExpression(float deltaSecs) = 0;
    virtual void UpdatePhysics(float deltaSecs) = 0;
    virtual void UpdatePose(float deltaSecs) = 0;

    // ---- 参数 ----
    virtual int GetParameterCount() = 0;
    virtual void GetParameterIds(void* collector,
                                 void (*collect)(void* collector, const char* id)) = 0;
    virtual const char* GetParameterId(int index) = 0;
    virtual float GetParameterValue(int index) = 0;
    virtual float GetParameterMaximumValue(int index) = 0;
    virtual float GetParameterMinimumValue(int index) = 0;
    virtual float GetParameterDefaultValue(int index) = 0;
    virtual void SetParameterValue(const char* id, float value, float weight = 1.0f) = 0;
    virtual void SetParameterValue(int index, float value, float weight = 1.0f) = 0;
    virtual void AddParameterValue(const char* id, float value) = 0;
    virtual void AddParameterValue(int index, float value) = 0;
    virtual void SetAndSaveParameterValue(const char* id, float value, float weight = 1.0f) = 0;
    virtual void SetAndSaveParameterValue(int index, float value, float weight = 1.0f) = 0;
    virtual void AddAndSaveParameterValue(const char* id, float value) = 0;
    virtual void AddAndSaveParameterValue(int index, float value) = 0;
    virtual void LoadParameters() = 0;
    virtual void SaveParameters() = 0;

    // ---- 变换 ----
    virtual void Resize(int width, int height) = 0;
    virtual void SetOffset(float x, float y) = 0;
    virtual void SetOffsetX(float x) = 0;
    virtual void SetOffsetY(float y) = 0;
    virtual void Rotate(float angle) = 0;
    virtual void SetScale(float scale) = 0;
    virtual void SetScaleX(float scaleX) = 0;
    virtual void SetScaleY(float scaleY) = 0;
    virtual const float* GetMvp() = 0;

    // ---- 动作 ----
    virtual void StartMotion(const std::string& group,
                             int no,
                             int priority = 3,
                             MotionCallback onStart = nullptr,
                             MotionCallback onFinish = nullptr) = 0;
    virtual void StartRandomMotion(const std::string& group = "",
                                   int priority = 3,
                                   MotionCallback onStart = nullptr,
                                   MotionCallback onFinish = nullptr) = 0;
    virtual bool IsMotionFinished() = 0;
    virtual int LoadExtraMotion(const char* group, const char* motionJsonPath) = 0;
    virtual int GetMotionGroupCount() = 0;
    virtual int GetMotionCount(const char* group) = 0;
    // 返回指定动作的音效文件路径（无音效返回 ""）
    virtual const char* GetMotionSound(const char* group, int no) = 0;
    virtual void GetMotions(void* collector,
                            void (*collect)(void* collector,
                                            const char* group,
                                            int no,
                                            const char* file,
                                            const char* sound)) = 0;
    virtual void StopAllMotions() = 0;
    virtual void ResetAllParameters() = 0;
    virtual void ResetPose() = 0;

    // ---- 鼠标交互 ----
    virtual void HitPart(float x,
                         float y,
                         void* collector,
                         void (*collect)(void* collector, const char* id),
                         bool topOnly = false) = 0;
    virtual void HitDrawable(float x,
                             float y,
                             void* collector,
                             void (*collect)(void* collector, const char* id),
                             bool topOnly = false) = 0;
    virtual void Drag(float x, float y) = 0;
    virtual bool IsAreaHit(const char* areaName, float x, float y) = 0;
    virtual bool IsPartHit(int index, float x, float y) = 0;
    virtual bool IsDrawableHit(int index, float x, float y) = 0;

    // ---- 渲染 ----
    virtual void CreateRenderer(int maskBufferCount = 1) = 0;
    virtual void DestroyRenderer() = 0;
    virtual void Draw() = 0;

    // ---- 部件 ----
    virtual int GetPartCount() const = 0;
    virtual void GetPartIds(void* collector,
                            void (*collect)(void* collector, const char* id)) const = 0;
    virtual const char* GetPartId(int index) const = 0;
    virtual void SetPartOpacity(int index, float opacity) = 0;
    virtual void SetPartScreenColor(int index, float r, float g, float b, float a) = 0;
    virtual void SetPartMultiplyColor(int index, float r, float g, float b, float a) = 0;
    virtual void GetPartScreenColor(int index, float& r, float& g, float& b, float& a) const = 0;
    virtual void GetPartMultiplyColor(int index,
                                      float& r,
                                      float& g,
                                      float& b,
                                      float& a) const = 0;

    // ---- drawable ----
    virtual int GetDrawableCount() = 0;
    virtual void GetDrawableIds(void* collector,
                                void (*collect)(void* collector, const char* id)) = 0;
    virtual const float* GetDrawableVertices(int index) = 0;
    virtual int GetDrawableVertexCount(int index) = 0;
    virtual int GetDrawableVertexIndexCount(int index) = 0;
    virtual const unsigned short* GetDrawableIndices(int index) = 0;
    virtual void SetDrawableMultiColor(int index, float r, float g, float b, float a) = 0;
    virtual void SetDrawableScreenColor(int index, float r, float g, float b, float a) = 0;

    // ---- 表情 ----
    virtual void AddExpression(const char* expressionId) = 0;
    virtual void RemoveExpression(const char* expressionId) = 0;
    // fadeoutMs >= 0: 临时表情，fadeoutMs 毫秒后自动恢复上一个表情;
    // fadeoutMs < 0（默认）: 持久表情
    virtual void SetExpression(const char* expressionId, float fadeoutMs = -1.0f) = 0;
    virtual const char* SetRandomExpression(float fadeoutMs = -1.0f) = 0;
    virtual void ResetExpressions() = 0;
    virtual void ResetExpression() = 0;
    virtual int GetExpressionCount() = 0;
    virtual void GetExpressions(void* collector,
                                void (*collect)(void* collector,
                                                const char* id,
                                                const char* file)) = 0;
    virtual void LoadExtraExpression(const char* expressionId, const char* expressionJsonPath) = 0;

    // ---- 尺寸 ----
    virtual void GetCanvasSize(float& w, float& h) = 0;
    virtual void GetCanvasSizePixel(float& w, float& h) = 0;
    virtual float GetPixelsPerUnit() = 0;

    // ---- 自动机制 ----
    virtual void SetAutoBreath(bool on) = 0;
    virtual void SetAutoBlink(bool on) = 0;
    virtual bool AutoBreathEnabled() const = 0;
    virtual bool AutoBlinkEnabled() const = 0;

    // ---- 版本 ----
    virtual int Version() const = 0;
    virtual bool IsV2() const { return Version() == 2; }
    virtual bool IsV3() const { return Version() == 3; }

    virtual bool HasMocConsistencyFromFile(const char* mocFileName) = 0;
};
}   // namespace Live2D
