#pragma once
#ifndef STDAFX_H_INCLUDED
#define STDAFX_H_INCLUDED

#define NOMINMAX
#define _USE_MATH_DEFINES

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>

#include <Windows.h>

using c8 = char;
using c16 = char16_t;
using c32 = char32_t;

using b8 = bool;

using s8 = int8_t;
using u8 = uint8_t;
using s16 = int16_t;
using u16 = uint16_t;
using s32 = int32_t;
using u32 = uint32_t;
using s64 = int64_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

#define F_PI static_cast<f32>(M_PI)

#endif // STDAFX_H_INCLUDED
