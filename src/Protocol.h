/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

namespace _ys_ {

struct EventData
{
	enum { TypeNone = 0, TypeHeader = 1, TypeTick = 2, TypeRegion = 3, TypeCounter = 4, TypeString = 5 } type;
	union
	{
		struct
		{
			ysTime frequency;
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

} // namespace _ys_