/* Yardstick
 * Copyright (c) 2014-1016 Sean Middleditch
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 * associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "GlobalState.h"
#include "Algorithm.h"
#include "ThreadState.h"
#include "Clock.h"

using namespace _ys_;

ysResult GlobalState::Initialize(ysAllocator alloc)
{
	if (_active.load(std::memory_order_acquire))
		return ysResult::AlreadyInitialized;

	if (alloc == nullptr)
		return ysResult::InvalidParameter;

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

ysResult GlobalState::Shutdown()
{
	LockGuard guard(_stateLock);

	if (!_active.load(std::memory_order_acquire))
		return ysResult::Uninitialized;

	// do this first, so other systems know to stop trying to update things
	_active.store(false, std::memory_order_release);

	// wait for background thread to complete
	if (_backgroundThread.joinable())
	{
		_signal.Post();
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

	return _websocketSink.Listen(port, _allocator);
}

void GlobalState::ThreadMain()
{
	while (_active.load(std::memory_order_seq_cst))
	{
		_signal.Wait(100);

		FlushThreads();

		_websocketSink.Flush();
		_websocketSink.Update();
	}
}

ysResult GlobalState::ProcessThread(ThreadState* thread)
{
	EventData ev;
	int count = 512;
	while (--count && thread->Deque(ev))
		YS_TRY(WriteEvent(ev));
	return ysResult::Success;
}

ysResult GlobalState::FlushThreads()
{
	LockGuard guard(_threadsLock);
	for (ThreadState* thread = _threads; thread != nullptr; thread = thread->_next)
		YS_TRY(ProcessThread(thread));
	return ysResult::Success;
}

ysResult GlobalState::WriteEvent(EventData const& ev)
{
	return _websocketSink.WriteEvent(ev);
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
