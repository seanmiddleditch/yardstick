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

void ThreadState::post_write(void const* buffer, std::uint32_t len)
{
	if (!_buffer.TryWrite(buffer, len))
	{
		GlobalState::instance().PostThreadBuffer();
		_buffer.Write(buffer, len);
	}
}
