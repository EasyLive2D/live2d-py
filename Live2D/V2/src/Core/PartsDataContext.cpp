#include "PartsDataContext.hpp"

namespace live2d {

void PartsDataContext::setPartScreenColor(float r, float g, float b, float a) {
    mScreenColor = {r, g, b, a};
}

void PartsDataContext::setPartMultiplyColor(float r, float g, float b, float a) {
    mMultiplyColor = {r, g, b, a};
}

} // namespace live2d
