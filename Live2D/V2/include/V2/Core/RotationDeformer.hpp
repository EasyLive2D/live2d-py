#pragma once
#include <memory>

#include "Deformer.hpp"
#include <vector>

namespace Live2D {
namespace V2 {

class PivotManager;
class AffineEnt;

class RotationDeformer final : public Deformer {
public:
    RotationDeformer() = default;
    ~RotationDeformer() override;

    int GetType() const override { return TYPE_ROTATION; }
    void Read(class BinaryReader& br) override;
    void InitRuntimeState() override;
    void SetupInterpolate(Moc* moc) override;
    bool SetupTransform(Moc* moc) override;
    void TransformPoints(const std::vector<float>& srcPoints, std::vector<float>& dstPoints,
                         int numPoint, int ptOffset, int ptStep) override;

    std::unique_ptr<PivotManager> mPivotManager;
    std::vector<std::unique_ptr<AffineEnt>> mAffines;

    // ---- 运行时状态（原 RotationContext）----
    std::unique_ptr<AffineEnt> mInterpolatedAffine;
    std::unique_ptr<AffineEnt> mTransformedAffine;
};

}   // namespace V2
}   // namespace Live2D
