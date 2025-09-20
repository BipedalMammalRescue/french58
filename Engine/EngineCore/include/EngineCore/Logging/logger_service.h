#pragma once
#include <time.h>
#include <stdio.h>

namespace Engine
{
namespace Core
{
namespace Logging
{

// TODO: replace this logging with a threaded logger sending binary data
class LoggerService
{
private:
	template <typename... TArgs>
	void Log(const char* level, const char* channel, const char* message, TArgs... args)
	{
		time_t timeNow = time(nullptr);
		tm* now = localtime(&timeNow);

		printf("[%d:%d:%d][%s][%s] ", now->tm_hour, now->tm_min, now->tm_sec, level, channel);
		printf(message, args...);
		printf("\n");
	}

public:
	template <typename... TArgs>
	void Verbose(const char* channel, const char* message, TArgs... args)
	{
		Log("VER", channel, message, args...);
	}

	template <typename... TArgs>
	void Information(const char* channel, const char* message, TArgs... args)
	{
		Log("INF", channel, message, args...);
	}
	
	template <typename... TArgs>
	void Error(const char* channel, const char* message, TArgs... args)
	{
		Log("ERR", channel, message, args...);
	}

	template <typename... TArgs>
	void Warning(const char* channel, const char* message, TArgs... args)
	{
		Log("WRN", channel, message, args...);
	}
};

LoggerService* GetLogger();

}
}
}
