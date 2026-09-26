#include "L2DExpressionMotion.hpp"
#include "../Core/Id.hpp"
#include "../Model/ModelContext.hpp"
#include "L2DExpressionParam.hpp"
#include "nlohmann/json.hpp"
#include <string>

namespace Live2D {
namespace V2 {

using json = nlohmann::json;

L2DExpressionMotion::L2DExpressionMotion() = default;

void L2DExpressionMotion::updateParam(ModelContext* context, float timeSec, float weight)
{
    (void)timeSec;
    if (mParams.empty())
        return;
    if (weight <= 0)
        return;

    for (auto& p : mParams) {
        int pi = context->getParamIndex(&Id::getID(p.mId));
        if (pi < 0)
            continue;

        float cur = context->getParamFloat(pi);
        float newVal;
        if (p.mBlendType == 0) {   // TYPE_SET
            newVal = cur * (1.0f - weight) + p.mValue * weight;
        } else if (p.mBlendType == 2) {   // TYPE_MULT
            newVal = cur * (1.0f + (p.mValue - 1.0f) * weight);
        } else {   // TYPE_ADD (default)
            newVal = cur + p.mValue * weight;
        }
        context->setParamFloat(pi, newVal);
        // Expression effects are ephemeral — applied on top of the post-motion
        // snapshot each frame, NOT persisted to savedParamValues.
        // This matches Python v2 where expressionManager.updateParam() runs
        // AFTER saveParam(), so expression changes vanish on next loadParam().
    }
}

L2DExpressionMotion* L2DExpressionMotion::load(const std::vector<uint8_t>& data)
{
    auto* exp = new L2DExpressionMotion();
    if (data.empty())
        return exp;
    // Python: pm.jsonParseFromBytes == json.loads (l2d_expression_motion.py:31-66)
    json root = json::parse(std::string((const char*)data.data(), data.size()));

    // Python: int(get("fade_in", 0)) if > 0 else 1000, then /1000
    int fadeInMs = (int)root.value("fade_in", 0.0f);
    int fadeOutMs = (int)root.value("fade_out", 0.0f);
    exp->mFadeInSec = (fadeInMs > 0 ? fadeInMs : 1000) / 1000.0f;
    exp->mFadeOutSec = (fadeOutMs > 0 ? fadeOutMs : 1000) / 1000.0f;

    auto params = root.find("params");
    if (params == root.end() || !params->is_array())
        return exp;

    for (const auto& p : *params) {
        if (!p.is_object())
            continue;
        L2DExpressionParam out;
        out.mId = p.value("id", std::string());
        out.mValue = p.value("val", 0.0f);   // required in Python
        std::string calc = p.value("calc", std::string("add"));
        out.mBlendType = (calc == "set") ? 0 : (calc == "mult") ? 2 : 1;   // unknown => TYPE_ADD

        if (out.mBlendType == 1) {   // TYPE_ADD: value -= def (default 0)
            out.mDefValue = p.value("def", 0.0f);
            out.mValue -= out.mDefValue;
        } else if (out.mBlendType == 2) {   // TYPE_MULT: value /= def (default 1, 0 -> 1)
            out.mDefValue = p.value("def", 1.0f);
            out.mValue /= (out.mDefValue != 0.0f ? out.mDefValue : 1.0f);
        }
        // TYPE_SET: Python never reads "def" — mDefValue stays 0

        exp->mParams.push_back(std::move(out));
    }
    return exp;
}

}   // namespace V2
}   // namespace Live2D