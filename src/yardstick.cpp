// Copyright (C) 2014-2016 Sean Middleditch, all rights reserverd.

#include "GlobalState.h"
#include "ThreadState.h"
#include "Clock.h"

using namespace _ys_;

ysResult YS_API _ys_::initialize(ysAllocator allocator)
{
	return GlobalState::instance().Initialize(allocator);
}

ysResult YS_API _ys_::shutdown()
{
	return GlobalState::instance().Shutdown();
}

YS_API ysResult YS_CALL _ys_::emit_counter(ysTime when, double value, char const* name, char const* file, int line)
{
	ysEvent ev;
	ev.type = ysEvent::TypeCounter;
	ev.counter.line = line;
	ev.counter.name = ysStringHandle(name);
	ev.counter.file = ysStringHandle(file);
	ev.counter.when = when;
	ev.counter.value = value;
	return emit_event(ev);
}

YS_API ysResult YS_CALL _ys_::emit_region(ysTime startTime, ysTime endTime, char const* name, char const* file, int line)
{
	ysEvent ev;
	ev.type = ysEvent::TypeRegion;
	ev.region.line = line;
	ev.region.name = ysStringHandle(name);
	ev.region.file = ysStringHandle(name);
	ev.region.begin = startTime;
	ev.region.end = endTime;
	return emit_event(ev);
}

YS_API ysTime YS_CALL _ys_::read_clock()
{
	return ReadClock();
}

YS_API ysResult YS_CALL _ys_::tick()
{
	ysEvent ev;
	ev.type = ysEvent::TypeTick;
	ev.tick.when = ReadClock();
	return emit_event(ev);
}

YS_API ysResult YS_CALL _ys_::listen(unsigned short port)
{
	return GlobalState::instance().ListenWebsocket(port);
}
