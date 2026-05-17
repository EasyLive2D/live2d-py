#pragma once

#include "Deformer.hpp"
#include <vector>

namespace live2d {

class PivotManager;

class WarpDeformer final : public Deformer {
public:
    WarpDeformer() = default;

    int getType() const override { return TYPE_WARP; }
    void read(class BinaryReader& br) override;
    DeformerContext* init(ModelContext* mc) override;
    void setupInterpolate(ModelContext* mc, DeformerContext* dc) override;
    bool setupTransform(ModelContext* mc, DeformerContext* dc) override;
    void transformPoints(ModelContext* mc, DeformerContext* dc,
                         const std::vector<float>& srcPoints, std::vector<float>& dstPoints,
                         int numPoint, int ptOffset, int ptStep) override;

    int getPointCount() const { return (mRow + 1) * (mCol + 1); }

    static void transformPoints_sdk2(const std::vector<float>& src,
                                      std::vector<float>& dst,
                                      int pointCount, int srcOffset, int srcStep,
                                      const std::vector<float>& grid, int row, int col);

    int mRow = 0;
    int mCol = 0;
    PivotManager* mPivotMgr = nullptr;
    std::vector<std::vector<float>> mPivotPoints;
};

} // namespace live2d
