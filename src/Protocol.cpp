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

#include <yardstick/yardstick.h>

#include "Protocol.h"
#include "PointerHash.h"

#include <cstring>

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
	case EventType::None:
		break;
	case EventType::Header:
		TRY_WRITE(ev.header.frequency);
		TRY_WRITE(ev.header.start);
		break;
	case EventType::Tick:
		TRY_WRITE(ev.tick.when);
		break;
	case EventType::Region:
		TRY_WRITE(ev.counter_set.line);
		TRY_WRITE(hash_pointer(ev.counter_set.name));
		TRY_WRITE(hash_pointer(ev.counter_set.file));
		TRY_WRITE(ev.counter_set.begin);
		TRY_WRITE(ev.counter_set.end);
		break;
	case EventType::CounterSet:
		TRY_WRITE(ev.record.line);
		TRY_WRITE(hash_pointer(ev.record.name));
		TRY_WRITE(hash_pointer(ev.record.file));
		TRY_WRITE(ev.record.when);
		TRY_WRITE(ev.record.value);
		break;
	case EventType::String:
		TRY_WRITE(ev.string.id);
		TRY_WRITE(ev.string.size);
		std::memcpy(static_cast<char*>(out_buffer) + out_length, ev.string.str, ev.string.size);
		out_length += ev.string.size;
		break;
	case EventType::CounterAdd:
		TRY_WRITE(hash_pointer(ev.counter_add.name));
		TRY_WRITE(ev.counter_add.amount);
		break;
	}

	return ysResult::Success;
}

std::size_t _ys_::EncodeSize(EventData const& ev)
{
	switch (ev.type)
	{
	case EventType::None:
		return 1/*type*/;
	case EventType::Header:
		return 1/*type*/ + 8/*frequency*/ + 8/*start*/;
	case EventType::Tick:
		return 1/*type*/ + 8/*time*/;
	case EventType::Region:
		return 1/*type*/ + 4/*line*/ + 4/*name*/ + 4/*file*/ + 8/*start*/ + 8/*end*/;
	case EventType::CounterSet:
		return 1/*type*/ + 4/*line*/ + 4/*name*/ + 4/*file*/ + 8/*time*/ + 8/*value*/;
	case EventType::String:
		return 1/*type*/ + 4/*id*/ + 2/*size*/ + ev.string.size/*data*/;
	case EventType::CounterAdd:
		return 1/*type*/ + 4/*name*/ + 8/*amount*/;
	default:
		return std::size_t(-1);
	}
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
