#include "Physics.hpp"
#include "nlohmann/json.hpp"
#include <string>

namespace Live2D {
namespace V2 {

using json = nlohmann::json;

Physics::Physics() = default;

void Physics::UpdateParam(Moc* moc, long long dtMs) {
    mElapsedMs += dtMs;
    for (auto& ph : mPhysicsList)
        ph->Update(moc, mElapsedMs);
}

Physics* Physics::Load(const std::vector<uint8_t>& data, Id::IdPool& idPool) {
    auto* ret = new Physics();
    // Python: pm.jsonParseFromBytes == json.loads (l2d_physics.py:17-69)
    json root = json::parse(std::string((const char*)data.data(), data.size()));
    auto params = root.find("physics_hair");
    if (params == root.end() || !params->is_array())
        return ret;

    for (const auto& param : *params) {
        if (!param.is_object())
            continue;
        auto hair = std::make_unique<PhysicsHair>();

        auto setup = param.find("setup");
        if (setup != param.end() && setup->is_object()) {
            // PARITY QUIRK (l2d_physics.py:27): Python uses float(len(setup)) — the
            // number of keys in "setup" (3 for every real file) — NOT setup["length"].
            hair->Setup((float)setup->size(),
                        setup->value("regist", 0.5f),   // Cubism 2 spelling of "resist"
                        setup->value("mass", 0.1f));
        }

        auto srcList = param.find("src");
        if (srcList != param.end() && srcList->is_array()) {
            for (const auto& src : *srcList) {
                if (!src.is_object())
                    continue;
                std::string ptype = src.value("ptype", std::string());
                PhysicsSrcType t;
                if (ptype == "x")
                    t = SRC_TO_X;
                else if (ptype == "y")
                    t = SRC_TO_Y;
                else if (ptype == "angle")
                    t = SRC_TO_G_ANGLE;
                else
                    continue;   // Python raises here; skip instead
                hair->AddSrcParam(t,
                                  idPool.Resolve(src.value("id", std::string())),
                                  src.value("scale", 1.0f),
                                  src.value("weight", 1.0f));
            }
        }

        auto tgtList = param.find("targets");
        if (tgtList != param.end() && tgtList->is_array()) {
            for (const auto& target : *tgtList) {
                if (!target.is_object())
                    continue;
                std::string ptype = target.value("ptype", std::string());
                PhysicsTargetType t;
                if (ptype == "angle")
                    t = TARGET_FROM_ANGLE;
                else if (ptype == "angle_v")
                    t = TARGET_FROM_ANGLE_V;
                else
                    continue;   // Python raises here; skip instead
                hair->AddTargetParam(t,
                                     idPool.Resolve(target.value("id", std::string())),
                                     target.value("scale", 1.0f),
                                     target.value("weight", 1.0f));
            }
        }

        ret->mPhysicsList.push_back(std::move(hair));
    }
    return ret;
}

}   // namespace V2
}   // namespace Live2D