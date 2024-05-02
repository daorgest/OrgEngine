#pragma once
#include <chrono>
#include <iostream>
#include <fmt/chrono.h>
#include "fmt/core.h"

#define LOG(level, ...) Logger::Log(level, __FILE__, __LINE__, __VA_ARGS__)

enum LogLevel {
	INFO = 0,
	WARN,
	ERR,
	DEBUGGING
};

class Logger
{
public:
	// Variadic template function to accept multiple arguments of any type
	template<typename... Args>
	static void Log(LogLevel level, const char* file, int line, Args... args) {
		using namespace std::chrono;

		// Use stringstream to handle different data types
		std::ostringstream combinedStream;
		(combinedStream << ... << args); // Fold expression to insert all arguments into the stream

		// Get the current time
		const auto now = system_clock::now();
		const auto now_c = system_clock::to_time_t(now);
		auto local_tm = *std::localtime(&now_c);

		// Format the timestamp using local time and include AM/PM
		std::string timestamp = fmt::format("{:%m-%d-%Y %I:%M %p}", local_tm);
		std::string levelString = LogLevelToString(level); // Convert the log level to a string with color

		// Conditionally append file and line information if the log level is ERR
		std::string fileLineInfo = (level == ERR) ? fmt::format("{}:{}", file, line) : "";
		std::string message = combinedStream.str();

		// Print formatted log message with color reset immediately after the tag
		if (level == ERR) {
			std::cout << fmt::format("[{}] [{}{}] {} - {}\n", timestamp, levelString, "\033[0m", fileLineInfo, message);
		} else {
			std::cout << fmt::format("[{}] [{}{}] {}\n", timestamp, levelString, "\033[0m", message);
		}
	}

private:
	static std::string LogLevelToString(const LogLevel level) {
		switch (level) {
		case INFO:
			return "\u001b[32mINFO";
		case WARN:
			return "\033[33mWARN";
		case ERR:
			return "\033[31mERROR";
		case DEBUGGING:
			return "\033[34mDEBUG";
		default:
			return "\033[37mUNKNOWN";
		}
	}
};

