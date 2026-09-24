#pragma once
#include <string>
namespace live2d {
struct L2DExpressionParam {
    std::string mId; float mValue = 0; float mDefValue = 0; int mBlendType = 1;
};
} // namespace live2d
