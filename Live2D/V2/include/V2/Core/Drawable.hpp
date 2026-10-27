#pragma once
#include <array>
#include <memory>

#include "ISerializable.hpp"
#include "Id.hpp"
#include <vector>

namespace Live2D {
namespace V2 {

class PivotManager;
class Moc;

/**
 * Drawable — drawable 数据与运行时状态一体（原 IDrawData + Mesh 合并，
 * V2 仅有 mesh 一种 draw data 实现）。对标 V3 CubismModel 的 drawable 概念。
 */
class Drawable final : public ISerializable {
public:
    static constexpr int DEFORMER_INDEX_NOT_INIT = -2;
    static constexpr int DEFAULT_ORDER = 500;
    static constexpr int MASK_COLOR_COMPOSITION = 30;
    static constexpr int COLOR_COMPOSITION_NORMAL = 0;
    static constexpr int COLOR_COMPOSITION_SCREEN = 1;
    static constexpr int COLOR_COMPOSITION_MULTIPLY = 2;

    Drawable() = default;
    ~Drawable() override;

    void Read(class BinaryReader& br) override;

    void SetupInterpolate(Moc* moc);
    void SetupTransform(Moc* moc);

    /// 初始化运行时状态（顶点缓冲分配与 UV 修正，Moc::init 阶段调用一次）
    void InitRuntimeState();

    const Id* GetId() const { return mId; }
    void SetId(const Id* value) { mId = value; }
    const Id* GetTargetId() const { return mTargetId; }
    void SetTargetId(const Id* value) { mTargetId = value; }
    bool NeedTransform() const { return mTargetId != nullptr && mTargetId != Id::DST_BASE_ID(); }

    float GetOpacity() const { return mInterpolatedOpacity; }
    int GetDrawOrder() const { return mInterpolatedDrawOrder; }

    const std::vector<Id*>& GetClipIDList() const { return mClipIDList; }
    /// moc 键位 draw order 表（Read 阶段静态数据，order 插值输出落在其 [min, max] 内）
    const std::vector<int>& GetPivotDrawOrders() const { return mPivotDrawOrders; }

    /// drawable 级颜色覆盖（SDK 层扩展，V2 moc 格式无颜色数据；语义对齐 V3
    /// per-drawable override：覆盖时替换所属 part 颜色，仅影响本 drawable）
    void SetOverrideMultiplyColor(float r, float g, float b, float a) {
        mOverrideMultiplyColor = {r, g, b, a};
        mHasColorOverride = true;
    }
    void SetOverrideScreenColor(float r, float g, float b, float a) {
        mOverrideScreenColor = {r, g, b, a};
        mHasColorOverride = true;
    }
    bool HasColorOverride() const { return mHasColorOverride; }
    /// 清除覆盖，回落所属 part 颜色（后写的 SetPart*Color 覆盖先写的
    /// SetDrawable*Color，语义对齐 V3 SetPartColor 的下推覆盖）
    void ClearColorOverride() { mHasColorOverride = false; }

    void SetTextureNo(int texNo) { mTextureNo = texNo; }
    int GetTextureNo() const { return mTextureNo; }
    int GetColorCompositionType() const { return mColorCompositionType; }
    bool GetCulling() const { return mCulling; }
    const std::vector<float>& GetUvs() const { return mUvs; }
    int GetNumPoints() const { return mPointCount; }
    const std::vector<int16_t>& GetIndexArray() const { return mIndexArray; }

    // ---- 运行时状态 ----
    std::vector<float> mInterpolatedPoints;
    std::vector<float> mTransformedPoints;
    std::array<float, 4> mOverrideMultiplyColor = {1.0f, 1.0f, 1.0f, 0.0f};
    std::array<float, 4> mOverrideScreenColor = {0.0f, 0.0f, 0.0f, 0.0f};
    bool mHasColorOverride = false;
    float mBaseOpacity = 1.0f;
    float mInterpolatedOpacity = 1.0f;
    int mInterpolatedDrawOrder = 0;
    int mPartsIndex = 0;
    int mTmpDeformerIndex = DEFORMER_INDEX_NOT_INIT;
    bool mAvailable = true;
    bool mParamOutOfRange = false;

private:
    const Id* mId = nullptr;
    const Id* mTargetId = nullptr;
    std::unique_ptr<PivotManager> mPivotMgr;
    int mAverageDrawOrder = 0;
    std::vector<int> mPivotDrawOrders;
    std::vector<float> mPivotOpacities;
    std::vector<Id*> mClipIDList;

    int mTextureNo = -1;
    int mPointCount = 0;
    int mPolygonCount = 0;
    int mOptionFlag = 0;
    std::vector<int16_t> mIndexArray;
    std::vector<std::vector<float>> mPivotPoints;
    std::vector<float> mUvs;
    int mColorCompositionType = COLOR_COMPOSITION_NORMAL;
    bool mCulling = true;
};

}   // namespace V2
}   // namespace Live2D
