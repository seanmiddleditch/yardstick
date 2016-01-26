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
		_locations = Vector<Location>(_locations.begin(), _locations.end(), _allocator);
		_counters = Vector<char const*>(_counters.begin(), _counters.end(), _allocator);
		_regions = Vector<char const*>(_regions.begin(), _regions.end(), _allocator);

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
	_locations = Vector<Location>(_allocator);
	_counters = Vector<char const*>(_allocator);
	_regions = Vector<char const*>(_allocator);

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

ysLocationId GlobalState::RegisterLocation(char const* file, int line, char const* function)
{
	LockGuard guard(_globalStateLock);

	Location const location{ file, function, line };

	std::size_t const index = FindValue(_locations.data(), _locations.data() + _locations.size(), location);
	auto const id = ysLocationId(index + 1);
	if (index < _locations.size())
		return id;

	_locations.push_back(location);

	return id;
}

ysCounterId GlobalState::RegisterCounter(char const* name)
{
	LockGuard guard(_globalStateLock);

	// this may be a duplicate; return the existing one if so
	std::size_t const index = FindIf(_counters.data(), _counters.data() + _counters.size(), [=](char const* str){ return std::strcmp(str, name) == 0; });
	auto const id = ysCounterId(index + 1);
	if (index < _counters.size())
		return id;

	_counters.push_back(name);

	return id;
}

ysRegionId GlobalState::RegisterRegion(char const* name)
{
	LockGuard guard(_globalStateLock);

	// this may be a duplicate; return the existing one if so
	std::size_t const index = FindIf(_regions.data(), _regions.data() + _regions.size(), [=](char const* str){ return std::strcmp(str, name) == 0; });
	auto const id = ysRegionId(index + 1);
	if (index < _regions.size())
		return id;

	_regions.push_back(name);

	return id;
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
