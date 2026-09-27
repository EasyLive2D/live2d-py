#pragma once

namespace Live2D {
namespace V2 {

class L2DTargetPoint {
public:
    L2DTargetPoint() = default;

    void set(float x, float y) {
        mTargetX = x;
        mTargetY = y;
    }
    float getX() const { return mX; }
    float getY() const { return mY; }

    // 时间由外部传入（时钟只由 Model 管理）: delta_time_weight = deltaSec * FRAME_RATE
    void update(float deltaSec);

private:
    // Python l2d_target_point.py 物理核心（速度/加速度限制）
    void updateByWeight(float deltaTimeWeight);

    float mX = 0, mY = 0;
    float mTargetX = 0, mTargetY = 0;
    float mFaceVX = 0, mFaceVY = 0;
    static constexpr float sEpsilon = 0.01f;
};

}   // namespace V2
}   // namespace Live2D
