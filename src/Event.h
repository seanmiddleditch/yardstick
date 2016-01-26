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

#include <mutex>
#include <condition_variable>
#include <chrono>

namespace _ys_ {

class Event
{
	std::mutex _mutex;
	std::condition_variable _cond;
	std::atomic<int> _ready;

public:
  Event() : _ready(0) {}
	~Event() = default;

	Event(Event const&) = delete;
	Event& operator=(Event const&) = delete;

	inline void Wait(std::uint32_t microseconds);
	inline void Signal();
};

void Event::Wait(std::uint32_t microseconds)
{
	std::unique_lock<std::mutex> guard(_mutex);
	_cond.wait_for(guard, std::chrono::microseconds(microseconds), [this](){ return _ready.load(); });
	_ready = false;
}

void Event::Signal()
{
	_ready = 1;
	_cond.notify_all();
}

} // namespace _ys_

#endif // defined(_WIN32)
