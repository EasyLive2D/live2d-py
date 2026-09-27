#include "UtSystem.hpp"
#include <chrono>

namespace Live2D {
namespace V2 {

double UtSystem::getUserTimeMSec()
{
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(now.time_since_epoch()).count();
}

}   // namespace V2
}   // namespace Live2D