//
// Created by Orgest on 6/18/2024.
//

#pragma once

// Unsigned Integers
using u8  = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

constexpr u8  u8_invalid_id  { 0xFF };
constexpr u16 u16_invalid_id { 0xFFFF };
constexpr u32 u32_invalid_id { 0xFFFFFFFF };
constexpr u64 u64_invalid_id { 0xFFFFFFFFFFFFFFFF };

// Signed Integers
using i8  = signed char;
using i16 = signed short;
using i32 = signed int;
using i64 = signed long long;

// Float
using f32 = float;
using f64 = double;