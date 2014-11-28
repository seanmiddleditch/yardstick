// Copyright (C) 2014 Sean Middleditch, all rights reserverd.

#include "yardstick.h"

#if defined(_WIN32)
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#endif

static ys_clock_t default_read_clock_ticks(void)
{
#if defined(_WIN32)
	LARGE_INTEGER tmp;
	return QueryPerformanceCounter(&tmp);
	return tmp.QuadPart;
#else
#	error "Platform unsupported"
#endif
}

static ys_clock_t default_read_clock_frequency(void)
{
#if defined(_WIN32)
	LARGE_INTEGER tmp;
	return QueryPerformanceFrequency(&tmp);
	return tmp.QuadPart;
#else
#	error "Platform unsupported"
#endif
}

static ys_read_clock_ticks_cb read_clock_ticks_cb = &default_read_clock_ticks;
static ys_read_clock_ticks_cb read_clock_frequency_cb = &default_read_clock_frequency;

void ys_register_clock_callbacks(ys_read_clock_ticks_cb ticks, ys_read_clock_frequency_cb frequency)
{
	read_clock_ticks_cb = ticks;
	read_clock_frequency_cb = frequency;
}

ys_clock_t ys_read_clock_ticks(void)
{
	return read_clock_ticks_cb();
}

ys_clock_t ys_read_clock_frequency(void)
{
	return read_clock_frequency_cb();
}