#pragma once
#include <string>
#include <ctime>
#include <iostream>

#define LOGGER_BUFFER_SIZE 1024
inline char Logger_Buffer[LOGGER_BUFFER_SIZE] = { 0 };
class Logger;
Logger* GetLogger();
#ifdef __STDC_LIB_EXT1__
#define SPRINTF_S(buffer, ...) sprintf_s(buffer, ##__VA_ARGS__)
#else
#define SPRINTF_S(buffer, ...) sprintf(buffer, ##__VA_ARGS__)
#endif
#define Log(...)	\
{													\
	Logger* logger = GetLogger();					\
	SPRINTF_S(Logger_Buffer, ##__VA_ARGS__);		\
	logger->LogInfo(Logger_Buffer);						\
};

#define LogWarn(...)	\
{													\
	Logger* logger = GetLogger();					\
	SPRINTF_S(Logger_Buffer, ##__VA_ARGS__);		\
	logger->Log_Warn(Logger_Buffer);						\
};

#define LogError(...)	\
{													\
	Logger* logger = GetLogger();					\
	SPRINTF_S(Logger_Buffer, ##__VA_ARGS__);		\
	logger->Log_Error(Logger_Buffer);						\
};




class Logger
{
	enum Level
	{
		LEVEL_INFO,
		LEVEL_WARN,
		LEVEL_ERROR,
		LEVEL_FATAL
	};
protected:
	std::string GetTimeNow()const;
	void LogInternal(Level level, const std::string& msg) const;
public:
	virtual void LogInfo(const std::string& msg) const
	{
		LogInternal(Level::LEVEL_INFO, msg);
	}
	virtual void Log_Warn(const std::string& msg)const
	{
		LogInternal(Level::LEVEL_WARN, msg);
	}
	virtual void Log_Error(const std::string& msg)const
	{
		LogInternal(Level::LEVEL_ERROR, msg);
	}
	virtual void Log_Fatal(const std::string& msg)const
	{
		LogInternal(Level::LEVEL_FATAL, msg);
	}
};


