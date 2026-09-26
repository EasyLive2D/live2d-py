#include "Debug.hpp"

#include "Log.hpp"
#include <backward/backward.hpp>
#include <mutex>


namespace Live2D {
namespace Common {

namespace Debug {


void InstallCrashHandler()
{
    static backward::SignalHandling sh;
    static std::once_flag flag;
    std::call_once(flag, []() { Log::LOGI("Crash Handler installed"); });
}

}   // namespace Debug
}   // namespace Common
}   // namespace Live2D