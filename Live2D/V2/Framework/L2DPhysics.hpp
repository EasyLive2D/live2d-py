#pragma once
#include <vector>
namespace live2d {
class PhysicsHair;
class ALive2DModel;
class L2DPhysics {
public:
    L2DPhysics();
    void updateParam(ALive2DModel* model);
    static L2DPhysics* load(const std::vector<uint8_t>& data);
    std::vector<PhysicsHair*> mPhysicsList;
private:
    long long mStartTimeMs = 0;
};
} // namespace live2d
