/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

#include <atomic>

namespace _ys_ {

class Semaphore
{
	std::atomic<int> _count = 0;

public:
	Semaphore() = default;
	Semaphore(Semaphore const&) = delete;
	Semaphore& operator=(Semaphore const&) = delete;

	inline bool TryWait(int n = 1);
	inline void Wait(int n = 1);
	inline void Post(int n = 1);
};

bool Semaphore::TryWait(int n)
{
	int c = _count.load(std::memory_order_relaxed);
	return c >= n && _count.compare_exchange_weak(c, c - n, std::memory_order_acquire);
}

void Semaphore::Wait(int n)
{
	while (!TryWait(n))
		;
}

void Semaphore::Post(int n)
{
	_count.fetch_add(n, std::memory_order_release);
}

} // namespace _ys_
