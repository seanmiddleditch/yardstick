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

namespace _ys_ {

class Spinlock
{
	AlignedAtomic<int> _state;

public:
	Spinlock() : _state(0) {}
	Spinlock(Spinlock const&) = delete;
	Spinlock& operator=(Spinlock const&) = delete;

	void Lock()
	{
		int expected = 0;
		while (!_state.compare_exchange_weak(expected, 1, std::memory_order_acquire))
			expected = 0;
	}

	void Unlock()
	{
		_state.store(0, std::memory_order_release);
	}
};

class LockGuard
{
	Spinlock& _lock;

public:
	inline LockGuard(Spinlock& lock) : _lock(lock) { _lock.Lock(); }
	inline ~LockGuard() { _lock.Unlock(); }

	LockGuard(LockGuard const&) = delete;
	LockGuard& operator=(LockGuard const&) = delete;

};

} // namespace _ys_
