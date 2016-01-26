/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

#include "Atomics.h"
#include <type_traits>
#include <cstdint>
#include <cstring>

namespace _ys_ {

template <std::uint32_t S = 1024>
class ConcurrentCircularBuffer
{
	static constexpr std::uint32_t kBufferSize = S;
	static constexpr std::uint32_t kBufferMask = kBufferSize - 1;

	static_assert((kBufferSize & kBufferMask) == 0, "ConcurrentCircularBuffer size must be a power of 2");

	std::uint8_t _buffer[kBufferSize];
	AlignedAtomic<std::uint32_t> _head;
	AlignedAtomic<std::uint32_t> _tail;

public:
	ConcurrentCircularBuffer() : _head(0), _tail(0) {}
	ConcurrentCircularBuffer(ConcurrentCircularBuffer const&) = delete;
	ConcurrentCircularBuffer& operator=(ConcurrentCircularBuffer const&) = delete;

	bool TryWrite(void const* data, std::uint32_t size);
	inline void Write(void const* data, std::uint32_t size);
	int Read(void* out, std::uint32_t max);
};

template <std::uint32_t S>
bool ConcurrentCircularBuffer<S>::TryWrite(void const* data, std::uint32_t size)
{
	std::uint32_t const tail = _tail.load(std::memory_order_acquire);
	std::uint32_t const head = _head.load(std::memory_order_acquire);
	std::uint32_t const available = sizeof(_buffer) - ((tail - head) & kBufferMask);

	if (available >= size)
	{
		// calculate the head and tail indices in the buffer from the monotonic values
		std::uint32_t const tailIndex = tail & kBufferMask;
		std::uint32_t const headIndex = head & kBufferMask;

		std::uint32_t remaining = size;

		// write to up to end of buffer (pre-wrap)
		std::uint32_t const count = (sizeof(_buffer) - tailIndex) < remaining ? (sizeof(_buffer) - tailIndex) : remaining;
		std::memcpy(_buffer + tailIndex, data, count);
		remaining -= count;

		// write to head-end of buffer (post-wrap)
		std::uint32_t const wrapped = headIndex < remaining ? headIndex : remaining;
		std::memcpy(_buffer, static_cast<char const*>(data) + count, wrapped);

		// update the tail for the amount of bytes written
		_tail.store(tail + size, std::memory_order_release);

		return true;
	}
	else
	{
		return false;
	}
}

template <std::uint32_t S>
void ConcurrentCircularBuffer<S>::Write(void const* data, std::uint32_t size)
{
	while (!TryWrite(data, size))
		;
}

template <std::uint32_t S>
int ConcurrentCircularBuffer<S>::Read(void* out, std::uint32_t size)
{
	std::uint32_t tail = _tail.load(std::memory_order_acquire);
	std::uint32_t head = _head.load(std::memory_order_acquire);
	std::uint32_t available = (tail - head) & kBufferMask;

	// calculate the head and tail indices in the buffer from the monotonic values
	std::uint32_t const tailIndex = tail & kBufferMask;
	std::uint32_t const headIndex = head & kBufferMask;

	// determine how many bytes we will read
	std::uint32_t const amount = size < available ? size : available;

	std::uint32_t remaining = amount;

	// read all bytes from the current head (pre-wrap)
	std::uint32_t const count = sizeof(_buffer) - headIndex < remaining ? sizeof(_buffer) - headIndex : remaining;
	std::memcpy(out, _buffer + headIndex, count);
	remaining -= count;

	// read all bytes form the start of the buffer (post-wrap)
	std::uint32_t const wrapped = headIndex < remaining ? headIndex : remaining;
	std::memcpy(static_cast<char*>(out) + count, _buffer, wrapped);

	// update the tail for the amount of bytes read
	_head.store(head + amount, std::memory_order_release);

	return amount;
}

} // namespace _ys_
