// Copyright (C) 2014-2016 Sean Middleditch, all rights reserverd.

#include "GlobalState.h"
#include "ThreadState.h"
#include "Clock.h"

using namespace _ys_;

ysResult YS_API _ys_::initialize(ysAllocator allocator)
{
	GlobalState::instance().Initialize(allocator);
	return ysResult::Success;
}

ysResult YS_API _ys_::shutdown()
{
	GlobalState::instance().Shutdown();
	return ysResult::Success;
}

YS_API void YS_CALL _ys_::emit_counter(ysTime when, double value, char const* name, char const* file, int line)
{
	ysEvent ev;
	ev.type = ysEvent::TypeCounter;
	ev.counter.line = line;
	ev.counter.name = ysStringHandle(name);
	ev.counter.file = ysStringHandle(file);
	ev.counter.when = when;
	ev.counter.value = value;
	emit_event(ev);
}

YS_API void YS_CALL _ys_::emit_region(ysTime startTime, ysTime endTime, char const* name, char const* file, int line)
{
	ysEvent ev;
	ev.type = ysEvent::TypeRegion;
	ev.region.line = line;
	ev.region.name = ysStringHandle(name);
	ev.region.file = ysStringHandle(name);
	ev.region.begin = startTime;
	ev.region.end = endTime;
	emit_event(ev);
}

YS_API ysTime YS_CALL _ys_::read_clock()
{
	return ReadClock();
}

YS_API ysResult YS_CALL _ys_::emit_tick()
{
	ysEvent ev;
	ev.type = ysEvent::TypeTick;
	ev.tick.when = ReadClock();
	return emit_event(ev);
}
