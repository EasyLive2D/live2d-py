#pragma once
#include <vector>
#include "../Motion/AMotion.hpp"
#include "L2DExpressionParam.hpp"
namespace live2d {
class L2DExpressionMotion : public AMotion {
public:
    L2DExpressionMotion();
    void updateParam(ALive2DModel* model, float timeSec, float weight) override;
    float getDurationSec() const override { return 1.0f; }
    bool isLoop() const override { return false; }
    bool isFinished() const override { return mFinished; }
    static L2DExpressionMotion* load(const std::vector<uint8_t>& data);
    std::vector<L2DExpressionParam> mParams;
private:
    bool mFinished = false;
};
} // namespace live2d
