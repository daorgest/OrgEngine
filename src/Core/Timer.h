//
// Created by Orgest on 8/31/2024.
//

#ifndef TIME_H
#define TIME_H
#include <chrono>
#include <iostream>
#include <utility>

class Timer
{
public:
	explicit Timer(std::string  functionName)
		: m_FunctionName(std::move(functionName))
	{
		Reset();
	}

	~Timer()
	{
		std::cout << "Elapsed time for function " << m_FunctionName << ": " << ElapsedMillis() << " ms" << std::endl;
	}

	void Reset() { m_Start = std::chrono::high_resolution_clock::now(); }

	float Elapsed() const
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_Start).count() * 0.000001f;
	}

	float ElapsedMillis() const
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_Start).count();
	}

private:
	std::string m_FunctionName;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
};


#endif //TIME_H
