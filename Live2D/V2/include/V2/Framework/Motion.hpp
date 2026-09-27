#pragma once
#include <string>
#include <vector>
namespace Live2D {
namespace V2 {
struct Motion
{
    std::string mParamId;
    std::string mSecondaryId;
    std::vector<float> mValues;
};
}   // namespace V2
}   // namespace Live2D