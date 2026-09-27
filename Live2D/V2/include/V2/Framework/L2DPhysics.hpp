#pragma once
#include "PhysicsHair.hpp"
#include <cstdint>
#include <memory>
#include <vector>
namespace Live2D {
namespace V2 {
class ModelContext;
class L2DPhysics {
public:
    L2DPhysics();
    // 时间由外部传入（时钟只由 Model 管理）: dtMs 为本次时间步长（毫秒），内部累计 elapsed
    void updateParam(ModelContext* context, long long dtMs);
    static L2DPhysics* load(const std::vector<uint8_t>& data);
    std::vector<std::unique_ptr<PhysicsHair>> mPhysicsList;

private:
    long long mElapsedMs = 0;   // dt 累计
};
}   // namespace V2
}   // namespace Live2D
