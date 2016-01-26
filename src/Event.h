/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace _ys_ {

class Event
{
	HANDLE _handle = nullptr;

public:
	inline Event();
	inline ~Event();

	Event(Event const&) = delete;
	Event& operator=(Event const&) = delete;

	inline void Wait(std::uint32_t microseconds);
	inline void Signal();
};

Event::Event()
{
	_handle = CreateEventW(nullptr, TRUE, FALSE, nullptr);
}

Event::~Event()
{
	CloseHandle(_handle);
}

void Event::Wait(std::uint32_t microseconds)
{
	WaitForSingleObject(_handle, static_cast<DWORD>(microseconds));
}

void Event::Signal()
{
	PulseEvent(_handle);
}

} // namespace _ys_

#else // defined(_WIN32)
#	error "Unsupported platform"
#endif
