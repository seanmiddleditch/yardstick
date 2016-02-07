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

	AlignedAtomic<std::uint32_t> _sequence[kBufferSize];
	AlignedAtomic<std::uint32_t> _enque;
	AlignedAtomic<std::uint32_t> _deque;
	T _buffer[kBufferSize];

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
	_enque.store(0, std::memory_order_relaxed);
	_deque.store(0, std::memory_order_relaxed);
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
	std::uint32_t target = _deque.load(std::memory_order_relaxed);
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