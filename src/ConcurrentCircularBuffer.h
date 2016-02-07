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

	AlignedAtomic<std::uint32_t> _write;
	AlignedAtomic<std::uint32_t> _read;
	std::uint8_t _buffer[kBufferSize];

public:
	ConcurrentCircularBuffer() : _write(0), _read(0) {}
	ConcurrentCircularBuffer(ConcurrentCircularBuffer const&) = delete;
	ConcurrentCircularBuffer& operator=(ConcurrentCircularBuffer const&) = delete;

	bool TryWrite(void const* data, std::uint32_t size);
	int Read(void* out, std::uint32_t max);
};

template <std::uint32_t S>
bool ConcurrentCircularBuffer<S>::TryWrite(void const* data, std::uint32_t size)
{
	std::uint32_t const tail = _read.load(std::memory_order_acquire);
	std::uint32_t const head = _write.load(std::memory_order_acquire);
	std::uint32_t const used = (head - tail) & kBufferMask;
	std::uint32_t const available = sizeof(_buffer) - used;

	// use < and not <= because otherwise we treat tail==head as both full and empty
	if (size < available)
	{
		std::uint32_t remaining = size;

		// write to up to end of buffer (pre-wrap).
		// the available space is all of the remaining buffer, or the remaining bytes.
		// if the tail is after the head, then remaining < (bufsize - head) and we'll use remaining.
		// if the tail is before the head, then (bufsize - head) < remaining, since the remaining is (bufsize - head) + tail.
		std::uint32_t const space = sizeof(_buffer) - head;
		std::uint32_t const count = size < space ? size : space;
		std::memcpy(_buffer + head, data, count);
		remaining -= count;

		// write to head-end of buffer (post-wrap).
		// this is only necessary if the tail is before the head, meaning that we had to wrap.
		// we would have only written up to (bufsize - head) bytes prior.
		// we also know that we'd only be here if the total amount we're writing fits so remaining < tail.
		std::memcpy(_buffer, static_cast<char const*>(data) + count, remaining);

		// update the write head for the amount of bytes written
		_write.store((head + size) & kBufferMask, std::memory_order_release);

		return true;
	}
	else
	{
		return false;
	}
}

template <std::uint32_t S>
int ConcurrentCircularBuffer<S>::Read(void* out, std::uint32_t size)
{
	std::uint32_t tail = _write.load(std::memory_order_acquire);
	std::uint32_t head = _read.load(std::memory_order_acquire);
	std::uint32_t available = (tail - head) & kBufferMask;

	if (available > 0)
	{
		// determine how many bytes we will read
		std::uint32_t const amount = size < available ? size : available;

		std::uint32_t remaining = amount;

		// read all bytes from the current head (pre-wrap).
		// if the head < tail, then the total amount to read can be no more than head - tail.
		// if the head > tail, then we can only read up to bufsize - head.
		std::uint32_t const space = sizeof(_buffer) - head;
		std::uint32_t const count = space < amount ? space : amount;
		std::memcpy(out, _buffer + head, count);
		remaining -= count;

		// read all bytes form the start of the buffer (post-wrap).
		// if tail < head, then we will have to wrap to read all available bytes. read any
		// remaining bytes from the head of the buffer.
		std::memcpy(static_cast<char*>(out) + count, _buffer, remaining);

		// update the tail for the amount of bytes read
		_read.store((head + amount) & kBufferMask, std::memory_order_release);

		return amount;
	}
	else
	{
		return 0;
	}
}

} // namespace _ys_
