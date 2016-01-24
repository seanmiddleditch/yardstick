/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

#include <atomic>
#include <type_traits>

namespace _ys_ {

class ConcurrentCircularBuffer
{
	std::uint8_t _buffer[1024];
	std::atomic_uint32_t _head = 0;
	std::atomic_uint32_t _tail = 0;

public:
	ConcurrentCircularBuffer() = default;
	ConcurrentCircularBuffer(ConcurrentCircularBuffer const&) = delete;
	ConcurrentCircularBuffer& operator=(ConcurrentCircularBuffer const&) = delete;

	bool TryWrite(void const* data, std::uint32_t size);
	inline void Write(void const* data, std::uint32_t size);
	inline int Read(void* out, std::uint32_t max);
};

bool ConcurrentCircularBuffer::TryWrite(void const* data, std::uint32_t size)
{
	std::uint32_t const tail = _tail.load(std::memory_order_acquire);
	std::uint32_t const head = _head.load(std::memory_order_acquire);
	std::uint32_t const available = sizeof(_buffer) - ((tail - head) & (sizeof(_buffer) - 1));

	if (available >= size)
	{
		// calculate the head and tail indices in the buffer from the monotonic values
		std::uint32_t const tailIndex = tail & (sizeof(_buffer) - 1);
		std::uint32_t const headIndex = head & (sizeof(_buffer) - 1);

		std::uint32_t remaining = size;

		// write to tail-end of buffer
		std::uint32_t const tailCount = (sizeof(_buffer) - tailIndex) < remaining ? (sizeof(_buffer) - tailIndex) : remaining;
		std::memcpy(_buffer + tailIndex, data, tailCount);
		remaining -= tailCount;

		// write to head-end of buffer
		std::uint32_t const headCount = (sizeof(_buffer) - headIndex) < remaining ? (sizeof(_buffer) - headIndex) : remaining;
		std::memcpy(_buffer, static_cast<char const*>(data) + tailCount, headCount);

		// update the tail for the amount of bytes written
		_tail.store(tail + size, std::memory_order_release);

		return true;
	}
	else
	{
		return false;
	}
}

void ConcurrentCircularBuffer::Write(void const* data, std::uint32_t size)
{
	while (!TryWrite(data, size))
		;
}

int ConcurrentCircularBuffer::Read(void* out, std::uint32_t size)
{
	std::uint32_t tail = _tail.load(std::memory_order_acquire);
	std::uint32_t head = _head.load(std::memory_order_acquire);
	std::uint32_t available = (tail - head) & (sizeof(_buffer) - 1);

	// calculate the head and tail indices in the buffer from the monotonic values
	std::uint32_t const tailIndex = tail & (sizeof(_buffer) - 1);
	std::uint32_t const headIndex = head & (sizeof(_buffer) - 1);

	// determine how many bytes we will read
	std::uint32_t const amount = size < available ? size : available;

	std::uint32_t remaining = amount;

	std::uint32_t const tailCount = sizeof(_buffer) - headIndex < remaining ? sizeof(_buffer) - headIndex : remaining;
	std::memcpy(out, _buffer + headIndex, tailCount);
	remaining -= tailCount;

	std::uint32_t const headCount = headIndex < remaining ? headIndex : remaining;
	std::memcpy(static_cast<char*>(out) + tailCount, _buffer, headCount);

	// update the tail for the amount of bytes read
	_tail.store(head + amount, std::memory_order_release);
}

} // namespace _ys_