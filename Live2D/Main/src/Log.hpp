#pragma once

#include <atomic>

enum Live2DLogLevels 
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

void Debug(const char *fmt, ...);

void Info(const char *fmt, ...);

void Warn(const char *fmt, ...);

void Error(const char *fmt, ...);
