/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

#include "ConcurrentCircularBuffer.h"

#include <thread>

namespace _ys_ {

class ThreadState
{
	ConcurrentCircularBuffer<> _buffer;
	std::thread::id _thread;

	// managed by GlobalState _only_!!!
	ThreadState* _prev = nullptr;
	ThreadState* _next = nullptr;
	friend class GlobalState;

	template <typename T>
	static std::uint32_t calculate_size(T const& value) { return sizeof(value); }

	template <typename T, typename... Ts>
	static std::uint32_t calculate_size(T const& value, Ts const&... ts) { return calculate_size(value) + calculate_size(ts...); }

	template <typename T>
	static void write_value(void* out, T const& value) { std::memcpy(out, &value, sizeof(value)); }

	template <typename T, typename... Ts>
	static void write_value(void* out, T const& value, Ts const&... ts)
	{
		write_value(out, value);
		write_value(static_cast<char*>(out) + sizeof(value), ts...);
	}

public:
	ThreadState();
	~ThreadState();

	ThreadState(ThreadState const&) = delete;
	ThreadState& operator=(ThreadState const&) = delete;

	static ThreadState& thread_instance()
	{
		thread_local ThreadState state;
		return state;
	}

	std::thread::id const& GetThreadId() const { return _thread; }

	void Write(void const* bytes, std::uint32_t len);

	std::uint32_t Read(void* out, std::uint32_t max)
	{
		return _buffer.Read(out, max);
	}
};

} // namespace _ys_
