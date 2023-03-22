#pragma once

#include <cstdint>

namespace hakkou {
// TODO: maybe fast or smallest?
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using f32 = float;  // get the correst std::
using f64 = double;

// TODO: char types?

// Define STATIC_ASSERT as in KOHI project
#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif

// Validate size of defined types
STATIC_ASSERT(sizeof(u8) == 1, "Type `u8` expected to be 1 byte.");
STATIC_ASSERT(sizeof(u16) == 2, "Type `u16` expected to be 2 bytes.");
STATIC_ASSERT(sizeof(u32) == 4, "Type `u32` expected to be 4 bytes.");
STATIC_ASSERT(sizeof(u64) == 8, "Type `u64` expected to be 8 bytes.");

STATIC_ASSERT(sizeof(i8) == 1, "Type `i8` expected to be 1 byte.");
STATIC_ASSERT(sizeof(i16) == 2, "Type `i16` expected to be 2 bytes.");
STATIC_ASSERT(sizeof(i32) == 4, "Type `i32` expected to be 4 bytes.");
STATIC_ASSERT(sizeof(i64) == 8, "Type `i64` expected to be 8 bytes.");

STATIC_ASSERT(sizeof(f32) == 4, "Type `f32` expected to be 4 bytes.");
STATIC_ASSERT(sizeof(f64) == 8, "Type `f64` expected to be 8 bytes.");
}
