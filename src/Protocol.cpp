/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#include "yardstick/yardstick.h"

#include "ThreadState.h"
#include "GlobalState.h"

using namespace _ys_;

namespace {

template <typename T>
bool read(T& out_value, std::size_t& inout_len, void const*& inout_buffer, std::size_t& inout_available)
{
	if (sizeof(out_value) > inout_available)
		return false;

	std::memcpy(&out_value, inout_buffer, sizeof(out_value));

	reinterpret_cast<char const*&>(inout_buffer) += sizeof(out_value);
	inout_len += sizeof(out_value);
	inout_available -= sizeof(out_value);

	return true;
}

template <typename T>
bool write(T const& value, void*& inout_buffer, std::size_t& inout_available)
{
	if (sizeof(value) > inout_available)
		return false;

	std::memcpy(inout_buffer, &value, sizeof(value));

	reinterpret_cast<char*&>(inout_buffer) += sizeof(value);
	inout_available -= sizeof(value);

	return true;
}

#if defined(TRY_READ)
#	undef TRY_READ
#endif
#define TRY_READ(value) do{ if (!read((value), out_len, buffer, available)) return ysResult::NoMemory; }while(false)

#if defined(TRY_WRITE)
#	undef TRY_WRITE
#endif
#define TRY_WRITE(value) do{ if (!write((value), buffer, available)) return ysResult::NoMemory; }while(false)

} // anonymous namespace

YS_API ysResult YS_CALL _ys_::emit_event(ysEvent const& ev)
{
	char storage[64];
	void* buffer = storage;
	std::size_t available = sizeof(buffer);

	ThreadState& thrd = ThreadState::thread_instance();

	std::uint8_t const type = static_cast<std::uint8_t>(ev.type);
	TRY_WRITE(type);

	switch (ev.type)
	{
	case ysEvent::TypeNone:
		break;
	case ysEvent::TypeHeader:
		TRY_WRITE(ev.header.frequency);
		break;
	case ysEvent::TypeTick:
		TRY_WRITE(ev.tick.when);
		break;
	case ysEvent::TypeRegion:
		TRY_WRITE(ev.region.line);
		TRY_WRITE(ev.region.name);
		TRY_WRITE(ev.region.file);
		TRY_WRITE(ev.region.begin);
		TRY_WRITE(ev.region.end);
		break;
	case ysEvent::TypeCounter:
		TRY_WRITE(ev.counter.line);
		TRY_WRITE(ev.counter.name);
		TRY_WRITE(ev.counter.file);
		TRY_WRITE(ev.counter.when);
		TRY_WRITE(ev.counter.value);
		break;
	}

	GlobalState::instance().PostThreadBuffer();

	return ysResult::Success;
}

YS_API ysResult YS_CALL read_event(ysEvent& out_ev, std::size_t& out_len, void const* buffer, std::size_t available)
{
	out_len = 0;

	if (buffer == nullptr)
		return ysResult::InvalidParameter;

	std::uint8_t type;
	TRY_READ(type);
	out_ev.type = static_cast<decltype(out_ev.type)>(type);

	switch (out_ev.type)
	{
	case ysEvent::TypeNone:
		break;
	case ysEvent::TypeHeader:
		TRY_READ(out_ev.header.frequency);
		break;
	case ysEvent::TypeTick:
		TRY_READ(out_ev.tick.when);
		break;
	case ysEvent::TypeRegion:
		TRY_READ(out_ev.region.line);
		TRY_READ(out_ev.region.name);
		TRY_READ(out_ev.region.file);
		TRY_READ(out_ev.region.begin);
		TRY_READ(out_ev.region.end);
		break;
	case ysEvent::TypeCounter:
		TRY_READ(out_ev.counter.line);
		TRY_READ(out_ev.counter.name);
		TRY_READ(out_ev.counter.file);
		TRY_READ(out_ev.counter.when);
		TRY_READ(out_ev.counter.value);
		break;
	}

	return ysResult::Success;
}
