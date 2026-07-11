#include "L2DPose.hpp"
#include "L2DPartsParam.hpp"
#include "../Model/ALive2DModel.hpp"
#include "../Model/ModelContext.hpp"
#include "../Core/Id.hpp"
#include "../Util/UtSystem.hpp"
#include <algorithm>
#include <string>
#include <cstring>

namespace live2d {

L2DPose::L2DPose() = default;

void PartData::initIndex(ALive2DModel* model) {
    auto* mc = model->getModelContext();
    std::string paramId = "VISIBLE:" + id;
    paramIndex = mc->getParamIndex(&Id::getID(paramId));
    partsIndex = mc->getPartsDataIndex(&Id::getID(id));
    for (auto& l : link) l.initIndex(model);
}

static void normalizeGroup(ALive2DModel* model, PosePartGroup& group, float deltaSec) {
    int visibleIdx = -1;
    float visibleOpacity = 1.0f;
    float clearSec = 0.5f;
    float phi = 0.5f;
    float maxBack = 0.15f;

    for (int i = 0; i < (int)group.parts.size(); i++) {
        int pi = group.parts[i].paramIndex;
        if (pi < 0) continue;
        if (model->getParamFloat(pi) != 0) {
            if (visibleIdx >= 0) break;
            visibleIdx = i;
            int pIdx = group.parts[i].partsIndex;
            if (pIdx >= 0) {
                visibleOpacity = model->getModelContext()->getPartsOpacity(pIdx);
                visibleOpacity += deltaSec / clearSec;
                if (visibleOpacity > 1.0f) visibleOpacity = 1.0f;
            }
        }
    }

    if (visibleIdx < 0) { visibleIdx = 0; visibleOpacity = 1.0f; }

    for (int i = 0; i < (int)group.parts.size(); i++) {
        int pIdx = group.parts[i].partsIndex;
        if (pIdx < 0) continue;
        if (i == visibleIdx) {
            model->getModelContext()->setPartsOpacity(pIdx, visibleOpacity);
        } else {
            float cur = model->getModelContext()->getPartsOpacity(pIdx);
            float a1;
            if (visibleOpacity < phi)
                a1 = visibleOpacity * (phi - 1.0f) / phi + 1.0f;
            else
                a1 = (1.0f - visibleOpacity) * phi / (1.0f - phi);

            float backOp = (1.0f - a1) * (1.0f - visibleOpacity);
            if (backOp > maxBack) a1 = 1.0f - maxBack / (1.0f - visibleOpacity);
            cur = std::min(cur, a1);
            model->getModelContext()->setPartsOpacity(pIdx, cur);
        }
    }
}

static void copyOpacityOtherParts(ALive2DModel* model, PosePartGroup& group) {
    for (auto& p : group.parts) {
        if (p.partsIndex < 0) continue;
        float op = model->getModelContext()->getPartsOpacity(p.partsIndex);
        for (auto& lp : p.link) {
            if (lp.partsIndex < 0) continue;
            model->getModelContext()->setPartsOpacity(lp.partsIndex, op);
        }
    }
}

void L2DPose::updateParam(ALive2DModel* model) {
    if (model != mLastModel) {
        for (auto& g : mMGroups) {
            for (auto& p : g.parts) {
                p.initIndex(model);
                if (p.partsIndex < 0) continue;
                bool v = (model->getParamFloat(p.paramIndex) != 0);
                model->getModelContext()->setPartsOpacity(p.partsIndex, v ? 1.0f : 0.0f);
                model->setParamFloat(p.paramIndex, v ? 1.0f : 0.0f);
            }
        }
    }

    mLastModel = model;
    float now = (float)UtSystem::getUserTimeMSec();
    float dt = (mLastTime > 0) ? (now - mLastTime) / 1000.0f : 0;
    if (dt < 0) dt = 0;
    mLastTime = now;

    for (auto& g : mMGroups) {
        normalizeGroup(model, g, dt);
        copyOpacityOtherParts(model, g);
    }
}

L2DPose* L2DPose::load(const std::vector<uint8_t>& data) {
    auto* pose = new L2DPose();
    std::string json((const char*)data.data(), data.size());

    auto pvPos = json.find("\"parts_visible\"");
    if (pvPos == std::string::npos) return pose;

    auto arrStart = json.find('[', pvPos);
    if (arrStart == std::string::npos) return pose;
    int depth = 0;
    size_t arrEnd = arrStart;
    for (size_t i = arrStart; i < json.size(); i++) {
        if (json[i] == '[') depth++;
        else if (json[i] == ']') { depth--; if (depth == 0) { arrEnd = i; break; } }
    }
    if (arrEnd == arrStart) return pose;

    std::string arr = json.substr(arrStart + 1, arrEnd - arrStart - 1);
    size_t pos = 0;
    while (pos < arr.size()) {
        auto grpStart = arr.find("\"group\"", pos);
        if (grpStart == std::string::npos) break;
        auto gaStart = arr.find('[', grpStart);
        if (gaStart == std::string::npos) break;
        int gDepth = 0;
        size_t gaEnd = gaStart;
        for (size_t k = gaStart; k < arr.size(); k++) {
            if (arr[k] == '[') gDepth++;
            else if (arr[k] == ']') { gDepth--; if (gDepth == 0) { gaEnd = k; break; } }
        }
        if (gaEnd == gaStart) break;

        std::string groupStr = arr.substr(gaStart + 1, gaEnd - gaStart - 1);
        PosePartGroup group;

        size_t gPos = 0;
        while (gPos < groupStr.size()) {
            // Parse {"id":"PARTS_XX", "link":["...","..."]}
            auto objStart = groupStr.find('{', gPos);
            if (objStart == std::string::npos) break;
            // Find matching }
            int oDepth = 0;
            size_t objEnd = objStart;
            for (size_t k = objStart; k < groupStr.size(); k++) {
                if (groupStr[k] == '{') oDepth++;
                else if (groupStr[k] == '}') { oDepth--; if (oDepth == 0) { objEnd = k; break; } }
            }
            if (objEnd == objStart) break;

            std::string objStr = groupStr.substr(objStart, objEnd - objStart + 1);

            // Parse "id"
            auto idStart = objStr.find("\"id\"");
            if (idStart != std::string::npos) {
                auto q1 = objStr.find('"', idStart + 5);
                auto q2 = objStr.find('"', q1 + 1);
                if (q1 != std::string::npos && q2 != std::string::npos) {
                    PartData pd;
                    pd.id = objStr.substr(q1 + 1, q2 - q1 - 1);

                    // Parse "link" array if present
                    auto linkStart = objStr.find("\"link\"");
                    if (linkStart != std::string::npos) {
                        auto laStart = objStr.find('[', linkStart);
                        if (laStart != std::string::npos) {
                            int lDepth = 0;
                            size_t laEnd = laStart;
                            for (size_t l = laStart; l < objStr.size(); l++) {
                                if (objStr[l] == '[') lDepth++;
                                else if (objStr[l] == ']') { lDepth--; if (lDepth == 0) { laEnd = l; break; } }
                            }
                            if (laEnd > laStart) {
                                std::string linkStr = objStr.substr(laStart + 1, laEnd - laStart - 1);
                                size_t lp = 0;
                                while (lp < linkStr.size()) {
                                    auto lq1 = linkStr.find('"', lp);
                                    if (lq1 == std::string::npos) break;
                                    auto lq2 = linkStr.find('"', lq1 + 1);
                                    if (lq2 == std::string::npos) break;
                                    PartData linkPd;
                                    linkPd.id = linkStr.substr(lq1 + 1, lq2 - lq1 - 1);
                                    pd.link.push_back(linkPd);
                                    lp = lq2 + 1;
                                }
                            }
                        }
                    }

                    group.parts.push_back(pd);
                }
            }
            gPos = objEnd + 1;
        }

        pose->mMGroups.push_back(group);
        pos = gaEnd + 1;
    }
    return pose;
}

} // namespace live2d
