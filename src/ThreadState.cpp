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
