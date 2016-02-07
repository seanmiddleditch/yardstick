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

#pragma once

#include <yardstick/yardstick.h>

#include "Atomics.h"
#include "Spinlock.h"
#include "Signal.h"
#include "WebsocketSink.h"

#include <cstring>
#include <thread>

namespace _ys_ {

class ThreadState;

class GlobalState
{
	Signal _signal;
	AlignedAtomic<bool> _active;

	Spinlock _stateLock;
	std::thread _backgroundThread;
	ysAllocator _allocator;

	Spinlock _threadsLock;
	ThreadState* _threads = nullptr;

	WebsocketSink _websocketSink;

	void ThreadMain();
	ysResult ProcessThread(ThreadState* thread);
	ysResult FlushThreads();
	ysResult WriteEvent(EventData const& ev);

public:
	GlobalState() : _active(false) {}
	GlobalState(GlobalState const&) = delete;
	GlobalState& operator=(GlobalState const&) = delete;

	inline static GlobalState& instance();

	ysResult Initialize(ysAllocator allocator);
	bool IsActive() const { return _active.load(std::memory_order_relaxed); }
	ysResult Shutdown();

	ysResult ListenWebsocket(unsigned short port);

	void RegisterThread(ThreadState* thread);
	void DeregisterThread(ThreadState* thread);

	void SignalPost() { _signal.Post(); }
};

GlobalState& GlobalState::instance()
{
	static GlobalState state;
	return state;
}

} // namespace _ys_
