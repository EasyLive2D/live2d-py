#pragma once
#include <string>
#include <vector>
namespace live2d {
struct L2DPartsParam {
    std::string mId; int mLinkCount = 0; std::vector<std::string> mLinkIds;
};
} // namespace live2d
