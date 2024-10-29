//
// Created by Orgest on 6/18/2024.
//

#pragma once

#include <cstdint>

// Unsigned integer types
using u8  = uint8_t;   // 8-bit unsigned integer
using u16 = uint16_t;  // 16-bit unsigned integer
using u32 = uint32_t;  // 32-bit unsigned integer
using u64 = uint64_t;  // 64-bit unsigned integer

constexpr u32 INVALID_ID = 0xFFFFFFFF;  // Special constant for invalid IDs

// Signed integer types
using i8  = int8_t;    // 8-bit signed integer
using i16 = int16_t;   // 16-bit signed integer
using i32 = int32_t;   // 32-bit signed integer
using i64 = int64_t;   // 64-bit signed integer

// Floating point types
using f32 = float;     // 32-bit floating point
using f64 = double;    // 64-bit floating point