#include "UtSystem.hpp"
#include <chrono>

namespace live2d {

double UtSystem::getUserTimeMSec() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(
               now.time_since_epoch()).count();
}

} // namespace live2d
