#pragma once

#include <string>
#include <unordered_map>
#include <memory>

namespace live2d {

class Id {
public:
    explicit Id(const std::string& idStr) : mId(idStr) {}

    const std::string& str() const { return mId; }

    bool operator==(const Id& other) const { return mId == other.mId; }
    bool operator!=(const Id& other) const { return mId != other.mId; }
    bool operator==(const std::string& s) const { return mId == s; }
    bool operator!=(const std::string& s) const { return mId != s; }

    static const Id& DST_BASE_ID();
    static const Id& getID(const std::string& idStr);
    static void releaseStored();

private:
    std::string mId;
    static std::unordered_map<std::string, std::unique_ptr<Id>> sInstances;
};

} // namespace live2d

namespace std {
template <>
struct hash<live2d::Id> {
    size_t operator()(const live2d::Id& id) const noexcept {
        return hash<string>()(id.str());
    }
};
} // namespace std
