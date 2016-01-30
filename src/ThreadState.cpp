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

void ThreadState::Write(void const* buffer, std::uint32_t len)
{
	GlobalState& gs = GlobalState::instance();

	if (gs.IsActive() && !_buffer.TryWrite(buffer, len))
	{
		gs.PostThreadBuffer();
		while (gs.IsActive() && !_buffer.TryWrite(buffer, len))
			; // keep trying to submit until we succeed or we detect that Yardstick has shutdown
	}
}
