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

void ThreadState::Enque(EventData const& ev)
{
	GlobalState& gs = GlobalState::instance();
	
	if (gs.IsActive() && !_queue.TryEnque(ev))
	{
		gs.SignalPost();
		while (gs.IsActive() && !_queue.TryEnque(ev))
			; // keep trying to submit until we succeed or we detect that Yardstick has shutdown
	}
}

bool ThreadState::Deque(EventData& out_ev)
{
	return _queue.TryDeque(out_ev);
}

