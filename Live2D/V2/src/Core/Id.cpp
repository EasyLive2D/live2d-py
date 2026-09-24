#include "Id.hpp"

namespace live2d {

std::unordered_map<std::string, std::unique_ptr<Id>> Id::sInstances;

const Id& Id::DST_BASE_ID() {
    return Id::getID("DST_BASE");
}

const Id& Id::getID(const std::string& idStr) {
    auto it = sInstances.find(idStr);
    if (it != sInstances.end()) {
        return *it->second;
    }
    auto id = std::make_unique<Id>(idStr);
    const Id& ref = *id;
    sInstances[idStr] = std::move(id);
    return ref;
}

void Id::releaseStored() {
    sInstances.clear();
}

} // namespace live2d
