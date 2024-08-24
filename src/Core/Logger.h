#pragma once
#include <iostream>
#include <fmt/chrono.h>
#include <fmt/core.h>

enum LogLevel
{
    INFO,
    WARN,
    ERR,
    DEBUGGING
};

enum DateFormat
{
    DATE_FORMAT_12H,
    DATE_FORMAT_24H
};

inline void DebugBreak()
{
#ifdef _MSC_VER
    __debugbreak();
#else
    __builtin_trap();
#endif
}

namespace Logger
{
    inline DateFormat currentDateFormat = DATE_FORMAT_24H;
    inline bool includeTimestamp = true;

    inline void Init()
    {
        std::cout << "Logger initialized\n";
    }

    inline const char* LogLevelToString(LogLevel level)
    {
        switch (level)
        {
            case INFO:      return "\033[32mINFO";
            case WARN:      return "\033[33mWARN";
            case ERR:       return "\033[31mERROR";
            case DEBUGGING: return "\033[34mDEBUG";
            default:        return "\033[37mUNKNOWN";
        }
    }

    inline std::string GetTimestamp()
    {
        if (!includeTimestamp)
			return "";

		const auto now = std::chrono::system_clock::now();
		const auto now_c = std::chrono::system_clock::to_time_t(now);
        std::tm localTime{};
        localtime_s(&localTime, &now_c);

        switch (currentDateFormat)
        {
            case DATE_FORMAT_12H:
                return fmt::format("{:%m-%d-%Y %I:%M %p}", localTime);
            case DATE_FORMAT_24H:
                return fmt::format("{:%m-%d-%Y %H:%M}", localTime);
            default:
                return "";
        }
    }

    template<typename... Args>
    void Log(LogLevel level, const char* file, int line, Args... args)
    {
        std::ostringstream combinedStream;
        (combinedStream << ... << args);

        std::string timestamp = GetTimestamp();
        const char* levelString = LogLevelToString(level);

        if (timestamp.empty())
        {
            if (level == ERR)
            {
                std::cout << fmt::format("[{}{}] {}:{} - {}\n", levelString, "\033[0m", file, line, combinedStream.str());
#ifdef _DEBUG
                DebugBreak();
#endif
            }
            else
            {
                std::cout << fmt::format("[{}{}] {}\n", levelString, "\033[0m", combinedStream.str());
            }
        }
        else
        {
            if (level == ERR)
            {
                std::cout << fmt::format("[{}] [{}{}] {}:{} - {}\n", timestamp, levelString, "\033[0m", file, line, combinedStream.str());
#ifdef _DEBUG
                DebugBreak();
#endif
            }
            else
            {
                std::cout << fmt::format("[{}] [{}{}] {}\n", timestamp, levelString, "\033[0m", combinedStream.str());
            }
        }
    }

    inline void SetDateFormat(DateFormat format)
    {
        currentDateFormat = format;
    }

    inline void EnableTimestamp(bool enable)
    {
        includeTimestamp = enable;
    }
};

#ifndef NDEBUG
#define LOG(level, ...) Logger::Log(level, __FILE__, __LINE__, __VA_ARGS__)
#else
#define LOG(level, ...) (void)0
#endif // NDEBUG
