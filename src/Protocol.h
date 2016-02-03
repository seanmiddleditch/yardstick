/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

namespace _ys_ {

enum class EventType : std::uint8_t { None = 0, Header = 1, Tick = 2, Region = 3, Counter = 4, String = 5 };

struct EventData
{
	EventType type;
	union
	{
		struct
		{
			ysTime frequency;
			ysTime start;
		} header;
		struct
		{
			ysTime when;
		} tick;
		struct
		{
			std::uint32_t line;
			char const* name;
			char const* file;
			ysTime begin;
			ysTime end;
		} region;
		struct
		{
			std::uint32_t line;
			char const* name;
			char const* file;
			ysTime when;
			double value;
		} counter;
		struct
		{
			ysStringHandle id;
			std::uint16_t size;
			char const* str;
		} string;
	};
};

/// <summary> Writes an event into a buffer. </summary>
/// <param name="out_buffer"> [in,out] The position of a buffer to write the event into. </param>
/// <param name="available"> Length of the buffer from the given position. </param>
/// <param name="ev"> The evevent to be written. </param>
/// <param name="out_length"> [in,out] Number of bytes written into the buffer. </param>
/// <returns> ysResult::NoMemory if the buffer is not big enough, otherwise ysResult::Success. </returns>
ysResult EncodeEvent(void* out_buffer, std::size_t available, EventData const& ev, std::size_t& out_length);

/// <summary> Returns the amount of space needed to encode an event. </summary>
std::size_t EncodeSize(EventData const& ev);

} // namespace _ys_