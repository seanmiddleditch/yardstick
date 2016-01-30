/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#include "GlobalState.h"
#include "Algorithm.h"
#include "ThreadState.h"

using namespace _ys_;

ysResult GlobalState::Initialize(ysAllocator alloc)
{
	if (_active.load(std::memory_order_acquire))
		return ysResult::AlreadyInitialized;

	LockGuard guard(_stateLock);
	_allocator = alloc;

	// activate the system if not already.
	// the active boolean must be set before the background thread starts to ensure that it
	// doesn't early-exit.
	_active.store(true, std::memory_order_seq_cst);
	if (!_backgroundThread.joinable())
		_backgroundThread = std::thread(std::bind(&GlobalState::ThreadMain, this));

	return ysResult::Success;
}

bool GlobalState::IsActive() const
{
	return _active.load(std::memory_order_acquire);
}

ysResult GlobalState::Shutdown()
{
	LockGuard guard(_stateLock);

	if (!_active.load(std::memory_order_acquire))
		return ysResult::Uninitialized;

	// wait for background thread to complete
	_active.store(false, std::memory_order_release);
	if (_backgroundThread.joinable())
	{
		_signal.Signal();
		_backgroundThread.join();
	}

	_allocator = nullptr;

	return ysResult::Success;
}

ysResult GlobalState::ListenWebsocket(unsigned short port)
{
	LockGuard guard(_stateLock);

	if (!_active.load(std::memory_order_acquire))
		return ysResult::Uninitialized;

	return _websocketSink.Listen(port);
}

void GlobalState::ThreadMain()
{
	while (_active.load(std::memory_order_seq_cst))
	{
		_signal.Wait(100);

		LockGuard guard(_threadsLock);
		for (ThreadState* thread = _threads; thread != nullptr; thread = thread->_next)
			ProcessThread(thread);
	}
}

void GlobalState::ProcessThread(ThreadState* thread)
{
	char tmp[1024];
	int const len = thread->Read(tmp, sizeof(tmp));
	if (len != 0)
		WriteThreadSink(thread->GetThreadId(), tmp, len);
}

void GlobalState::WriteThreadSink(std::thread::id thread, void const* bytes, std::uint32_t len)
{

}

void GlobalState::RegisterThread(ThreadState* thread)
{
	LockGuard guard(_threadsLock);

	thread->_next = _threads;
	if (_threads != nullptr)
		_threads->_next = thread;
	_threads = thread;
}

void GlobalState::DeregisterThread(ThreadState* thread)
{
	LockGuard guard(_threadsLock);

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
