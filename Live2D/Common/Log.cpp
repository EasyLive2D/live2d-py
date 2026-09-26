#include "Log.hpp"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <mutex>

#ifdef CSM_TARGET_ANDROID_ES2
#include <android/log.h>
#define TAG "live2d-py"
#endif

namespace Live2D {
namespace Common {

namespace Log {

std::atomic<bool> sLive2DLogEnable(true);
std::atomic<int> sLive2DLogLevel(LV_INFO);

void EnableLive2DLog(bool on)
{
    sLive2DLogEnable.store(on);
}

bool IsLive2DLogEnabled()
{
    return sLive2DLogEnable.load();
}

void SetLive2DLogLevel(int level)
{
    sLive2DLogLevel.store(level);
}

int GetLive2DLogLevel()
{
    return sLive2DLogLevel.load();
}

void live2d_log_print(const char* tag, Levels level, const char* fmt, ...)
{
    if (sLive2DLogEnable.load() && level >= sLive2DLogLevel.load()) {
        char levelTag;
        switch (level) {
            case LV_DEBUG:
                levelTag = 'D';
                break;
            case LV_INFO:
                levelTag = 'I';
                break;
            case LV_WARN:
                levelTag = 'W';
                break;
            case LV_ERROR:
                levelTag = 'E';
                break;
        }
#ifndef CSM_TARGET_ANDROID_ES2
        if (tag) {
            printf("[%s][%c] ", tag, levelTag);
        } else {
            printf("[%c] ", levelTag);
        }
#endif
        va_list args;
        va_start(args, fmt);
#ifdef CSM_TARGET_ANDROID_ES2
        int androidLevel;
        switch (level) {
            case LV_DEBUG:
                androidLevel = ANDROID_LOG_DEBUG;
                break;
            case LV_INFO:
                androidLevel = ANDROID_LOG_INFO;
                break;
            case LV_WARN:
                androidLevel = ANDROID_LOG_WARN;
                break;
            case LV_ERROR:
                androidLevel = ANDROID_LOG_ERROR;
                break;
            default:
                androidLevel = ANDROID_LOG_DEBUG;
                break;
        }
        __android_log_vprint(ANDROID_LOG_DEBUG, TAG, fmt, args);
#else
        vfprintf(stdout, fmt, args);
#endif
        va_end(args);
#ifndef CSM_TARGET_ANDROID_ES2
        printf("\n");
#endif
    }
}




}   // namespace Log
}   // namespace Common
}   // namespace Live2D
