#include "L2DPhysics.hpp"
namespace live2d {
L2DPhysics::L2DPhysics() = default;
void L2DPhysics::updateParam(ALive2DModel* model) { (void)model; }
L2DPhysics* L2DPhysics::load(const std::vector<uint8_t>& data) { (void)data; return new L2DPhysics(); }
} // namespace live2d
