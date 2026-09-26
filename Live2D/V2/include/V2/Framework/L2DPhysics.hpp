#pragma once
#include "PhysicsHair.hpp"
#include <cstdint>
#include <memory>
#include <vector>
namespace Live2D {
namespace V2 {
class ModelContext;
class L2DPhysics
{
public:
    L2DPhysics();
    void updateParam(ModelContext* context);
    static L2DPhysics* load(const std::vector<uint8_t>& data);
    std::vector<std::unique_ptr<PhysicsHair>> mPhysicsList;

private:
    long long mStartTimeMs = 0;
};
}   // namespace V2
}   // namespace Live2D
