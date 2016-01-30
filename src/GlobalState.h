/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

#include "Atomics.h"
#include "Spinlock.h"
#include "Allocator.h"
#include "Event.h"
#include "ConcurrentCircularBuffer.h"

#include <cstring>
#include <thread>

namespace _ys_ {

class ThreadState;

class GlobalState
{
	using Location = std::pair<char const*, int>;

	Event _signal;
	Spinlock _globalStateLock;
	AlignedAtomic<bool> _active;
	std::thread _backgroundThread;
	Allocator<void> _allocator;

	Spinlock _threadStateLock;
	Vector<ThreadState*> _threads;

	void ThreadMain();
	void ProcessThread(ThreadState* thread);
	void FlushNetBuffer();

public:
	GlobalState() : _active(false), _threads(_allocator) {}
	GlobalState(GlobalState const&) = delete;
	GlobalState& operator=(GlobalState const&) = delete;

	inline static GlobalState& instance();

	bool Initialize(ysAllocator allocator);
	void Shutdown();

	void RegisterThread(ThreadState* thread);
	void DeregisterThread(ThreadState* thread);

	void PostThreadBuffer();
};

GlobalState& GlobalState::instance()
{
	static GlobalState state;
	return state;
}

} // namespace _ys_
