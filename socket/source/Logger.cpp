#include "Logger.h"
#include <memory>

Logger* GetLogger()
{
	static std::shared_ptr<Logger> s_pLogger;
	if (!s_pLogger)
	{
		s_pLogger = std::make_shared<Logger>();
	}
	return s_pLogger.get();
}

std::string Logger::GetTimeNow() const
{
	std::time_t now = std::time(nullptr);
	std::tm now_tm;
#if defined(_WIN32) || defined(_WIN64)
	localtime_s(&now_tm, &now);
#else
	localtime_r(&now, &now_tm);
#endif
	const int offset_year = 1900;
	const int offset_month = 1;
	char Buffer[64] = { '\0' };
	SPRINTF_S(Buffer, "%d-%02d-%02d %02d:%02d:%02d", now_tm.tm_year + offset_year, now_tm.tm_mon + offset_month, now_tm.tm_mday,
		now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec);
	return std::string(Buffer);
}

void Logger::LogInternal(Level level, const std::string& msg) const
{
	std::string Lev = "Unknown";
	switch (level)
	{
	case Logger::LEVEL_INFO:
		Lev = "Info";
		break;
	case Logger::LEVEL_WARN:
		Lev = "Warn";
		break;
	case Logger::LEVEL_ERROR:
		Lev = "Error";
		break;
	case Logger::LEVEL_FATAL:
		Lev = "Fatal";
		break;
	default:
		break;
	}
	std::string res = GetTimeNow();
	res = "[" + res + "]";
	res += "[" + Lev + "]";
	res += msg;
	printf("%s", res.c_str());
}
