/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#include "yardstick/yardstick.h"

#include "ThreadState.h"
#include "GlobalState.h"
#include "PointerHash.h"

using namespace _ys_;

namespace {

template <typename T>
bool read(T& out_value, void const* buffer, std::size_t available, std::size_t& inout_read)
{
	if (sizeof(out_value) > available - inout_read)
		return false;

	std::memcpy(&out_value, static_cast<char const*>(buffer) + inout_read, sizeof(out_value));
	inout_read += sizeof(out_value);

	return true;
}

template <typename T>
bool write(T const& value, void* buffer, std::size_t available, std::size_t& inout_written)
{
	if (sizeof(value) > available - inout_written)
		return false;

	std::memcpy(static_cast<char*>(buffer) + inout_written, &value, sizeof(value));
	inout_written += sizeof(value);

	return true;
}

#if defined(TRY_READ)
#	undef TRY_READ
#endif
#define TRY_READ(value) do{ if (!read((value), buffer, available, out_length)) return ysResult::NoMemory; }while(false)

#if defined(TRY_WRITE)
#	undef TRY_WRITE
#endif
#define TRY_WRITE(value) do{ if (!write((value), out_buffer, available, out_length)) return ysResult::NoMemory; }while(false)

} // anonymous namespace

ysResult _ys_::EncodeEvent(void* out_buffer, std::size_t available, EventData const& ev, std::size_t& out_length)
{
	out_length = 0;

	if (out_buffer == nullptr)
		return ysResult::InvalidParameter;

	std::uint8_t const type = static_cast<std::uint8_t>(ev.type);
	TRY_WRITE(type);

	switch (ev.type)
	{
	case EventData::TypeNone:
		break;
	case EventData::TypeHeader:
		TRY_WRITE(ev.header.frequency);
		break;
	case EventData::TypeTick:
		TRY_WRITE(ev.tick.when);
		break;
	case EventData::TypeRegion:
		TRY_WRITE(ev.region.line);
		TRY_WRITE(hash_pointer(ev.region.name));
		TRY_WRITE(hash_pointer(ev.region.file));
		TRY_WRITE(ev.region.begin);
		TRY_WRITE(ev.region.end);
		break;
	case EventData::TypeCounter:
		TRY_WRITE(ev.counter.line);
		TRY_WRITE(hash_pointer(ev.counter.name));
		TRY_WRITE(hash_pointer(ev.counter.file));
		TRY_WRITE(ev.counter.when);
		TRY_WRITE(ev.counter.value);
		break;
	case EventData::TypeString:
		TRY_WRITE(ev.string.id);
		TRY_WRITE(ev.string.size);
		std::memcpy(static_cast<char*>(out_buffer) + out_length, ev.string.str, ev.string.size);
		break;
	}

	return ysResult::Success;
}

//YS_API ysResult YS_CALL read_event(ysEvent& out_ev, void const* buffer, std::size_t available, ysStringMapper mapper, std::size_t& out_length)
//{
//	out_length = 0;
//
//	if (buffer == nullptr)
//		return ysResult::InvalidParameter;
//
//	std::uint8_t type;
//	TRY_READ(type);
//	out_ev.type = static_cast<decltype(out_ev.type)>(type);
//
//	ysStringHandle str1, str2;
//
//	switch (out_ev.type)
//	{
//	case ysEvent::TypeNone:
//		break;
//	case ysEvent::TypeHeader:
//		TRY_READ(out_ev.header.frequency);
//		break;
//	case ysEvent::TypeTick:
//		TRY_READ(out_ev.tick.when);
//		break;
//	case ysEvent::TypeRegion:
//		TRY_READ(out_ev.region.line);
//		TRY_READ(str1);
//		out_ev.region.name = mapper(str1, 0, nullptr);
//		TRY_READ(str2);
//		out_ev.region.file = mapper(str2, 0, nullptr);
//		TRY_READ(out_ev.region.begin);
//		TRY_READ(out_ev.region.end);
//		break;
//	case ysEvent::TypeCounter:
//		TRY_READ(out_ev.counter.line);
//		TRY_READ(str1);
//		out_ev.counter.name = mapper(str1, 0, nullptr);
//		TRY_READ(str2);
//		out_ev.counter.file = mapper(str1, 0, nullptr);
//		TRY_READ(out_ev.counter.when);
//		TRY_READ(out_ev.counter.value);
//		break;
//	case ysEvent::TypeString:
//		TRY_READ(out_ev.string.id);
//		TRY_READ(out_ev.string.size);
//		if (available - out_length < out_ev.string.size)
//			return ysResult::NoMemory;
//		mapper(out_ev.string.id, out_ev.string.size, static_cast<char const*>(buffer) + out_length);
//		out_length += out_ev.string.size;
//		// #FIXME - where to store the string?
//		break;
//	}
//
//	return ysResult::Success;
//}
