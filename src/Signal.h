/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace _ys_ {

class Signal
{
	HANDLE _handle = nullptr;

public:
	inline Signal();
	inline ~Signal();

	Signal(Signal const&) = delete;
	Signal& operator=(Signal const&) = delete;

	inline void Wait(std::uint32_t microseconds);
	inline void Post();
};

Signal::Signal()
{
	_handle = CreateEventW(nullptr, TRUE, FALSE, nullptr);
}

Signal::~Signal()
{
	CloseHandle(_handle);
}

void Signal::Wait(std::uint32_t microseconds)
{
	WaitForSingleObject(_handle, static_cast<DWORD>(microseconds));
}

void Signal::Post()
{
	PulseEvent(_handle);
}

} // namespace _ys_

#else // defined(_WIN32)

#include <mutex>
#include <condition_variable>
#include <chrono>

namespace _ys_ {

class Signal
{
	std::mutex _mutex;
	std::condition_variable _cond;
	std::atomic<int> _ready;

public:
  Signal() : _ready(0) {}
	~Signal() = default;

	Signal(Signal const&) = delete;
	Signal& operator=(Signal const&) = delete;

	inline void Wait(std::uint32_t microseconds);
	inline void Post();
};

void Signal::Wait(std::uint32_t microseconds)
{
	std::unique_lock<std::mutex> guard(_mutex);
	_cond.wait_for(guard, std::chrono::microseconds(microseconds), [this](){ return _ready.load(); });
	_ready = false;
}

void Signal::Post()
{
	_ready = 1;
	_cond.notify_all();
}

} // namespace _ys_

#endif // defined(_WIN32)
