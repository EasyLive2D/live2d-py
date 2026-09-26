#include "PartsDataContext.hpp"

namespace Live2D {
namespace V2 {

void PartsDataContext::setPartScreenColor(float r, float g, float b, float a)
{
    mScreenColor = {r, g, b, a};
}

void PartsDataContext::setPartMultiplyColor(float r, float g, float b, float a)
{
    mMultiplyColor = {r, g, b, a};
}

}   // namespace V2
}   // namespace Live2D