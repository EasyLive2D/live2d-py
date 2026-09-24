#pragma once
#include <string>
#include <vector>
namespace live2d {
struct Motion {
    std::string mParamId;
    std::string mSecondaryId;
    std::vector<float> mValues;
};
} // namespace live2d
