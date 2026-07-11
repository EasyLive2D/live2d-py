#include "PhysicsHair.hpp"
#include "../Model/ALive2DModel.hpp"
#include "../Model/ModelContext.hpp"
#include "../Core/Id.hpp"
#include "../Util/UtMath.hpp"
#include <cmath>

namespace live2d {

// ---- PhysicsSrc ----
void PhysicsSrc::update(ALive2DModel* model, PhysicsHair* ctx) {
    auto* mc = model->getModelContext();
    int idx = mc->getParamIndex(&Id::getID(paramId));
    if (idx < 0) return;
    float scaledValue = scale * mc->getParamFloat(idx);
    auto& p1 = ctx->getP1();

    if (type == SRC_TO_X)
        p1.x += (scaledValue - p1.x) * weight;
    else if (type == SRC_TO_Y)
        p1.y += (scaledValue - p1.y) * weight;
    else if (type == SRC_TO_G_ANGLE) {
        float a = ctx->getAngle();
        a += (scaledValue - a) * weight;
        ctx->setAngle(a);
    }
}

// ---- PhysicsTarget ----
void PhysicsTarget::update(ALive2DModel* model, PhysicsHair* ctx) {
    auto* mc = model->getModelContext();
    int idx = mc->getParamIndex(&Id::getID(paramId));
    if (idx < 0) return;
    if (type == TARGET_FROM_ANGLE)
        mc->setParamFloat(idx, scale * ctx->getLastAngle());
    else if (type == TARGET_FROM_ANGLE_V)
        mc->setParamFloat(idx, scale * ctx->getLastAngleVelocity());
}

// ---- PhysicsHair ----
PhysicsHair::PhysicsHair() { setup(0.3f, 0.5f, 0.1f); }

void PhysicsHair::setup(float l, float stiff, float mass) {
    currentAngle = calcAngle();
    p2.setupLast();
    length = l;
    stiffness = stiff;
    p1.mass = mass;
    p2.mass = mass;
    p2.y = l;
}

float PhysicsHair::calcAngle() const {
    return -180.0f * std::atan2(p1.x - p2.x, -(p1.y - p2.y)) / 3.14159265f;
}

void PhysicsHair::addSrcParam(PhysicsSrcType t, const std::string& id, float scale, float weight) {
    sourceParams.push_back({t, id, scale, weight});
}
void PhysicsHair::addTargetParam(PhysicsTargetType t, const std::string& id, float scale, float weight) {
    targetParams.push_back({t, id, scale, weight});
}

void PhysicsHair::update(ALive2DModel* model, long long timeMs) {
    if (lastTime == 0) {
        lastTime = currentTime = timeMs;
        float dx = p1.x - p2.x, dy = p1.y - p2.y;
        length = std::sqrt(dx * dx + dy * dy);
        return;
    }
    float deltaSec = (float)(timeMs - currentTime) / 1000.0f;
    if (deltaSec != 0) {
        for (auto& s : sourceParams) s.update(model, this);
        updatePhysics(model, deltaSec);
        lastAngle = calcAngle();
        angleVelocity = (lastAngle - currentAngle) / deltaSec;
        currentAngle = lastAngle;
    }
    for (auto& t : targetParams) t.update(model, this);
    currentTime = timeMs;
}

void PhysicsHair::updatePhysics(ALive2DModel*, float deltaSec) {
    if (deltaSec < 0.033f) deltaSec = 0.033f;
    float inv = 1.0f / deltaSec;

    p1.vx = (p1.x - p1.lastX) * inv;
    p1.vy = (p1.y - p1.lastY) * inv;
    p1.ax = (p1.vx - p1.lastVX) * inv;
    p1.ay = (p1.vy - p1.lastVY) * inv;
    p1.fx = p1.ax * p1.mass;
    p1.fy = p1.ay * p1.mass;
    p1.setupLast();

    float a = -std::atan2(p1.y - p2.y, p1.x - p2.x);
    float cs = std::cos(a), sn = std::sin(a);
    float gravity = 9.8f * p2.mass;
    float aRad = angle * 3.14159265f / 180.0f;
    float gForce = gravity * std::cos(a - aRad);
    float fX = gForce * sn;
    float fY = gForce * cs;
    float dragX = -p1.fx * sn * sn;
    float dragY = -p1.fy * sn * cs;
    float stiffX = -p2.vx * stiffness;
    float stiffY = -p2.vy * stiffness;

    p2.fx = fX + dragX + stiffX;
    p2.fy = fY + dragY + stiffY;
    p2.ax = p2.fx / p2.mass;
    p2.ay = p2.fy / p2.mass;
    p2.vx += p2.ax * deltaSec;
    p2.vy += p2.ay * deltaSec;
    p2.x += p2.vx * deltaSec;
    p2.y += p2.vy * deltaSec;

    float dx = p1.x - p2.x, dy = p1.y - p2.y;
    float dist = std::sqrt(dx * dx + dy * dy);
    p2.x = p1.x + length * (p2.x - p1.x) / dist;
    p2.y = p1.y + length * (p2.y - p1.y) / dist;
    p2.vx = (p2.x - p2.lastX) * inv;
    p2.vy = (p2.y - p2.lastY) * inv;
    p2.setupLast();
}

} // namespace live2d
