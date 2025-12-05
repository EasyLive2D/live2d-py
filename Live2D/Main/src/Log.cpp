#include "Log.hpp"
#include <ctime>
#include <cstdio>
#include <cstdarg>

#ifdef CSM_TARGET_ANDROID_ES2
#include <android/log.h>
#define TAG "live2d-py"
#endif

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

void Debug(const char *fmt, ...)
{
	if (sLive2DLogEnable.load() && LV_DEBUG >= sLive2DLogLevel.load())
	{
#ifndef CSM_TARGET_ANDROID_ES2
		printf("\033[34m[DEBUG] ");
#endif
		va_list args;
		va_start(args, fmt);
#ifdef CSM_TARGET_ANDROID_ES2
		__android_log_vprint(ANDROID_LOG_DEBUG, TAG, fmt, args);
#else
		vfprintf(stdout, fmt, args);
#endif
		va_end(args);
#ifndef CSM_TARGET_ANDROID_ES2
		printf("\033[0m\n");
#endif
	}
}

void Info(const char *fmt, ...)
{
	if (sLive2DLogEnable.load() && LV_INFO >= sLive2DLogLevel.load())
	{
#ifndef CSM_TARGET_ANDROID_ES2
		printf("[INFO]  ");
#endif
		va_list args;
		va_start(args, fmt);
#ifdef CSM_TARGET_ANDROID_ES2
		__android_log_vprint(ANDROID_LOG_INFO, TAG, fmt, args);
#else
		vfprintf(stdout, fmt, args);
#endif
		va_end(args);
#ifndef CSM_TARGET_ANDROID_ES2
		printf("\n");
#endif
	}
}

void Warn(const char *fmt, ...)
{
	if (sLive2DLogEnable.load() && LV_WARN >= sLive2DLogLevel.load())
	{
#ifndef CSM_TARGET_ANDROID_ES2
		printf("\033[33m[WARN]  ");
#endif
		va_list args;
		va_start(args, fmt);
#ifdef CSM_TARGET_ANDROID_ES2
		__android_log_vprint(ANDROID_LOG_WARN, TAG, fmt, args);
#else
		vfprintf(stdout, fmt, args);
#endif
		va_end(args);
#ifndef CSM_TARGET_ANDROID_ES2
		printf("\033[0m\n");
#endif
	}
}

void Error(const char *fmt, ...)
{
	if (sLive2DLogEnable.load() && LV_ERROR >= sLive2DLogLevel.load())
	{
#ifndef CSM_TARGET_ANDROID_ES2
		printf("\033[31m[ERROR] ");
#endif
		va_list args;
		va_start(args, fmt);
#ifdef CSM_TARGET_ANDROID_ES2
		__android_log_vprint(ANDROID_LOG_ERROR, TAG, fmt, args);
#else
		vfprintf(stdout, fmt, args);
#endif
		va_end(args);
#ifndef CSM_TARGET_ANDROID_ES2
		printf("\033[0m\n");
#endif
	}
}