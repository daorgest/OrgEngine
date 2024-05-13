#pragma once
#include <chrono>
#include <iostream>
#ifndef NDEBUG
#include <fmt/chrono.h>
#include <fmt/core.h>

#define LOG(level, ...) Logger::Log(level, __FILE__, __LINE__, __VA_ARGS__)

enum LogLevel {
	INFO,
	WARN,
	ERR,
	DEBUGGING
};

class Logger {
public:
	template<typename... Args>
	static void Log(LogLevel level, const char* file, int line, Args... args) {
		std::ostringstream combinedStream;
		(combinedStream << ... << args);

		auto now = std::chrono::system_clock::now();
		auto now_c = std::chrono::system_clock::to_time_t(now);
		std::tm local_tm{};
		localtime_s(&local_tm, &now_c);

		std::string timestamp = fmt::format("{:%m-%d-%Y %I:%M %p}", local_tm);
		std::string levelString = LogLevelToString(level);

		std::string fileLineInfo = (level == ERR) ? fmt::format("{}:{}", file, line) : "";
		std::string message = combinedStream.str();

		if (level == ERR) {
			std::cout << fmt::format("[{}] [{}{}] {} - {}\n", timestamp, levelString, "\033[0m", fileLineInfo, message);
		} else {
			std::cout << fmt::format("[{}] [{}{}] {}\n", timestamp, levelString, "\033[0m", message);
		}
	}

private:
	static std::string LogLevelToString(LogLevel level) {
		switch (level) {
		case INFO:      return "\033[32mINFO";
		case WARN:      return "\033[33mWARN";
		case ERR:       return "\033[31mERROR";
		case DEBUGGING: return "\033[34mDEBUG";
		default:        return "\033[37mUNKNOWN";
		}
	}
};

#else

#define LOG(level, ...) (void)0

#endif // NDEBUG
