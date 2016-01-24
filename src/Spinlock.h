/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

#include <atomic>

namespace _ys_ {

class Spinlock
{
	std::atomic<int> _state = 0;

public:
	Spinlock() = default;
	Spinlock(Spinlock const&) = delete;
	Spinlock& operator=(Spinlock const&) = delete;

	void Lock()
	{
		int expected = 0;
		while (!_state.compare_exchange_weak(expected, 1, std::memory_order_acquire))
			expected = 0;
	}

	void Unlock()
	{
		_state.store(0, std::memory_order_release);
	}
};

class LockGuard
{
	Spinlock& _lock;

public:
	inline LockGuard(Spinlock& lock) : _lock(lock) { _lock.Lock(); }
	inline ~LockGuard() { _lock.Unlock(); }

	LockGuard(LockGuard const&) = delete;
	LockGuard& operator=(LockGuard const&) = delete;

};

} // namespace _ys_
