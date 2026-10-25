#include "Log.hpp"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <atomic>

// for std::memcpy
#ifdef __linux__
#include <cstring>
#endif

// use render-irrelevant marco
#ifdef __ANDROID__
#include <android/log.h>
#define TAG "live2d-py"
#endif

namespace Live2D {
namespace Common {

namespace Log {

std::atomic<bool> sLive2DLogEnable(true);
std::atomic<int> sLive2DLogLevel(LV_INFO);

void EnableLive2DLog(bool on) {
    sLive2DLogEnable.store(on, std::memory_order_relaxed);
}

bool IsLive2DLogEnabled() {
    return sLive2DLogEnable.load(std::memory_order_relaxed);
}

void SetLive2DLogLevel(int level) {
    sLive2DLogLevel.store(level, std::memory_order_relaxed);
}

int GetLive2DLogLevel() {
    return sLive2DLogLevel.load(std::memory_order_relaxed);
}

void live2d_log_print(const char* tag, Levels level, const char* fmt, ...) {
    // 1) 热路径兜底：宏层已判，这里防有人绕过宏直接调用
    if (!sLive2DLogEnable.load(std::memory_order_relaxed))
        return;
    if (level < sLive2DLogLevel.load(std::memory_order_relaxed))
        return;

    // 2) levelTag：非法值用 '?'，绝不产生 '\0'（否则 %c 会往 line 注入截断符）
    static constexpr char kTags[] = {'D', 'I', 'W', 'E', 'F'};
    const char levelTag = (level >= LV_DEBUG && level <= LV_FATAL) ? kTags[level] : '?';

    // 3) 正文格式化：一次 vsnprintf，clamp 返回值
    char msg[1024];
    va_list args;
    va_start(args, fmt);
    int n = std::vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);
    if (n < 0)
        return;
    if (n >= (int)sizeof(msg))
        n = (int)sizeof(msg) - 1;
    msg[n] = '\0';

#ifdef CSM_TARGET_ANDROID_ES2
    // 4a) Android：msg 已格式化，直接 write；tag 缺失时退回默认 TAG
    int prio;
    switch (level) {
        case LV_DEBUG:
            prio = ANDROID_LOG_DEBUG;
            break;
        case LV_INFO:
            prio = ANDROID_LOG_INFO;
            break;
        case LV_WARN:
            prio = ANDROID_LOG_WARN;
            break;
        case LV_ERROR:
            prio = ANDROID_LOG_ERROR;
            break;
        case LV_FATAL:
            prio = ANDROID_LOG_FATAL;
            break;
        default:
            prio = ANDROID_LOG_DEBUG;
            break;
    }
    __android_log_write(prio, tag ? tag : TAG, msg);
#else
    // 4b) PC：前缀一次 snprintf + 立即 clamp；正文 memcpy；单次 fwrite
    char line[1200];
    int len;
    if (tag) {
        len = std::snprintf(line, sizeof(line), "[%s][%c] ", tag, levelTag);
    } else {
        len = std::snprintf(line, sizeof(line), "[%c] ", levelTag);
    }

    if (len < 0) [[unlikely]]
        return;
    if ((size_t)len >= sizeof(line))
        len = (int)sizeof(line) - 1;   // 截断就不再加正文

    // 正文长度已知（n，已 clamp），直接 memcpy，不再走一次 snprintf
    size_t remain = sizeof(line) - (size_t)len;
    size_t msgLen = (size_t)n;
    size_t maxMsg = (remain >= 2) ? remain - 2 : 0;   // 给 '\n' 和 '\0' 留位
    if (msgLen > maxMsg)
        msgLen = maxMsg;

    std::memcpy(line + len, msg, msgLen);
    len += (int)msgLen;
    line[len++] = '\n';
    line[len] = '\0';

    // 单次 fwrite：一次 stdout 锁，不扫描 '\0'，不依赖 null 终止
    std::fwrite(line, 1, (size_t)len, stdout);
    std::fflush(stdout);
#endif
}




}   // namespace Log
}   // namespace Common
}   // namespace Live2D
