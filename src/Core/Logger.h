#pragma once
#include <cassert>
#include <chrono>
#include <iostream>
#include <fmt/chrono.h>
#include "fmt/core.h"
enum LogLevel {
	INFO = 0,
	WARN,
	ERR
};

class Logger
{
public:

	// Helper function to convert any type to string using stringstream
	template<typename T>
	std::string ToString(const T& value) {
		std::ostringstream oss;
		oss << value;
		return oss.str();
	}

	// Variadic template function to accept multiple arguments of any type
	template<typename... Args>
	static void Log(LogLevel level, Args... args) {
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

		// Print formatted log message with color reset immediately after the tag
		std::cout << fmt::format("[{}] [{}{}] {}\n", timestamp, levelString, "\033[0m", combinedStream.str());
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
		default:
			return "\033[37mUNKNOWN";
		}
	}
};
