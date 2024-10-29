//
// Created by Orgest on 8/31/2024.
//

#ifndef TIME_H
#define TIME_H
#include <chrono>
#include <unordered_map>

#include "PrimTypes.h"

class Timer
{
public:
	explicit Timer(const std::string_view functionName, std::unordered_map<std::string, f32>& timingResults)
		: m_FunctionName(functionName), m_TimingResults(timingResults)
	{
		Reset();
	}

	~Timer()
	{
		const f32 elapsedMillis = ElapsedMillis();
		m_TimingResults[m_FunctionName] = elapsedMillis;
	}

	void Reset() { m_Start = std::chrono::high_resolution_clock::now(); }

	[[nodiscard]] f32 Elapsed() const
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_Start).count() * 0.000001f;
	}

	[[nodiscard]] f32 ElapsedMillis() const
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_Start).count();
	}

private:
	std::string m_FunctionName;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;

	// Reference to the timing results map where we'll store the elapsed time
	std::unordered_map<std::string, f32>& m_TimingResults;
};


#endif //TIME_H
