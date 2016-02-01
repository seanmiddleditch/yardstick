/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#include "ThreadState.h"
#include "GlobalState.h"

using namespace _ys_;

ThreadState::ThreadState() : _thread(std::this_thread::get_id())
{
	GlobalState::instance().RegisterThread(this);
}

ThreadState::~ThreadState()
{
	GlobalState::instance().DeregisterThread(this);
}

void ThreadState::Enque(ysEvent const& ev)
{
	GlobalState& gs = GlobalState::instance();
	
	if (gs.IsActive() && !_queue.TryEnque(ev))
	{
		gs.PostThreadBuffer();
		while (gs.IsActive() && !_queue.TryEnque(ev))
			; // keep trying to submit until we succeed or we detect that Yardstick has shutdown
	}
}

bool ThreadState::Deque(ysEvent& out_ev)
{
	return _queue.TryDeque(out_ev);
}

