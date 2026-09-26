#include "L2DPose.hpp"
#include "../Core/Id.hpp"
#include "../Model/ModelContext.hpp"
#include "../Util/UtSystem.hpp"
#include "L2DPartsParam.hpp"
#include "nlohmann/json.hpp"
#include <algorithm>
#include <cstring>
#include <string>

namespace Live2D {
namespace V2 {

using json = nlohmann::json;

L2DPose::L2DPose() = default;

void PartData::initIndex(ModelContext* context)
{
    std::string paramId = "VISIBLE:" + id;
    paramIndex = context->getParamIndex(&Id::getID(paramId));
    partsIndex = context->getPartsDataIndex(&Id::getID(id));
    for (auto& l : link)
        l.initIndex(context);
}

static void normalizeGroup(ModelContext* context, PosePartGroup& group, float deltaSec)
{
    int visibleIdx = -1;
    float visibleOpacity = 1.0f;
    float clearSec = 0.5f;
    float phi = 0.5f;
    float maxBack = 0.15f;

    for (int i = 0; i < (int)group.parts.size(); i++) {
        int pi = group.parts[i].paramIndex;
        if (pi < 0)
            continue;
        if (context->getParamFloat(pi) != 0) {
            if (visibleIdx >= 0)
                break;
            visibleIdx = i;
            int pIdx = group.parts[i].partsIndex;
            if (pIdx >= 0) {
                visibleOpacity = context->getPartsOpacity(pIdx);
                visibleOpacity += deltaSec / clearSec;
                if (visibleOpacity > 1.0f)
                    visibleOpacity = 1.0f;
            }
        }
    }

    if (visibleIdx < 0) {
        visibleIdx = 0;
        visibleOpacity = 1.0f;
    }

    for (int i = 0; i < (int)group.parts.size(); i++) {
        int pIdx = group.parts[i].partsIndex;
        if (pIdx < 0)
            continue;
        if (i == visibleIdx) {
            context->setPartsOpacity(pIdx, visibleOpacity);
        } else {
            float cur = context->getPartsOpacity(pIdx);
            float a1;
            if (visibleOpacity < phi)
                a1 = visibleOpacity * (phi - 1.0f) / phi + 1.0f;
            else
                a1 = (1.0f - visibleOpacity) * phi / (1.0f - phi);

            float backOp = (1.0f - a1) * (1.0f - visibleOpacity);
            if (backOp > maxBack)
                a1 = 1.0f - maxBack / (1.0f - visibleOpacity);
            cur = std::min(cur, a1);
            context->setPartsOpacity(pIdx, cur);
        }
    }
}

static void copyOpacityOtherParts(ModelContext* context, PosePartGroup& group)
{
    for (auto& p : group.parts) {
        if (p.partsIndex < 0)
            continue;
        float op = context->getPartsOpacity(p.partsIndex);
        for (auto& lp : p.link) {
            if (lp.partsIndex < 0)
                continue;
            context->setPartsOpacity(lp.partsIndex, op);
        }
    }
}

void L2DPose::initParam(ModelContext* context)
{
    // Python: initParam runs once per model instance (l2d_pose.py:27-42),
    // not per frame — the per-frame opacity hard-set was wiping the fade.
    for (auto& g : mMGroups) {
        for (auto& p : g.parts) {
            p.initIndex(context);
            if (p.partsIndex < 0)
                continue;
            bool v = (context->getParamFloat(p.paramIndex) != 0);
            context->setPartsOpacity(p.partsIndex, v ? 1.0f : 0.0f);
            context->setParamFloat(p.paramIndex, v ? 1.0f : 0.0f);
        }
    }
}

void L2DPose::updateParam(ModelContext* context)
{
    float now = (float)UtSystem::getUserTimeMSec();
    float dt = (mLastTime > 0) ? (now - mLastTime) / 1000.0f : 0;
    if (dt < 0)
        dt = 0;
    mLastTime = now;

    for (auto& g : mMGroups) {
        normalizeGroup(context, g, dt);
        copyOpacityOtherParts(context, g);
    }
}

L2DPose* L2DPose::load(const std::vector<uint8_t>& data)
{
    auto* pose = new L2DPose();
    if (data.empty())
        return pose;
    // Python: pm.jsonParseFromBytes == json.loads (l2d_pose.py:106-132)
    json root = json::parse(std::string((const char*)data.data(), data.size()));
    auto visible = root.find("parts_visible");
    if (visible == root.end() || !visible->is_array())
        return pose;

    for (const auto& poseInfo : *visible) {
        auto groups = poseInfo.find("group");
        if (groups == poseInfo.end() || !groups->is_array())
            continue;
        PosePartGroup group;
        for (const auto& partsInfo : *groups) {
            if (!partsInfo.is_object())
                continue;
            PartData parts;
            parts.id = partsInfo.value("id", std::string());
            // Python: get("link") — missing or null => no links
            auto link = partsInfo.find("link");
            if (link != partsInfo.end() && link->is_array()) {
                for (const auto& linkId : *link) {
                    if (!linkId.is_string())
                        continue;
                    PartData linkParts;
                    linkParts.id = linkId.get<std::string>();
                    parts.link.push_back(std::move(linkParts));
                }
            }
            group.parts.push_back(std::move(parts));
        }
        pose->mMGroups.push_back(std::move(group));
    }
    return pose;
}

}   // namespace V2
}   // namespace Live2D