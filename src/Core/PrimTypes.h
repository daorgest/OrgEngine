//
// Created by Orgest on 6/18/2024.
//

#pragma once

#include <cstdint>

// Unsigned Integers
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

constexpr u8  u8_invalid_id  { 0xFF };
constexpr u16 u16_invalid_id { 0xFFFF };
constexpr u32 u32_invalid_id { 0xFFFFFFFF };
constexpr u64 u64_invalid_id { 0xFFFFFFFFFFFFFFFF };

// Signed Integers
using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

// Float
using f32 = float;
