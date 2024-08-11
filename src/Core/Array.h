//
// Created by Orgest on 7/30/2024.
//

#pragma once
#include <cassert>
#include <cstring>

#include "PrimTypes.h"


template <typename T, u32 N>  requires (N > 0)
class SafeArray
{
protected:
	T a[N];
public:
	constexpr SafeArray()
	{
		memset(a, 0, sizeof(a));
	}

	constexpr u32 size() const
	{
		return sizeof(a) / sizeof(a[0]);
	}


	constexpr T* begin()
	{
		return a + size();
	}

	constexpr T* data()
	{
		return a;
	}

	constexpr T* data() const
	{
		return a;
	}

	constexpr T* end()
	{
		return a + N;
	}

	constexpr void fill(const T& val) noexcept
	{
		for (u32 i = 0; i < N; i++)
		{
			a[i] = val;
		}
	}

	constexpr T& operator[] (u32 i)
	{
		assert(i < N && "Index out of bounds");
		return a[i];
	}

	const T& operator[](u32 i) const
	{
		assert(i < N && "Index out of bounds");
		return a[i];
	}

	// Conversion operator to void*
	explicit operator void*()
	{
		return static_cast<void*>(a);
	}

	// Conversion operator to const void*
	explicit operator const void*() const
	{
		return static_cast<const void*>(a);
	}
};