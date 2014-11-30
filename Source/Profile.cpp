// Copyright (C) 2014 Sean Middleditch, all rights reserverd.

#include "yardstick.h"
#include "ys_private.h"
#include "Profile.h"

#if defined(ENABLE_PROFILER)

#include <cstdio>
#include <vector>
#include <tuple>
#include <algorithm>

#include "ProfileFileSink.h"

namespace
{
	std::vector<std::tuple<char const*, char const*, int>> s_Locations;
	std::vector<char const*> s_Counters;
	std::vector<char const*> s_Zones;
	std::vector<std::pair<uint16_t, uint64_t>> s_Stack;
	std::vector<IProfileSink*> s_Sinks;
}

void detail::profile::IncrementCounter(uint16_t id, uint16_t loc, double amount)
{
	auto const time = _ys_read_clock_ticks();

	for (auto sink : s_Sinks)
		if (sink != nullptr)
			sink->WriteAdjustCounter(id, loc, time, amount);
}

void detail::profile::StartZone(uint16_t id, uint16_t loc)
{
	auto const start = _ys_read_clock_ticks();
	auto const depth = static_cast<uint16_t>(s_Stack.size());

	s_Stack.emplace_back(id, start);

	for (auto sink : s_Sinks)
		if (sink != nullptr)
			sink->WriteZoneStart(id, loc, start, depth);
}

void detail::profile::StopZone()
{
	auto const end = _ys_read_clock_ticks();

	auto const& entry = s_Stack.back();
	auto const id = entry.first;
	auto const start = entry.second;
	s_Stack.pop_back();
	auto const depth = static_cast<uint16_t>(s_Stack.size());

	for (auto sink : s_Sinks)
		if (sink != nullptr)
			sink->WriteZoneEnd(id, start, end - start, depth);
}

uint16_t detail::profile::RegisterLocation(const char* file, int line, char const* function)
{
	auto const location = std::make_tuple(file, function, line);

	size_t const index = std::find(begin(s_Locations), end(s_Locations), location) - begin(s_Locations);
	if (index < s_Locations.size())
		return static_cast<uint16_t>(index);

	s_Locations.push_back(location);
	uint16_t const id = static_cast<uint16_t>(index);

	// tell all the sinks about the new counter
	for (auto sink : s_Sinks)
		if (sink != nullptr)
			sink->CreateLocationId(id, file, line, function);

	return id;
}

uint16_t detail::profile::RegisterCounter(const char* name, EProfileUnits units)
{
	// this may be a duplicate; return the existing one if so
	size_t const index = std::find_if(begin(s_Counters), end(s_Counters), [=](char const* str){ return std::strcmp(str, name) == 0; }) - begin(s_Counters);
	if (index < s_Counters.size())
		return static_cast<uint16_t>(index);

	s_Counters.push_back(name);
	uint16_t const id = static_cast<uint16_t>(index);

	// tell all the sinks about the new counter
	for (auto sink : s_Sinks)
		if (sink != nullptr)
			sink->CreateCounterId(id, name);

	return id;
}

uint16_t detail::profile::RegisterZone(const char* name)
{
	// this may be a duplicate; return the existing one if so
	size_t const index = std::find_if(begin(s_Zones), end(s_Zones), [=](char const* str){ return std::strcmp(str, name) == 0; }) - begin(s_Zones);
	if (index < s_Zones.size())
		return static_cast<uint16_t>(index);

	s_Zones.push_back(name);
	uint16_t const id = static_cast<uint16_t>(index);

	// tell all the sinks about the new counter
	for (auto sink : s_Sinks)
		if (sink != nullptr)
			sink->CreateZoneId(id, name);

	return id;
}

void detail::profile::Tick()
{
	for (auto sink : s_Sinks)
		if (sink != nullptr)
			sink->Tick();
}

void YS_API ys_shutdown()
{
	s_Counters.clear();
	s_Zones.clear();
	s_Sinks.clear();

	s_Counters.shrink_to_fit();
	s_Zones.shrink_to_fit();
	s_Sinks.shrink_to_fit();
}

void detail::profile::AddSink(IProfileSink* newSink)
{
	s_Sinks.push_back(newSink);

	// tell the sink about existing counters and zones
	for (auto const& counter : s_Counters)
		newSink->CreateCounterId(static_cast<uint16_t>(&counter - s_Counters.data()), counter);
	for (auto const& zone : s_Zones)
		newSink->CreateCounterId(static_cast<uint16_t>(&zone - s_Zones.data()), zone);
}

void detail::profile::RemoveSink(IProfileSink* oldSink)
{
	auto it = std::find(begin(s_Sinks), end(s_Sinks), oldSink);
	if (it != end(s_Sinks))
		s_Sinks.erase(it);
}

#endif