// Copyright (C) 2014 Sean Middleditch, all rights reserverd.

#include "yardstick.h"
#include "ys_private.h"

ys_clock_t _ys_default_read_clock_ticks(void)
{
#if defined(_WIN32)
	LARGE_INTEGER tmp;
	QueryPerformanceCounter(&tmp);
	return tmp.QuadPart;
#else
#	error "Platform unsupported"
#endif
}

ys_clock_t _ys_default_read_clock_frequency(void)
{
#if defined(_WIN32)
	LARGE_INTEGER tmp;
	QueryPerformanceFrequency(&tmp);
	return tmp.QuadPart;
#else
#	error "Platform unsupported"
#endif
}

ys_clock_t _ys_read_clock_ticks(void)
{
	return _ys_context.read_clock_ticks();
}

ys_clock_t _ys_read_clock_frequency(void)
{
	return _ys_context.read_clock_frequency();
}