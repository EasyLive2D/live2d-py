#pragma once
#include <memory>

#include "ISerializable.hpp"
#include "Id.hpp"
#include <cstdint>
#include <vector>

namespace Live2D {
namespace V2 {

class Moc;

/**
 * PivotManager — pivot 插值表求值器。
 * 每个 deformer/drawable 持有一个，内含作用在其上的全部参数的 pivot
 * 键位表（Pivot），负责参数变化检测与多维插值索引计算。
 * （V3 无对应 Framework 类，等价逻辑在 Core 的 keyform 绑定求值内部。）
 */
class PivotManager final : public ISerializable {
public:
    /// 单个参数的 pivot 键位（参数 id + 分段点值 + 运行时求值缓存）
    class Pivot final : public ISerializable {
    public:
        static constexpr int PARAM_INDEX_NOT_INIT = -2;

        Pivot() = default;

        void Read(class BinaryReader& br) override;

        int GetParamIndex();
        void SetParamIndex(int index) { mParamIndex = index; }

        const Id* GetId() const { return mParamId; }
        int GetPivotCount() const { return mCount; }
        const std::vector<float>& GetValues() const { return mValues; }

        int GetCurrentPivotIndex() const { return mCurrentPivotIndex; }
        void SetCurrentPivotIndex(int index) { mCurrentPivotIndex = index; }
        float GetCurrentT() const { return mCurrentT; }
        void SetCurrentT(float value) { mCurrentT = value; }

    private:
        // 关键帧
        int mCount = 0;
        const Id* mParamId = nullptr;
        std::vector<float> mValues;
        int mParamIndex = PARAM_INDEX_NOT_INIT;
        int mInitVersion = -1;
        int mCurrentPivotIndex = 0;
        float mCurrentT = 0.0f;
    };

    PivotManager() = default;
    ~PivotManager() override;

    void Read(class BinaryReader& br) override;

    int CalcPivotValues(Moc* moc, bool& outRet /* 参数值越界 */);

    void CalcPivotIndices(std::vector<int16_t>& indexArray, std::vector<float>& tArray,
                          int interpolationCount);

    int GetParamCount() const { return static_cast<int>(mPivots.size()); }

private:
    std::vector<std::unique_ptr<Pivot>> mPivots;
};

}   // namespace V2
}   // namespace Live2D
