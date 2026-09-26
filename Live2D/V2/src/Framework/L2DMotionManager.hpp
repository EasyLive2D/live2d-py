#pragma once
#include "../Motion/AMotion.hpp"
#include "Model/ModelContext.hpp"
#include <vector>
namespace Live2D {
namespace V2 {
class ALive2DModel;
struct MotionQueueEntry
{
    AMotion* mMotion = nullptr;
    float mFadeIn = 0, mFadeOut = 0;
    bool mStarted = false;
    float mStartTimeMs = 0;
    float mFadeInStartMs = 0;   // for easing calculation
    float mEndTimeMs = -1;      // for fade-out
    bool mFinished = false;     // true when endTimeMs has passed
};
class L2DMotionManager
{
public:
    L2DMotionManager();
    int startMotion(AMotion* motion, bool autoPriority);
    bool updateParam(ModelContext* context);
    bool isFinished() const;
    void stopAllMotions();
    int mCurrentPriority = 0, mReservePriority = 0;
    std::vector<MotionQueueEntry> mMotions;
    bool reserveMotion(int priority);
    void setReservePriority(int val) { mReservePriority = val; }
    int startMotionPrio(AMotion* motion, int priority);
};
}   // namespace V2
}   // namespace Live2D