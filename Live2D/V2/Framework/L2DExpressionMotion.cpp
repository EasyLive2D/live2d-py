#include "L2DExpressionMotion.hpp"
namespace live2d {
L2DExpressionMotion::L2DExpressionMotion() = default;
void L2DExpressionMotion::updateParam(ALive2DModel* model, float timeSec, float weight) { (void)model; (void)timeSec; (void)weight; }
L2DExpressionMotion* L2DExpressionMotion::load(const std::vector<uint8_t>& data) { (void)data; return new L2DExpressionMotion(); }
} // namespace live2d
