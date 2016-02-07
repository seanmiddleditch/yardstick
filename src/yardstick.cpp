// Copyright (C) 2014-2016 Sean Middleditch, all rights reserverd.

#include "GlobalState.h"
#include "ThreadState.h"
#include "Clock.h"

using namespace _ys_;

namespace
{
	ysResult EmitEvent(EventData const& ev)
	{
		ThreadState& thrd = ThreadState::thread_instance();
		thrd.Enque(ev);
		return ysResult::Success;
	}
}

ysResult YS_API _ys_::initialize(ysAllocator allocator)
{
	return GlobalState::instance().Initialize(allocator);
}

ysResult YS_API _ys_::shutdown()
{
	return GlobalState::instance().Shutdown();
}

YS_API ysResult YS_CALL _ys_::emit_counter_set(ysTime when, double value, char const* name, char const* file, int line)
{
	EventData ev;
	ev.type = EventType::CounterSet;
	ev.counter_set.line = line;
	ev.counter_set.name = name;
	ev.counter_set.file = file;
	ev.counter_set.when = when;
	ev.counter_set.value = value;
	return EmitEvent(ev);
}

YS_API ysResult YS_CALL _ys_::emit_counter_add(double amount, char const* name)
{
	EventData ev;
	ev.type = EventType::CounterAdd;
	ev.counter_add.name = name;
	ev.counter_add.amount = amount;
	return EmitEvent(ev);
}

YS_API ysResult YS_CALL _ys_::emit_enter_region(ysTime when, char const* name, char const* file, int line)
{
	EventData ev;
	ev.type = EventType::EnterRegion;
	ev.enter_region.line = line;
	ev.enter_region.name = name;
	ev.enter_region.file = file;
	ev.enter_region.when = when;
	return EmitEvent(ev);
}

YS_API ysResult YS_CALL _ys_::emit_leave_region(ysTime when)
{
	EventData ev;
	ev.type = EventType::LeaveRegion;
	ev.leave_region.when = when;
	return EmitEvent(ev);
}

YS_API ysTime YS_CALL _ys_::read_clock()
{
	return ReadClock();
}

YS_API ysResult YS_CALL _ys_::tick()
{
	EventData ev;
	ev.type = EventType::Tick;
	ev.tick.when = ReadClock();
	return EmitEvent(ev);
}

YS_API ysResult YS_CALL _ys_::listen_web(unsigned short port)
{
	return GlobalState::instance().ListenWebsocket(port);
}
