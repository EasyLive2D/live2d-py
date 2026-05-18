#include "Live2DMotion.hpp"
#include "../Model/ALive2DModel.hpp"
#include "../Model/ModelContext.hpp"
#include "../Core/Id.hpp"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <cmath>

namespace live2d {

Live2DMotion::Live2DMotion() = default;

void Live2DMotion::updateParam(ALive2DModel* model, float timeSec, float weight) {
    if (mDurationMs <= 0 || mMotions.empty()) return;
    float timeMs = timeSec * 1000.0f;

    if (mLoop) {
        float loopDur = mLoopDurationMs > 0 ? mLoopDurationMs : mDurationMs;
        timeMs = std::fmod(timeMs, loopDur);
    } else if (timeMs > mDurationMs) {
        mFinished = true;
        timeMs = mDurationMs;
    }

    float frame = timeMs * mFps / 1000.0f;
    int idx0 = (int)frame;
    int idx1 = idx0 + 1;
    float t = frame - (float)idx0;

    for (auto& m : mMotions) {
        int sz = (int)m.mValues.size();
        if (sz == 0) continue;
        float v0, v1, val;
        if (idx0 >= sz) {
            // Past end of curve: use last value (should be 0 for hand params)
            v0 = m.mValues[sz - 1];
            v1 = v0;
            val = v0;
        } else {
            v0 = m.mValues[idx0];
            v1 = (idx1 < sz) ? m.mValues[idx1] : v0;
            val = v0 + (v1 - v0) * t;
        }

        if (m.mParamId == "VISIBLE") {
            // Match Python: set the "VISIBLE:xxx" parameter so the pose system
            // (L2DPose::updateParam) can read it and apply smooth fade transitions.
            std::string paramId = "VISIBLE:" + m.mSecondaryId;
            int pi = model->getModelContext()->getParamIndex(&Id::getID(paramId));
            if (pi >= 0)
                model->getModelContext()->setParamFloat(pi, val);
        } else {
            int pi = model->getModelContext()->getParamIndex(&Id::getID(m.mParamId));
            if (pi >= 0) {
                model->setParamFloat(pi, val, weight);
            }
        }
    }
}

float Live2DMotion::getDurationSec() const { return mDurationMs / 1000.0f; }
bool Live2DMotion::isLoop() const { return mLoop; }

Live2DMotion* Live2DMotion::load(const std::vector<uint8_t>& data) {
    auto* m = new Live2DMotion();
    std::string content((const char*)data.data(), data.size());
    std::istringstream ss(content);
    std::string line;

while (std::getline(ss, line)) {
        if (line.empty() || line[0] == '#') continue;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
            line.pop_back();

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        // Strip leading '$' from key (MTN format: $fps=30)
        if (!key.empty() && key[0] == '$') key = key.substr(1);

        if (key == "fps") {
            m->mFps = (float)std::strtof(val.c_str(), nullptr);
        }
        else if (key == "FADEIN") { m->mFadeInSec = (float)std::atoi(val.c_str()) / 1000.0f; }
        else if (key == "FADEOUT") { m->mFadeOutSec = (float)std::atoi(val.c_str()) / 1000.0f; }
        else if (key == "LOOP") { m->mLoop = (val == "1" || val == "true"); }
        else if (key == "LOOPFADEIN") { m->mLoopFadeIn = (val == "1" || val == "true"); }
        else {
            Motion motion;
            if (key.find("VISIBLE:") == 0) {
                motion.mParamId = "VISIBLE";
                motion.mSecondaryId = key.substr(8);
            } else if (key.find("LAYOUT:") == 0) {
                motion.mParamId = "LAYOUT";
                motion.mSecondaryId = key.substr(7);
            } else {
                motion.mParamId = key;
            }
            std::istringstream vs(val);
            std::string token;
            while (std::getline(vs, token, ','))
                if (!token.empty()) motion.mValues.push_back((float)std::atof(token.c_str()));
            if (!motion.mValues.empty()) m->mMotions.push_back(motion);
        }
    }

    int maxLen = 0;
    for (auto& mot : m->mMotions)
        maxLen = std::max(maxLen, (int)mot.mValues.size());
    m->mDurationMs = (maxLen > 1) ? (float)(maxLen - 1) * 1000.0f / m->mFps : 0;
    m->mLoopDurationMs = m->mDurationMs;
    return m;
}

} // namespace live2d
