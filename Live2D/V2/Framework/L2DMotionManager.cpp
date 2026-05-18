#include "L2DMotionManager.hpp"
#include "../Model/ALive2DModel.hpp"
#include "../Util/UtSystem.hpp"
#include <cmath>
namespace live2d {
L2DMotionManager::L2DMotionManager() = default;
bool L2DMotionManager::reserveMotion(int priority) {
    if (priority < mReservePriority) return false;
    if (priority < mCurrentPriority) return false;
    mReservePriority = priority;
    return true;
}
int L2DMotionManager::startMotion(AMotion* motion, bool autoPriority) {
    float now = (float)UtSystem::getUserTimeMSec();
    // Fade out existing motions, matching Python v2: shorter end time wins
    for (auto& e : mMotions) {
        if (e.mFadeOut > 0) {
            float newEnd = now + e.mFadeOut * 1000.0f;
            if (e.mEndTimeMs < 0 || newEnd < e.mEndTimeMs)
                e.mEndTimeMs = newEnd;
        }
    }
    mMotions.push_back({motion, motion->mFadeInSec, motion->mFadeOutSec, false,
                        now, now, -1.0f});
    return (int)mMotions.size() - 1;
}
int L2DMotionManager::startMotionPrio(AMotion* motion, int priority) {
    if (priority == mReservePriority) mReservePriority = 0;
    mCurrentPriority = priority;
    return startMotion(motion, false);
}
// Easing: 0.5 - 0.5*cos(x*pi), clamped [0,1]
static float easeSine(float x) {
    if (x <= 0) return 0;
    if (x >= 1) return 1;
    return 0.5f - 0.5f * cosf(x * 3.14159265f);
}

bool L2DMotionManager::updateParam(ALive2DModel* model) {
    float now = (float)UtSystem::getUserTimeMSec();
    bool updated = false;
    for (size_t i = 0; i < mMotions.size(); ) {
        auto& e = mMotions[i];
        if (!e.mStarted) {
            e.mStartTimeMs = now;
            e.mFadeInStartMs = now;
            // Don't reset mEndTimeMs if startFadeOut already set it
            if (e.mEndTimeMs < 0)
                e.mEndTimeMs = -1;
            e.mStarted = true;
        }
        float elapsed = (now - e.mStartTimeMs) / 1000.0f;

        // Fade-in weight
        float fadeIn = 1.0f;
        if (e.mFadeIn > 0 && e.mFadeInStartMs >= 0) {
            fadeIn = easeSine((now - e.mFadeInStartMs) / (e.mFadeIn * 1000.0f));
        }
        // Fade-out weight
        float fadeOut = 1.0f;
        if (e.mFadeOut > 0 && e.mEndTimeMs >= 0) {
            float remaining = (e.mEndTimeMs - now) / (e.mFadeOut * 1000.0f);
            if (remaining <= 0) {
                e.mFinished = true;
                fadeOut = 0;
            } else {
                fadeOut = easeSine(remaining);
            }
        }

        float weight = e.mMotion->mWeight * fadeIn * fadeOut;
        if (weight < 0) weight = 0;
        if (weight > 1) weight = 1;

        e.mMotion->updateParam(model, elapsed, weight);
        updated = true;
        if (e.mFinished || e.mMotion->isFinished()) {
            mMotions.erase(mMotions.begin() + i);
        } else { i++; }
    }
    if (mMotions.empty()) mCurrentPriority = 0;
    return updated;
}
bool L2DMotionManager::isFinished() const { return mMotions.empty(); }
void L2DMotionManager::stopAllMotions() { mMotions.clear(); }
} // namespace live2d
