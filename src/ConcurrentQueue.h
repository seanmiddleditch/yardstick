/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

// http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue

#pragma once

#include "Atomics.h"
#include <type_traits>

namespace _ys_ {

template <typename T, std::size_t S = 512>
class ConcurrentQueue
{
	static constexpr std::uint32_t kBufferSize = S;
	static constexpr std::uint32_t kBufferMask = kBufferSize - 1;

	static_assert(std::is_pod<T>::value, "ConcurrentQueue can only be used for PODs");
	static_assert((kBufferSize & kBufferMask) == 0, "ConcurrentQueue size must be a power of 2");

	AlignedAtomicU32 _sequence[kBufferSize];
	T _buffer[kBufferSize];
	AlignedAtomicU32 _enque = 0;
	AlignedAtomicU32 _deque = 0;

public:
	inline ConcurrentQueue();
	ConcurrentQueue(ConcurrentQueue const&) = delete;
	ConcurrentQueue& operator=(ConcurrentQueue const&) = delete;

	inline bool TryEnque(T const& value);
	inline void Enque(T const& value);
	inline bool TryDeque(T& out);
};

template <typename T, std::size_t S>
ConcurrentQueue<T, S>::ConcurrentQueue() 
{
	for (std::uint32_t i = 0; i != kBufferSize; ++i)
		_sequence[i].store(i, std::memory_order_relaxed);
}

template <typename T, std::size_t S>
bool ConcurrentQueue<T, S>::TryEnque(T const& value)
{
	std::uint32_t target = _enque.load(std::memory_order_relaxed);
	std::uint32_t id = _sequence[target & kBufferMask].load(std::memory_order_acquire);
	std::int32_t delta = id - target;

	while (!(delta == 0 && _enque.compare_exchange_weak(target, target + 1, std::memory_order_relaxed)))
	{
		if (delta < 0)
			return false;

		target = _enque.load(std::memory_order_relaxed);
		id = _sequence[target & kBufferMask].load(std::memory_order_acquire);
		delta = id - target;
	}

	_buffer[target & kBufferMask] = value;
	_sequence[target & kBufferMask].store(target + 1, std::memory_order_release);
	return true;
}

template <typename T, std::size_t S>
void ConcurrentQueue<T, S>::Enque(T const& value)
{
	while (!TryEnque(value))
		;
}

template <typename T, std::size_t S>
bool ConcurrentQueue<T, S>::TryDeque(T& out)
{
	std::uint32_t target = _enque.load(std::memory_order_relaxed);
	std::uint32_t id = _sequence[target & kBufferMask].load(std::memory_order_acquire);
	std::int32_t delta = id - (target + 1);

	while (!(delta == 0 && _deque.compare_exchange_weak(target, target + 1, std::memory_order_relaxed)))
	{
		if (delta < 0)
			return false;

		target = _deque.load(std::memory_order_relaxed);
		id = _sequence[target & kBufferMask].load(std::memory_order_acquire);
		delta = id - (target + 1);
	}

	out = _buffer[target & kBufferMask];
	_sequence[target & kBufferMask].store(target + kBufferMask + 1, std::memory_order_release);
	return true;
}

} // namespace _ys_