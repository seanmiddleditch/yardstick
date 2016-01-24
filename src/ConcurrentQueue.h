/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

#include <atomic>
#include <type_traits>

namespace _ys_ {

template <typename T>
class ConcurrentQueue
{
	static_assert(std::is_pod<T>::value, "ConcurrentQueue can only be used for PODs");

	static constexpr std::uint32_t kBufferSize = 512;
	static constexpr std::uint32_t kBufferMask = kBufferSize - 1;

	std::atomic_uint32_t _sequence[kBufferSize];
	T _buffer[kBufferSize];
	std::atomic_uint32_t _enque = 0;
	std::atomic_uint32_t _deque = 0;

public:
	inline ConcurrentQueue();
	ConcurrentQueue(ConcurrentQueue const&) = delete;
	ConcurrentQueue& operator=(ConcurrentQueue const&) = delete;

	inline bool TryEnque(T const& value);
	inline void Enque(T const& value);
	inline bool TryDeque(T& out);
};

template <typename T>
ConcurrentQueue<T>::ConcurrentQueue() 
{
	for (std::uint32_t i = 0; i != kBufferSize; ++i)
		_sequence[i].store(i, std::memory_order_relaxed);
}

template <typename T>
bool ConcurrentQueue<T>::TryEnque(T const& value)
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

template <typename T>
void ConcurrentQueue<T>::Enque(T const& value)
{
	while (!TryEnque(value))
		;
}

template <typename T>
bool ConcurrentQueue<T>::TryDeque(T& out)
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