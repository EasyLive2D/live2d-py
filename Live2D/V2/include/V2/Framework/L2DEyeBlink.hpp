#pragma once
#include <string>
namespace Live2D {
namespace V2 {
class ModelContext;
class L2DEyeBlink
{
public:
    L2DEyeBlink();
    void setInterval(int ms) { mBlinkIntervalMs = ms; }
    void setEyeMotion(int closeMs, int closedMs, int openMs);
    void updateParam(ModelContext* context);
    float mBlinkIntervalMs = 5000;

private:
    enum State
    {
        FIRST,
        INTERVAL,
        CLOSING,
        CLOSED,
        OPENING
    };
    State mState = FIRST;
    float mClosingMs = 150, mClosedMs = 80, mOpeningMs = 220;
    float mCurrentTime = 0, mNextBlinkTime = 0, mStateStartTime = 0;
    bool mCloseIfZero = true;
    std::string mEyeLId = "PARAM_EYE_L_OPEN", mEyeRId = "PARAM_EYE_R_OPEN";
};
}   // namespace V2
}   // namespace Live2D