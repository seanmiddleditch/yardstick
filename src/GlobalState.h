/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

#include <yardstick/yardstick.h>

#include "Atomics.h"
#include "Spinlock.h"
#include "Event.h"
#include "ConcurrentCircularBuffer.h"
#include "WebsocketSink.h"

#include <cstring>
#include <thread>

namespace _ys_ {

class ThreadState;

class GlobalState
{
	Event _signal;
	AlignedAtomic<bool> _active;

	Spinlock _stateLock;
	std::thread _backgroundThread;
	ysAllocator _allocator;

	Spinlock _threadsLock;
	ThreadState* _threads = nullptr;

	WebsocketSink _websocketSink;

	void ThreadMain();
	void ProcessThread(ThreadState* thread);
	void WriteThreadSink(std::thread::id thread, void const* bytes, std::uint32_t len);

public:
	GlobalState() : _active(false) {}
	GlobalState(GlobalState const&) = delete;
	GlobalState& operator=(GlobalState const&) = delete;

	inline static GlobalState& instance();

	ysResult Initialize(ysAllocator allocator);
	bool IsActive() const;
	ysResult Shutdown();

	ysResult ListenWebsocket(unsigned short port);

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
