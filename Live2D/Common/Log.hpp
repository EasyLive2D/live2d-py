#pragma once

#ifndef MODULE_LOG_TAG
#define MODULE_LOG_TAG nullptr
#endif

namespace Live2D {
namespace Common {
namespace Log {

enum Levels {
    LV_DEBUG = 0,
    LV_INFO,
    LV_WARN,
    LV_ERROR,
    LV_FATAL,
};

void EnableLive2DLog(bool on);

bool IsLive2DLogEnabled();

void SetLive2DLogLevel(int level);

int GetLive2DLogLevel();

void live2d_log_print(const char* tag, Levels level, const char* fmt, ...);

#define LOGD(...)                                                                      \
    do {                                                                               \
        if (::Live2D::Common::Log::IsLive2DLogEnabled() &&                             \
            ::Live2D::Common::Log::Levels::LV_DEBUG >=                                 \
                ::Live2D::Common::Log::GetLive2DLogLevel()) {                          \
            ::Live2D::Common::Log::live2d_log_print(                                   \
                MODULE_LOG_TAG, ::Live2D::Common::Log::Levels::LV_DEBUG, __VA_ARGS__); \
        }                                                                              \
    } while (0)

#define LOGI(...)                                                                     \
    do {                                                                              \
        if (::Live2D::Common::Log::IsLive2DLogEnabled() &&                            \
            ::Live2D::Common::Log::Levels::LV_INFO >=                                 \
                ::Live2D::Common::Log::GetLive2DLogLevel()) {                         \
            ::Live2D::Common::Log::live2d_log_print(                                  \
                MODULE_LOG_TAG, ::Live2D::Common::Log::Levels::LV_INFO, __VA_ARGS__); \
        }                                                                             \
    } while (0)

#define LOGW(...)                                                                     \
    do {                                                                              \
        if (::Live2D::Common::Log::IsLive2DLogEnabled() &&                            \
            ::Live2D::Common::Log::Levels::LV_WARN >=                                 \
                ::Live2D::Common::Log::GetLive2DLogLevel()) {                         \
            ::Live2D::Common::Log::live2d_log_print(                                  \
                MODULE_LOG_TAG, ::Live2D::Common::Log::Levels::LV_WARN, __VA_ARGS__); \
        }                                                                             \
    } while (0)

#define LOGE(...)                                                                      \
    do {                                                                               \
        if (::Live2D::Common::Log::IsLive2DLogEnabled() &&                             \
            ::Live2D::Common::Log::Levels::LV_ERROR >=                                 \
                ::Live2D::Common::Log::GetLive2DLogLevel()) {                          \
            ::Live2D::Common::Log::live2d_log_print(                                   \
                MODULE_LOG_TAG, ::Live2D::Common::Log::Levels::LV_ERROR, __VA_ARGS__); \
        }                                                                              \
    } while (0)

#define LOGF(...)                                                                  \
    do {                                                                           \
        ::Live2D::Common::Log::live2d_log_print(                                   \
            MODULE_LOG_TAG, ::Live2D::Common::Log::Levels::LV_FATAL, __VA_ARGS__); \
        std::abort();                                                              \
    } while (0)

}   // namespace Log
}   // namespace Common
}   // namespace Live2D
