#pragma once

namespace Live2D {
namespace V2 {

class L2DTargetPoint
{
public:
    L2DTargetPoint() = default;

    void set(float x, float y)
    {
        mTargetX = x;
        mTargetY = y;
    }
    float getX() const { return mX; }
    float getY() const { return mY; }
    void update(float deltaSec);

private:
    float mX = 0, mY = 0;
    float mTargetX = 0, mTargetY = 0;
    static constexpr float sEpsilon = 0.01f;
};

}   // namespace V2
}   // namespace Live2D