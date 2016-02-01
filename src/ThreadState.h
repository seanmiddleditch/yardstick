/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

#include <yardstick/yardstick.h>

#include "ConcurrentQueue.h"

#include <thread>

namespace _ys_ {

class ThreadState
{
	ConcurrentQueue<ysEvent, 512> _queue;
	std::thread::id _thread;

	// managed by GlobalState _only_!!!
	ThreadState* _prev = nullptr;
	ThreadState* _next = nullptr;
	friend class GlobalState;

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

	void Enque(ysEvent const& ev);

	bool Deque(ysEvent& out_ev);
};

} // namespace _ys_
