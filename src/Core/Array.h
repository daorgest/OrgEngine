//
// Created by Orgest on 7/30/2024.
//

#pragma once
#include <cassert>

#include "PrimTypes.h"


template <typename T, auto N>  requires (N > 0)
class SafeArray
{
protected:
	T a[N];
public:
	constexpr SafeArray() : a {} {}

	[[nodiscard]] static constexpr auto size()
	{
		return N;
	}

	constexpr bool contains(const T& val) const
	{
		for (u32 i = 0; i < N; i++)
		{
			if (a[i] == val)
			{
				return true;
			}
		}
		return false;
	}

	constexpr T* begin() noexcept
	{
		return a;
	}

	constexpr const T* begin() const noexcept
	{
		return a;
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

	constexpr T& front() noexcept
	{
		return a[0];
	}

	constexpr const T& front() const noexcept
	{
		return a[0];
	}

	constexpr T& operator[] (auto i)
	{
		assert(i < N && "Index out of bounds");
		return a[i];
	}

	const T& operator[](auto i) const
	{
		assert(i < N && "Index out of bounds");
		return a[i];
	}

	constexpr void reset() noexcept
	{
		for (u32 i = 0; i < N; i++)
		{
			a[i] = T();
		}
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