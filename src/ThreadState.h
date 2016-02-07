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

#include "ConcurrentQueue.h"
#include "Protocol.h"

#include <thread>

namespace _ys_ {

class ThreadState
{
	ConcurrentQueue<EventData, 512> _queue;
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

	void Enque(EventData const& ev);

	bool Deque(EventData& out_ev);
};

} // namespace _ys_
