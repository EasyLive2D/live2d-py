#pragma once
#include "AMotion.hpp"
#include "ModelContext.hpp"
#include <vector>
namespace Live2D {
namespace V2 {
class ALive2DModel;
struct MotionQueueEntry {
    AMotion* mMotion = nullptr;
    float mFadeIn = 0, mFadeOut = 0;
    bool mStarted = false;
    float mElapsedMs = 0;             // 累计播放时长（dt 积分）
    float mFadeInElapsedMs = 0;       // 累计淡入时长
    float mFadeOutEndElapsedMs = -1;  // 淡出结束时刻的 mElapsedMs（-1 = 未调度）
    bool mFinished = false;           // true when fade-out 已完成
};
class L2DMotionManager {
public:
    L2DMotionManager();
    int startMotion(AMotion* motion, bool autoPriority);
    // 时间由外部传入（时钟只由 Model 管理）: dtMs 为本次时间步长（毫秒），内部累计 elapsed
    bool updateParam(ModelContext* context, float dtMs);
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