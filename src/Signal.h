/* Yardstick
 * Copyright (c) 2014-1016 Sean Middleditch
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 * associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

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
