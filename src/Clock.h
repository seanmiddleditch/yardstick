/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

#include "yardstick/yardstick.h"

#if defined(_WIN32)

#include <profileapi.h>

namespace _ys_ {

static inline __forceinline ysTime ReadClock()
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
#	error "Unsupported platform"
#endif
