#pragma once

#include "Deformer.hpp"
#include <vector>

namespace live2d {

class PivotManager;
class AffineEnt;

class RotationDeformer final : public Deformer {
public:
    RotationDeformer() = default;

    int getType() const override { return TYPE_ROTATION; }
    void read(class BinaryReader& br) override;
    DeformerContext* init(ModelContext* mc) override;
    void setupInterpolate(ModelContext* mc, DeformerContext* dc) override;
    bool setupTransform(ModelContext* mc, DeformerContext* dc) override;
    void transformPoints(ModelContext* mc, DeformerContext* dc,
                         const std::vector<float>& srcPoints, std::vector<float>& dstPoints,
                         int numPoint, int ptOffset, int ptStep) override;

    PivotManager* mPivotManager = nullptr;
    std::vector<AffineEnt*> mAffines;
};

} // namespace live2d
