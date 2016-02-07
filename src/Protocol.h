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

#pragma once

namespace _ys_ {

enum class EventType : std::uint8_t { None = 0, Header = 1, Tick = 2, Region = 3, CounterSet = 4, String = 5, CounterAdd = 6 };

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
		} counter_set;
		struct
		{
			std::uint32_t line;
			char const* name;
			char const* file;
			ysTime when;
			double value;
		} record;
		struct
		{
			ysStringHandle id;
			std::uint16_t size;
			char const* str;
		} string;
		struct
		{
			char const* name;
			double amount;
		} counter_add;
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