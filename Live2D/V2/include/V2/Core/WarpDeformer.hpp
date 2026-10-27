#pragma once
#include <memory>

#include "Deformer.hpp"
#include <vector>

namespace Live2D {
namespace V2 {

class PivotManager;

class WarpDeformer final : public Deformer {
public:
    WarpDeformer() = default;
    ~WarpDeformer() override;

    int GetType() const override { return TYPE_WARP; }
    void Read(class BinaryReader& br) override;
    void InitRuntimeState() override;
    void SetupInterpolate(Moc* moc) override;
    bool SetupTransform(Moc* moc) override;
    void TransformPoints(const std::vector<float>& srcPoints, std::vector<float>& dstPoints,
                         int numPoint, int ptOffset, int ptStep) override;

    int GetPointCount() const { return (mRow + 1) * (mCol + 1); }

    static void TransformPointsInner(const std::vector<float>& src, std::vector<float>& dst,
                                     int pointCount, int srcOffset, int srcStep,
                                     const std::vector<float>& grid, int row, int col);

    int mRow = 0;
    int mCol = 0;
    std::unique_ptr<PivotManager> mPivotMgr;
    std::vector<std::vector<float>> mPivotPoints;

    // ---- 运行时状态（原 WarpContext）----
    std::vector<float> mInterpolatedPoints;
    std::vector<float> mTransformedPoints;
};

}   // namespace V2
}   // namespace Live2D
