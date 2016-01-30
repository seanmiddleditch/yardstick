/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#include "GlobalState.h"
#include "Algorithm.h"
#include "ThreadState.h"

using namespace _ys_;

bool GlobalState::Initialize(ysAllocator alloc)
{
	LockGuard guard(_globalStateLock);

	// switch all internal allocations over to the new allocator
	Allocator<void> allocator(alloc);
	if (allocator != _allocator)
	{
		_allocator = allocator;

		LockGuard guard2(_threadStateLock);
	}

	// activate the system if not already
	_active.store(true, std::memory_order_seq_cst);
	if (!_backgroundThread.joinable())
		_backgroundThread = std::thread(std::bind(&GlobalState::ThreadMain, this));
	return true;
}

void GlobalState::Shutdown()
{
	LockGuard guard(_globalStateLock);

	// wait for background thread to complete
	_active.store(false, std::memory_order_seq_cst);
	if (_backgroundThread.joinable())
	{
		_signal.Signal();
		_backgroundThread.join();
	}

	_allocator = Allocator<void>();
}

void GlobalState::ThreadMain()
{
	while (_active.load(std::memory_order_relaxed))
	{
		_signal.Wait(100);

		LockGuard guard(_threadStateLock);
		for (ThreadState* thread = _threads; thread != nullptr; thread = thread->_next)
			ProcessThread(thread);
	}
}

void GlobalState::ProcessThread(ThreadState* thread)
{
	char tmp[1024];
	int const len = thread->Read(tmp, sizeof(tmp));
	if (len != 0)
	{

	}
}

void GlobalState::RegisterThread(ThreadState* thread)
{
	LockGuard guard(_threadStateLock);

	thread->_next = _threads;
	if (_threads != nullptr)
		_threads->_next = thread;
	_threads = thread;
}

void GlobalState::DeregisterThread(ThreadState* thread)
{
	LockGuard guard(_threadStateLock);

	if (thread->_next != nullptr)
		thread->_next->_prev = thread->_prev;
	if (thread->_prev != nullptr)
		thread->_prev->_next = thread->_next;
	if (_threads == thread)
		_threads = _threads->_next;
}

void GlobalState::PostThreadBuffer()
{
	_signal.Signal();
}
