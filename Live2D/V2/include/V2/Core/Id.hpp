#pragma once

#include <functional>
#include <string>
#include <unordered_map>

namespace Live2D {
namespace V2 {


class Id {
public:
    class IdPool {
    public:
        IdPool();
        ~IdPool();

        Id* Resolve(const std::string& idStr);

    private:
        std::unordered_map<std::string, Id*> mIds;
    };

    const std::string& Str() const { return mId; }

    // compare pointer equality only
    bool operator==(const Id& other) = delete;
    bool operator!=(const Id& other) = delete;
    bool operator==(const std::string& s) const { return mId == s; }
    bool operator!=(const std::string& s) const { return mId != s; }

    static Id* DST_BASE_ID();

private:
    Id(const std::string& idStr);

    std::string mId;
};

}   // namespace V2
}   // namespace Live2D

namespace std {
template<>
struct hash<Live2D::V2::Id> {
    size_t operator()(const Live2D::V2::Id& id) const noexcept { return hash<string>()(id.Str()); }
};
}   // namespace std
