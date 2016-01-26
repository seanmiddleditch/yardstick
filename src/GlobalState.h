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
	struct Location
	{
		char const* file;
		char const* func;
		int line;

		bool operator==(Location const& rhs) const
		{
			return line == rhs.line &&
				0 == std::strcmp(file, rhs.file) &&
				0 == std::strcmp(func, rhs.func);
		}
	};

	Event _signal;
	Spinlock _globalStateLock;
	AlignedAtomic<bool> _active;
	std::thread _backgroundThread;
	Allocator<void> _allocator;
	Vector<Location> _locations;
	Vector<char const*> _counters;
	Vector<char const*> _regions;

	Spinlock _threadStateLock;
	Vector<ThreadState*> _threads;

	void ThreadMain();
	void ProcessThread(ThreadState* thread);
	void FlushNetBuffer();

public:
	GlobalState() : _active(false), _locations(_allocator), _counters(_allocator), _regions(_allocator), _threads(_allocator) {}
	GlobalState(GlobalState const&) = delete;
	GlobalState& operator=(GlobalState const&) = delete;

	inline static GlobalState& instance();

	bool Initialize(ysAllocator allocator);
	void Shutdown();

	ysLocationId RegisterLocation(char const* file, int line, char const* function);
	ysCounterId RegisterCounter(char const* name);
	ysRegionId RegisterRegion(char const* name);

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
