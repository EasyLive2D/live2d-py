#pragma once

#include "ISerializable.hpp"
#include "Id.hpp"
#include <vector>

namespace Live2D {
namespace V2 {

class Moc;
class PivotManager;

/**
 * Deformer — deformer 数据与运行时状态一体（原 DeformerContext 已合并入本类）。
 * Read() 填充静态数据；InitRuntimeState() 在 Moc::init 中调用，初始化运行时状态；
 * 每帧经 setupInterpolate/setupTransform 更新运行时状态。
 */
class Deformer : public ISerializable {
public:
    static constexpr int DEFORMER_INDEX_NOT_INIT = -2;
    static constexpr int TYPE_ROTATION = 1;
    static constexpr int TYPE_WARP = 2;

    Deformer() = default;

    void Read(class BinaryReader& br) override;
    void ReadOpacity(class BinaryReader& br);

    /// 初始化运行时状态（原 Context 构造逻辑下沉，Moc::init 阶段调用一次）
    virtual void InitRuntimeState() = 0;
    virtual void SetupInterpolate(Moc* moc) = 0;
    virtual void InterpolateOpacity(Moc* moc, PivotManager* pivotMgr, bool& ret);
    virtual bool SetupTransform(Moc* moc) = 0;
    virtual void TransformPoints(const std::vector<float>& srcPoints, std::vector<float>& dstPoints,
                                 int numPoint, int ptOffset, int ptStep) = 0;
    virtual int GetType() const = 0;

    void SetTargetId(const Id* targetId) { mTargetId = targetId; }
    void SetId(const Id* idVal) { mId = idVal; }
    const Id* GetTargetId() const { return mTargetId; }
    const Id* GetId() const { return mId; }
    bool NeedTransform() const;

    // ---- 运行时状态（原 DeformerContext）----
    bool IsAvailable() const { return mAvailable; }
    void SetAvailable(bool v) { mAvailable = v; }

    bool IsParamOutOfRange() const { return mOutOfRange; }
    void SetParamOutOfRange(bool v) { mOutOfRange = v; }

    float GetTotalScale() const { return mTotalScale; }
    void SetTotalScale_notForClient(float v) { mTotalScale = v; }

    float GetTotalOpacity() const { return mTotalOpacity; }
    void SetTotalOpacity(float v) { mTotalOpacity = v; }

    float GetInterpolatedOpacity() const { return mInterpolatedOpacity; }
    void SetInterpolatedOpacity(float v) { mInterpolatedOpacity = v; }

    int mTmpDeformerIndex = DEFORMER_INDEX_NOT_INIT;
    int mPartsIndex = 0;

protected:
    const Id* mId = nullptr;
    const Id* mTargetId = nullptr;
    bool mDirty = true;
    std::vector<float> mPivotOpacities;

    bool mAvailable = true;
    bool mOutOfRange = false;
    float mTotalScale = 1.0f;
    float mTotalOpacity = 1.0f;
    float mInterpolatedOpacity = 1.0f;
};

}   // namespace V2
}   // namespace Live2D
