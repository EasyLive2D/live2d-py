#pragma once

#ifndef MODULE_LOG_TAG
#define MODULE_LOG_TAG ""
#endif

namespace Live2D {
namespace Common {
namespace Log {

enum Levels
{
    LV_DEBUG = 0,
    LV_INFO,
    LV_WARN,
    LV_ERROR,
};

void EnableLive2DLog(bool on);

bool IsLive2DLogEnabled();

void SetLive2DLogLevel(int level);

int GetLive2DLogLevel();

void live2d_log_print(const char* tag, Levels level, const char* fmt, ...);

#define LOGD(...) live2d_log_print(MODULE_LOG_TAG, Live2D::Common::Log::Levels::LV_DEBUG, __VA_ARGS__)
#define LOGI(...) live2d_log_print(MODULE_LOG_TAG, Live2D::Common::Log::Levels::LV_INFO, __VA_ARGS__)
#define LOGW(...) live2d_log_print(MODULE_LOG_TAG, Live2D::Common::Log::Levels::LV_WARN, __VA_ARGS__)
#define LOGE(...) live2d_log_print(MODULE_LOG_TAG, Live2D::Common::Log::Levels::LV_ERROR, __VA_ARGS__)

}   // namespace Log
}   // namespace Common
}   // namespace Live2D
