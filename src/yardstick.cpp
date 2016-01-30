// Copyright (C) 2014-2016 Sean Middleditch, all rights reserverd.

#include "Allocator.h"
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

YS_API ysLocationId YS_CALL _ys_::add_location(char const* fileName, int line)
{
	return GlobalState::instance().RegisterLocation(fileName, line);
}

YS_API ysCounterId YS_CALL _ys_::add_counter(const char* counterName)
{
	return GlobalState::instance().RegisterCounter(counterName);
}

YS_API void YS_CALL _ys_::emit_counter_add(ysCounterId counterId, ysLocationId locationId, double amount)
{
	ysEvent ev;
	ev.type = ysEvent::TypeCounter;
	ev.counter.id = counterId;
	ev.counter.loc = locationId;
	ev.counter.when = ReadClock();
	ev.counter.amount = amount;
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
