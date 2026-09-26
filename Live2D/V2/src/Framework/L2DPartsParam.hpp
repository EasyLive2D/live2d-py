#pragma once
#include <string>
#include <vector>
namespace Live2D {
namespace V2 {
struct L2DPartsParam
{
    std::string mId;
    int mLinkCount = 0;
    std::vector<std::string> mLinkIds;
};
}   // namespace V2
}   // namespace Live2D
