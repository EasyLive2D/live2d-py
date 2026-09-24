#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "PhysicsHair.hpp"
namespace live2d {
class ModelContext;
class L2DPhysics {
public:
    L2DPhysics();
    void updateParam(ModelContext* context);
    static L2DPhysics* load(const std::vector<uint8_t>& data);
    std::vector<std::unique_ptr<PhysicsHair>> mPhysicsList;
private:
    long long mStartTimeMs = 0;
};
} // namespace live2d
