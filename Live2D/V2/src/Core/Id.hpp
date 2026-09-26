#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace Live2D {
namespace V2 {

class Id
{
public:
    explicit Id(const std::string& idStr)
        : mId(idStr)
    {}

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

}   // namespace V2
}   // namespace Live2D

namespace std {
template<>
struct hash<Live2D::V2::Id>
{
    size_t operator()(const Live2D::V2::Id& id) const noexcept { return hash<string>()(id.str()); }
};
}   // namespace std
