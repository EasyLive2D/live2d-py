#pragma once
#include "AMotion.hpp"
#include "Moc.hpp"
#include <vector>
namespace Live2D {
namespace V2 {
class ALive2DModel;
struct MotionQueueEntry {
    AMotion* mMotion = nullptr;
    float mFadeIn = 0, mFadeOut = 0;
    bool mStarted = false;
    float mElapsedMs = 0;              // 累计播放时长（dt 积分）
    float mFadeInElapsedMs = 0;        // 累计淡入时长
    float mFadeOutEndElapsedMs = -1;   // 淡出结束时刻的 mElapsedMs（-1 = 未调度）
    bool mFinished = false;            // true when fade-out 已完成
};
class MotionManager {
public:
    MotionManager();
    int StartMotion(AMotion* motion, bool autoPriority);
    // 时间由外部传入（时钟只由 Model 管理）: dtMs 为本次时间步长（毫秒），内部累计 elapsed
    bool UpdateParam(Moc* moc, float dtMs);
    bool IsFinished() const;
    void StopAllMotions();
    int mCurrentPriority = 0, mReservePriority = 0;
    std::vector<MotionQueueEntry> mMotions;
    bool ReserveMotion(int priority);
    void SetReservePriority(int val) { mReservePriority = val; }
    int StartMotionPrio(AMotion* motion, int priority);
};
}   // namespace V2
}   // namespace Live2D