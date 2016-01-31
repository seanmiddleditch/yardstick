/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

#include "yardstick/yardstick.h"

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace _ys_ {

static inline YS_INLINE ysTime ReadClock()
{
	LARGE_INTEGER tmp;
	QueryPerformanceCounter(&tmp);
	return tmp.QuadPart;
}

/// Default clock frequency reader.
/// @internal
static inline ysTime GetClockFrequency()
{
	LARGE_INTEGER tmp;
	QueryPerformanceFrequency(&tmp);
	return tmp.QuadPart;
}

} // namespace _ys_

#else // _WIN32

#include <chrono>

namespace _ys_ {

static inline YS_INLINE ysTime ReadClock()
{
	auto const now = std::chrono::high_resolution_clock::now();
	auto const time = now.time_since_epoch();
	auto const ns = std::chrono::duration_cast<std::chrono::nanoseconds>(time);
	return ns.count();
}

static inline ysTime GetClockFrequency()
{
	return std::nano::den;
}

} // namespace _ys_

#endif
