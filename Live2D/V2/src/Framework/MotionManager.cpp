#include "MotionManager.hpp"
#include <cmath>
namespace Live2D {
namespace V2 {
MotionManager::MotionManager() = default;
bool MotionManager::ReserveMotion(int priority) {
    if (priority < mReservePriority)
        return false;
    if (priority < mCurrentPriority)
        return false;
    mReservePriority = priority;
    return true;
}
int MotionManager::StartMotion(AMotion* motion, bool autoPriority) {
    // Fade out existing motions, matching Python v2: shorter end time wins.
    // 无条件记录终点（fadeout=0 时 newEnd=elapsed，下一帧 remaining<=0 立即
    // 结束移除，对齐 V3 SetFadeout(0) 的即时终止语义）——若跳过设置，该
    // entry 永无 fadeout 终点：越线判定时被当作未抢占（误触发），loop 场景
    // 更会永久占用队列成为僵尸 entry
    for (auto& e : mMotions) {
        float newEnd = e.mElapsedMs + e.mFadeOut * 1000.0f;
        if (e.mFadeOutEndElapsedMs < 0 || newEnd < e.mFadeOutEndElapsedMs)
            e.mFadeOutEndElapsedMs = newEnd;
    }
    mMotions.push_back({motion, motion->mFadeInSec, motion->mFadeOutSec});
    if (motion->mOnStart) {
        // Copy before invoke: a reentrant StartMotion from inside the callback
        // overwrites the slot, destroying the in-flight wrapper state (its
        // captured group string is the very argument of this call chain)
        std::function<void()> onStart = motion->mOnStart;
        onStart();
    }
    return (int)mMotions.size() - 1;
}
int MotionManager::StartMotionPrio(AMotion* motion, int priority) {
    if (priority == mReservePriority)
        mReservePriority = 0;
    mCurrentPriority = priority;
    return StartMotion(motion, false);
}
// Easing: 0.5 - 0.5*cos(x*pi), clamped [0,1]
static float easeSine(float x) {
    if (x <= 0)
        return 0;
    if (x >= 1)
        return 1;
    return 0.5f - 0.5f * cosf(x * 3.14159265f);
}

bool MotionManager::UpdateParam(Moc* moc, float dtMs) {
    bool updated = false;
    for (size_t i = 0; i < mMotions.size();) {
        auto& e = mMotions[i];
        if (!e.mStarted) {
            e.mStarted = true;
        }
        e.mElapsedMs += dtMs;
        float elapsed = e.mElapsedMs / 1000.0f;

        // Fade-in weight
        float fadeIn = 1.0f;
        if (e.mFadeIn > 0) {
            e.mFadeInElapsedMs += dtMs;
            fadeIn = easeSine(e.mFadeInElapsedMs / (e.mFadeIn * 1000.0f));
        }
        // Fade-out weight
        float fadeOut = 1.0f;
        if (e.mFadeOut > 0 && e.mFadeOutEndElapsedMs >= 0) {
            float remaining = (e.mFadeOutEndElapsedMs - e.mElapsedMs) / (e.mFadeOut * 1000.0f);
            if (remaining <= 0) {
                e.mFinished = true;
                fadeOut = 0;
            } else {
                fadeOut = easeSine(remaining);
            }
        }

        float weight = e.mMotion->mWeight * fadeIn * fadeOut;
        if (weight < 0)
            weight = 0;
        if (weight > 1)
            weight = 1;

        e.mMotion->UpdateParam(moc, elapsed, weight);
        updated = true;
        // per-entry 结束判定：Motion::mFinished 是共享 flag（StartMotion 时
        // Reset、UpdateParam 越线置位），多 entry 共享同一 motion 对象时会被
        // 被抢占 entry 置位，污染同帧后续 entry 的判定（误杀+重复触发），
        // 故用 entry 自身 elapsed 对比 duration
        const bool timeOver =
            !e.mMotion->IsLoop() && e.mElapsedMs > e.mMotion->GetDurationSec() * 1000.0f;
        if (e.mFinished || timeOver) {
            // 先移出队列再回调：回调内可安全重入 StartMotion（含同一
            // motion），不依赖 e 在回调后的有效性
            // 静默范围：fadeOut 到期结束（e.mFinished），以及被抢占中
            // （已设 fadeOutEnd）时越过 duration——两者都不是自然播完；
            // 回调槽位只调用不消费（与 mOnStart/V3 一致）——槽位挂在共
            // 享 motion 对象上（最后一次挂载语义），静默时不得动它，残
            // 留随挂载覆盖或 motion 析构释放
            const bool preempted = e.mFinished || (timeOver && e.mFadeOutEndElapsedMs >= 0);
            AMotion* motion = e.mMotion;
            mMotions.erase(mMotions.begin() + i);
            if (!preempted && motion->mOnFinish) {
                // Copy before invoke（同 StartMotion 的 mOnStart）：回调内重入
                // StartMotion 会覆盖槽位、析构在飞 wrapper 的捕获状态
                std::function<void()> onFinish = motion->mOnFinish;
                onFinish();
            }
        } else {
            i++;
        }
    }
    if (mMotions.empty())
        mCurrentPriority = 0;
    return updated;
}
bool MotionManager::IsFinished() const {
    return mMotions.empty();
}
void MotionManager::StopAllMotions() {
    mMotions.clear();
}
}   // namespace V2
}   // namespace Live2D