#include "L2DPhysics.hpp"
#include "PhysicsHair.hpp"
#include "../Model/ALive2DModel.hpp"
#include "../Util/UtSystem.hpp"
#include <string>
#include <cmath>

namespace live2d {

L2DPhysics::L2DPhysics() {
    mStartTimeMs = (long long)UtSystem::getUserTimeMSec();
}

void L2DPhysics::updateParam(ALive2DModel* model) {
    long long t = (long long)UtSystem::getUserTimeMSec() - mStartTimeMs;
    for (auto* ph : mPhysicsList) ph->update(model, t);
}

L2DPhysics* L2DPhysics::load(const std::vector<uint8_t>& data) {
    auto* ret = new L2DPhysics();
    std::string json((const char*)data.data(), data.size());

    auto hairPos = json.find("\"physics_hair\"");
    if (hairPos == std::string::npos) return ret;
    auto arrStart = json.find('[', hairPos);
    if (arrStart == std::string::npos) return ret;

    // Find matching ']'
    int depth = 0;
    size_t arrEnd = arrStart;
    for (size_t i = arrStart; i < json.size(); i++) {
        if (json[i] == '[') depth++;
        else if (json[i] == ']') { depth--; if (depth == 0) { arrEnd = i; break; } }
    }
    std::string arr = json.substr(arrStart + 1, arrEnd - arrStart - 1);

    size_t pos = 0;
    while (pos < arr.size()) {
        auto objStart = arr.find('{', pos);
        if (objStart == std::string::npos) break;
        int oDepth = 0;
        size_t objEnd = objStart;
        for (size_t i = objStart; i < arr.size(); i++) {
            if (arr[i] == '{') oDepth++;
            else if (arr[i] == '}') { oDepth--; if (oDepth == 0) { objEnd = i; break; } }
        }
        std::string obj = arr.substr(objStart, objEnd - objStart + 1);

        auto* ph = new PhysicsHair();

        // Parse setup
        auto setupPos = obj.find("\"setup\"");
        if (setupPos != std::string::npos) {
            auto sOpen = obj.find('{', setupPos);
            int sDepth = 0;
            size_t sClose = sOpen;
            for (size_t i = sOpen; i < obj.size(); i++) {
                if (obj[i] == '{') sDepth++;
                else if (obj[i] == '}') { sDepth--; if (sDepth == 0) { sClose = i; break; } }
            }
            std::string setup = obj.substr(sOpen + 1, sClose - sOpen - 1);
            float length = 0.3f, resist = 0.5f, mass = 0.1f;
            auto readF = [&](const char* key, float def) -> float {
                auto kp = setup.find(key); if (kp == std::string::npos) return def;
                auto col = setup.find(':', kp); if (col == std::string::npos) return def;
                return (float)std::atof(setup.c_str() + col + 1);
            };
            length = readF("\"length\"", 0.3f);
            resist = readF("\"regist\"", 0.5f);
            mass   = readF("\"mass\"", 0.1f);
            ph->setup(length, resist, mass);
        }

        // Parse src
        auto srcPos = obj.find("\"src\"");
        if (srcPos != std::string::npos) {
            auto sa = obj.find('[', srcPos);
            if (sa != std::string::npos) {
                int sd = 0; size_t se = sa;
                for (size_t i = sa; i < obj.size(); i++) {
                    if (obj[i] == '[') sd++;
                    else if (obj[i] == ']') { sd--; if (sd == 0) { se = i; break; } }
                }
                std::string srcArr = obj.substr(sa + 1, se - sa - 1);
                size_t sp = 0;
                while (sp < srcArr.size()) {
                    auto so = srcArr.find('{', sp); if (so == std::string::npos) break;
                    int d2 = 0; size_t sc = so;
                    for (size_t i = so; i < srcArr.size(); i++) {
                        if (srcArr[i] == '{') d2++;
                        else if (srcArr[i] == '}') { d2--; if (d2 == 0) { sc = i; break; } }
                    }
                    std::string srcObj = srcArr.substr(so, sc - so + 1);
                    auto readS = [&](const char* key) -> std::string {
                        auto kp = srcObj.find(key); if (kp == std::string::npos) return "";
                        auto q1 = srcObj.find('"', kp + 5);
                        auto q2 = srcObj.find('"', q1 + 1);
                        if (q1 == std::string::npos || q2 == std::string::npos) return "";
                        return srcObj.substr(q1 + 1, q2 - q1 - 1);
                    };
                    auto readF2 = [&](const char* key, float def) -> float {
                        auto kp = srcObj.find(key); if (kp == std::string::npos) return def;
                        auto col = srcObj.find(':', kp); if (col == std::string::npos) return def;
                        return (float)std::atof(srcObj.c_str() + col + 1);
                    };
                    std::string tid = readS("\"id\"");
                    std::string ptype = readS("\"ptype\"");
                    float scale = readF2("\"scale\"", 1.0f);
                    float weight = readF2("\"weight\"", 1.0f);
                    PhysicsSrcType st;
                    if (ptype == "x") st = SRC_TO_X;
                    else if (ptype == "y") st = SRC_TO_Y;
                    else if (ptype == "angle") st = SRC_TO_G_ANGLE;
                    else { sp = sc + 1; continue; }
                    ph->addSrcParam(st, tid, scale, weight);
                    sp = sc + 1;
                }
            }
        }

        // Parse targets
        auto tgtPos = obj.find("\"targets\"");
        if (tgtPos != std::string::npos) {
            auto ta = obj.find('[', tgtPos);
            if (ta != std::string::npos) {
                int td = 0; size_t te = ta;
                for (size_t i = ta; i < obj.size(); i++) {
                    if (obj[i] == '[') td++;
                    else if (obj[i] == ']') { td--; if (td == 0) { te = i; break; } }
                }
                std::string tgtArr = obj.substr(ta + 1, te - ta - 1);
                size_t tp = 0;
                while (tp < tgtArr.size()) {
                    auto to = tgtArr.find('{', tp); if (to == std::string::npos) break;
                    int d3 = 0; size_t tc = to;
                    for (size_t i = to; i < tgtArr.size(); i++) {
                        if (tgtArr[i] == '{') d3++;
                        else if (tgtArr[i] == '}') { d3--; if (d3 == 0) { tc = i; break; } }
                    }
                    std::string tgtObj = tgtArr.substr(to, tc - to + 1);
                    auto readS = [&](const char* key) -> std::string {
                        auto kp = tgtObj.find(key); if (kp == std::string::npos) return "";
                        auto q1 = tgtObj.find('"', kp + 5);
                        auto q2 = tgtObj.find('"', q1 + 1);
                        if (q1 == std::string::npos || q2 == std::string::npos) return "";
                        return tgtObj.substr(q1 + 1, q2 - q1 - 1);
                    };
                    auto readF2 = [&](const char* key, float def) -> float {
                        auto kp = tgtObj.find(key); if (kp == std::string::npos) return def;
                        auto col = tgtObj.find(':', kp); if (col == std::string::npos) return def;
                        return (float)std::atof(tgtObj.c_str() + col + 1);
                    };
                    std::string tid = readS("\"id\"");
                    std::string ttype = readS("\"ptype\"");
                    float scale = readF2("\"scale\"", 1.0f);
                    float weight = readF2("\"weight\"", 1.0f);
                    PhysicsTargetType tt;
                    if (ttype == "angle") tt = TARGET_FROM_ANGLE;
                    else if (ttype == "angle_v") tt = TARGET_FROM_ANGLE_V;
                    else { tp = tc + 1; continue; }
                    ph->addTargetParam(tt, tid, scale, weight);
                    tp = tc + 1;
                }
            }
        }

        ret->mPhysicsList.push_back(ph);
        pos = objEnd + 1;
    }
    return ret;
}

} // namespace live2d
