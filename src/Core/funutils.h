//
// Created by Orgest on 10/8/2024.
//

// Some utils i created/ stole
#pragma once
/**
 * @brief Computes the greatest common divisor (GCD) of two numbers using the Euclidean algorithm.
 */
template<typename T>
T gcd(T a, T b)
{
	return ((a % b) ? gcd(b, a % b) : b);
}

template<typename T>
void swap(T& x, T& y) noexcept
{
	T temp = x;
	x = y;
	y = temp;
}

template<typename T>
T clamp(T x, T min, T max)
{
	if (x < min)
		return min;
	if (x > max)
		return max;
	return x;
}