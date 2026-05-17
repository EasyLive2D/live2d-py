#include "UtSystem.hpp"
#include <chrono>

namespace live2d {

int64_t UtSystem::getUserTimeMSec() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               now.time_since_epoch())
        .count();
}

} // namespace live2d
