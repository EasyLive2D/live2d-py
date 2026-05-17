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
    for (auto& e : mMotions) e.mStarted = false;
    float now = (float)UtSystem::getUserTimeMSec();
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

void L2DMotionManager::updateParam(ALive2DModel* model) {
    float now = (float)UtSystem::getUserTimeMSec();
    for (size_t i = 0; i < mMotions.size(); ) {
        auto& e = mMotions[i];
        if (!e.mStarted) {
            e.mStartTimeMs = now;
            e.mFadeInStartMs = now;
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
            fadeOut = easeSine((e.mEndTimeMs - now) / (e.mFadeOut * 1000.0f));
        }

        float weight = e.mMotion->mWeight * fadeIn * fadeOut;
        if (weight < 0) weight = 0;
        if (weight > 1) weight = 1;

        e.mMotion->updateParam(model, elapsed, weight);
        if (e.mMotion->isFinished()) {
            mMotions.erase(mMotions.begin() + i);
        } else { i++; }
    }
    if (mMotions.empty()) mCurrentPriority = 0;
}
bool L2DMotionManager::isFinished() const { return mMotions.empty(); }
void L2DMotionManager::stopAllMotions() { mMotions.clear(); }
} // namespace live2d
