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
		_threads = Vector<ThreadState*>(_threads.begin(), _threads.end(), _allocator);
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

	LockGuard guard2(_threadStateLock);
	_threads = Vector<ThreadState*>(_allocator);
}

void GlobalState::ThreadMain()
{
	while (_active.load(std::memory_order_relaxed))
	{
		_signal.Wait(100);

		LockGuard guard(_threadStateLock);
		for (ThreadState* thread : _threads)
			ProcessThread(thread);
	}
}

void GlobalState::ProcessThread(ThreadState* thread)
{
	char tmp[1024];
	int const len = thread->Read(tmp, sizeof(tmp));
}

void GlobalState::RegisterThread(ThreadState* thread)
{
	LockGuard guard(_threadStateLock);

	_threads.push_back(thread);
}

void GlobalState::DeregisterThread(ThreadState* thread)
{
	LockGuard guard(_threadStateLock);

	std::size_t const index = FindValue(_threads.data(), _threads.data() + _threads.size(), thread);
	if (index < _threads.size())
		_threads.erase(_threads.begin() + index);
}

void GlobalState::PostThreadBuffer()
{
	_signal.Signal();
}
