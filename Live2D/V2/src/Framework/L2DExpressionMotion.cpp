#include "L2DExpressionMotion.hpp"
#include "L2DExpressionParam.hpp"
#include "../Model/ALive2DModel.hpp"
#include "../Model/ModelContext.hpp"
#include "../Core/Id.hpp"
#include <string>
#include <cmath>

namespace live2d {

L2DExpressionMotion::L2DExpressionMotion() = default;

void L2DExpressionMotion::updateParam(ALive2DModel* model, float timeSec, float weight) {
    (void)timeSec;
    if (mParams.empty()) return;
    if (weight <= 0) return;

    auto* mc = model->getModelContext();
    for (auto& p : mParams) {
        int pi = mc->getParamIndex(&Id::getID(p.mId));
        if (pi < 0) continue;

        float cur = mc->getParamFloat(pi);
        float newVal;
        if (p.mBlendType == 0) {           // TYPE_SET
            newVal = cur * (1.0f - weight) + p.mValue * weight;
        } else if (p.mBlendType == 2) {    // TYPE_MULT
            newVal = cur * (1.0f + (p.mValue - 1.0f) * weight);
        } else {                            // TYPE_ADD (default)
            newVal = cur + p.mValue * weight;
        }
        mc->setParamFloat(pi, newVal);
        // Expression effects are ephemeral — applied on top of the post-motion
        // snapshot each frame, NOT persisted to savedParamValues.
        // This matches Python v2 where expressionManager.updateParam() runs
        // AFTER saveParam(), so expression changes vanish on next loadParam().
    }
}

L2DExpressionMotion* L2DExpressionMotion::load(const std::vector<uint8_t>& data) {
    auto* exp = new L2DExpressionMotion();
    std::string json((const char*)data.data(), data.size());

    // Parse fade_in (match Python: default 1000ms if not specified or 0)
    auto fiPos = json.find("\"fade_in\"");
    float fadeInMs = 1000;
    if (fiPos != std::string::npos) {
        auto colon = json.find(':', fiPos);
        if (colon != std::string::npos) {
            float v = (float)std::atoi(json.c_str() + colon + 1);
            if (v > 0) fadeInMs = v;
        }
    }
    exp->mFadeInSec = fadeInMs / 1000.0f;

    // Parse fade_out
    auto foPos = json.find("\"fade_out\"");
    float fadeOutMs = 1000;
    if (foPos != std::string::npos) {
        auto colon = json.find(':', foPos);
        if (colon != std::string::npos) {
            float v = (float)std::atoi(json.c_str() + colon + 1);
            if (v > 0) fadeOutMs = v;
        }
    }
    exp->mFadeOutSec = fadeOutMs / 1000.0f;

    // Parse params array
    auto paramsPos = json.find("\"params\"");
    if (paramsPos != std::string::npos) {
        auto arrStart = json.find('[', paramsPos);
        if (arrStart != std::string::npos) {
            int depth = 0;
            size_t arrEnd = arrStart;
            for (size_t k = arrStart; k < json.size(); k++) {
                if (json[k] == '[') depth++;
                else if (json[k] == ']') { depth--; if (depth == 0) { arrEnd = k; break; } }
            }
            std::string arr = json.substr(arrStart + 1, arrEnd - arrStart - 1);
            size_t pos = 0;
            while (pos < arr.size()) {
                auto idPos = arr.find("\"id\"", pos);
                if (idPos == std::string::npos) break;
                auto q1 = arr.find('"', idPos + 5);
                auto q2 = arr.find('"', q1 + 1);
                if (q1 == std::string::npos || q2 == std::string::npos) break;

                // Find the closing } of this parameter object to bound searches
                size_t objEnd = arr.find('}', q2);
                if (objEnd == std::string::npos) break;

                L2DExpressionParam p;
                p.mId = arr.substr(q1 + 1, q2 - q1 - 1);

                auto valPos = arr.find("\"val\"", q2);
                if (valPos != std::string::npos && valPos < objEnd) {
                    auto colon = arr.find(':', valPos);
                    if (colon != std::string::npos && colon < objEnd)
                        p.mValue = (float)std::atof(arr.c_str() + colon + 1);
                }

                // Parse "def" (default value for ADD/MULT blending, Python default: 0 for ADD, 1 for MULT)
                auto defPos = arr.find("\"def\"", q2);
                if (defPos != std::string::npos && defPos < objEnd) {
                    auto colon = arr.find(':', defPos);
                    if (colon != std::string::npos && colon < objEnd)
                        p.mDefValue = (float)std::atof(arr.c_str() + colon + 1);
                }

                // Parse "calc" (Python default: "add" → TYPE_ADD=1)
                auto calcPos = arr.find("\"calc\"", q2);
                if (calcPos != std::string::npos && calcPos < objEnd) {
                    auto cq1 = arr.find('"', calcPos + 7);
                    auto cq2 = arr.find('"', cq1 + 1);
                    if (cq1 != std::string::npos && cq2 != std::string::npos && cq1 < objEnd) {
                        std::string calc = arr.substr(cq1 + 1, cq2 - cq1 - 1);
                        if (calc == "set") p.mBlendType = 0;
                        else if (calc == "mult") p.mBlendType = 2;
                        else p.mBlendType = 1; // "add" (default)
                    }
                }

                // Apply def adjustment (match Python: for ADD, value = value - default)
                if (p.mBlendType == 1) // ADD
                    p.mValue = p.mValue - p.mDefValue;
                else if (p.mBlendType == 2) // MULT
                    p.mValue = p.mValue / (p.mDefValue != 0 ? p.mDefValue : 1.0f);

                exp->mParams.push_back(p);
                pos = objEnd + 1;
            }
        }
    }
    return exp;
}

} // namespace live2d
