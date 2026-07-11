#pragma once
#include <string>
#include <vector>
namespace live2d {
class ALive2DModel;

enum PhysicsSrcType { SRC_TO_X = 0, SRC_TO_Y, SRC_TO_G_ANGLE };
enum PhysicsTargetType { TARGET_FROM_ANGLE = 0, TARGET_FROM_ANGLE_V };

struct PhysicsPoint {
    float mass = 1, x = 0, y = 0, vx = 0, vy = 0, ax = 0, ay = 0, fx = 0, fy = 0;
    float lastX = 0, lastY = 0, lastVX = 0, lastVY = 0;
    void setupLast() { lastX = x; lastY = y; lastVX = vx; lastVY = vy; }
};

struct PhysicsSrc {
    PhysicsSrcType type; std::string paramId; float scale = 1, weight = 1;
    void update(ALive2DModel* model, class PhysicsHair* ctx);
};
struct PhysicsTarget {
    PhysicsTargetType type; std::string paramId; float scale = 1, weight = 1;
    void update(ALive2DModel* model, class PhysicsHair* ctx);
};

class PhysicsHair {
public:
    PhysicsHair();
    void setup(float length, float stiffness, float mass);
    void addSrcParam(PhysicsSrcType t, const std::string& id, float scale, float weight);
    void addTargetParam(PhysicsTargetType t, const std::string& id, float scale, float weight);
    void update(ALive2DModel* model, long long timeMs);

    PhysicsPoint& getP1() { return p1; }
    PhysicsPoint& getP2() { return p2; }
    float getAngle() const { return angle; }
    void setAngle(float a) { angle = a; }
    float getLastAngle() const { return lastAngle; }
    float getLastAngleVelocity() const { return angleVelocity; }

private:
    float calcAngle() const;
    void updatePhysics(ALive2DModel* model, float deltaSec);

    PhysicsPoint p1, p2;
    float length = 0, angle = 0, stiffness = 0;
    float lastAngle = 0, currentAngle = 0, angleVelocity = 0;
    long long lastTime = 0, currentTime = 0;
    std::vector<PhysicsSrc> sourceParams;
    std::vector<PhysicsTarget> targetParams;
};
} // namespace live2d
