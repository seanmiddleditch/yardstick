// Copyright (C) 2014 Sean Middleditch, all rights reserverd.

#include "yardstick.h"
#include "yardstick.hpp"

#include <cstring>
#include <vector>
#include <tuple>
#include <algorithm>

// we need this on Windows for the default clock
#if defined(_WIN32)
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#endif

namespace
{
	/// Definition of a location.
	/// \internal
	using Location = std::tuple<char const*, char const*, int>;

	/// Entry for the zone stack.
	/// \internal
	using Zone = std::pair<ys_id_t, ys_clock_t>;

	/// A registered event sink.
	/// \internal
	using EventSink = std::pair<void*, ys_event_cb>;

	/// Default realloc() wrapper.
	/// \internal
	void* YS_CALL DefaultAllocator(void* block, size_t bytes);

	/// Default clock tick reader.
	/// \internal
	ys_clock_t YS_CALL DefaultReadClockTicks();

	/// Default clock frequency reader.
	/// \internal
	ys_clock_t YS_CALL DefaultReadClockFrequency();

	/// Allocator using the main context.
	/// \internal
	template <typename T>
	struct YsAllocator
	{
		using value_type = T;

		T* allocate(std::size_t bytes);
		void deallocate(T* block, std::size_t);
	};

	/// An active context.
	/// \internal
	struct Context final
	{
		ys_alloc_cb Alloc = DefaultAllocator;
		ys_read_clock_ticks_cb ReadClockTicks = DefaultReadClockTicks;
		ys_read_clock_frequency_cb ReadClockFrequency = DefaultReadClockFrequency;

		std::vector<Location, YsAllocator<Location>> locations;
		std::vector<char const*, YsAllocator<char const*>> counters;
		std::vector<char const*, YsAllocator<char const*>> zones;
		std::vector<Zone, YsAllocator<Zone>> zoneStack;
		std::vector<EventSink, YsAllocator<EventSink>> sinks;
	};

	/// The currently active context;
	Context* gContext = nullptr;
}

namespace
{
	void* YS_CALL DefaultAllocator(void* block, size_t bytes)
	{
		return realloc(block, bytes);
	}

	ys_clock_t DefaultReadClockTicks()
	{
#if defined(_WIN32)
		LARGE_INTEGER tmp;
		QueryPerformanceCounter(&tmp);
		return tmp.QuadPart;
#else
#	error "Platform unsupported"
#endif
	}

	ys_clock_t DefaultReadClockFrequency()
	{
#if defined(_WIN32)
		LARGE_INTEGER tmp;
		QueryPerformanceFrequency(&tmp);
		return tmp.QuadPart;
#else
#	error "Platform unsupported"
#endif
	}

	template <typename T>
	T* YsAllocator<T>::allocate(std::size_t count)
	{
		return static_cast<T*>(gContext->Alloc(nullptr, count * sizeof(T)));
	}

	template <typename T>
	void YsAllocator<T>::deallocate(T* block, std::size_t)
	{
		gContext->Alloc(block, 0U);
	}
}

extern "C" {

ys_error_t YS_API _ys_initialize(ys_configuration_t const* config, size_t size)
{
	// only allow initializing once per process
	if (gContext != nullptr)
		return YS_ONCE_ONLY;

	// if the user supplied a configuration of function pointers, validate it
	if (config != nullptr && size != sizeof(ys_configuration_t))
		return YS_INVALID_PARAMETER;

	// handle under-sized configs from older versions
	ys_configuration_t tmp = YS_DEFAULT_CONFIGURATION;
	std::memcpy(&tmp, config, size);

	// determine which allocator to use
	ys_alloc_cb allocator = tmp.alloc != nullptr ? tmp.alloc : DefaultAllocator;

	// allocate and initialize the current context
	auto ctx = new (allocator(nullptr, sizeof(Context))) Context();
	if (ctx == nullptr)
		return YS_RESOURCES_EXHAUSTED;

	// initialize the context with the user's configuration, if any
	ctx->Alloc = allocator;
	if (tmp.read_clock_ticks != nullptr)
		ctx->ReadClockTicks = tmp.read_clock_ticks;
	if (tmp.read_clock_frequency != nullptr)
		ctx->ReadClockFrequency = tmp.read_clock_frequency;

	// install the context
	gContext = ctx;

	return YS_OK;
}

void YS_API _ys_shutdown()
{
	// check the context to ensure it's valid
	auto ctx = gContext;
	if (ctx == nullptr)
		return;

	ctx->locations.clear();
	ctx->counters.clear();
	ctx->zones.clear();
	ctx->zoneStack.clear();
	ctx->sinks.clear();

	ctx->locations.shrink_to_fit();
	ctx->counters.shrink_to_fit();
	ctx->zones.shrink_to_fit();
	ctx->zoneStack.shrink_to_fit();
	ctx->sinks.shrink_to_fit();

	// release the context
	gContext->~Context();
	gContext->Alloc(ctx, 0U);

	// mark the global context as 'finalized' so it can never be reallocated.
	// FIXME: this should be done sooner than later, but the shink_to_fit calls above need gContext to deallocate memory
	std::memset(&gContext, 0xDD, sizeof(gContext));
}

YS_API ys_id_t YS_CALL _ys_add_location(char const* fileName, int line, char const* functionName)
{
	auto const location = std::make_tuple(fileName, functionName, line);

	size_t const index = std::find(begin(gContext->locations), end(gContext->locations), location) - begin(gContext->locations);
	if (index < gContext->locations.size())
		return static_cast<ys_id_t>(index);

	gContext->locations.push_back(location);
	ys_id_t const id = static_cast<ys_id_t>(index);

	// construct the event
	ys_event_t ev;
	ev.add_location.type = YS_EV_ADD_LOCATION;
	ev.add_location.locationId = id;
	ev.add_location.fileName = fileName;
	ev.add_location.lineNumber = line;
	ev.add_location.functionName = functionName;

	// tell all the sinks about the new location
	for (auto const& sink : gContext->sinks)
		sink.second(sink.first, &ev);

	return id;
}

YS_API ys_id_t YS_CALL _ys_add_counter(const char* counterName)
{
	// this may be a duplicate; return the existing one if so
	size_t const index = std::find_if(begin(gContext->counters), end(gContext->counters), [=](char const* str){ return std::strcmp(str, counterName) == 0; }) - begin(gContext->counters);
	if (index < gContext->counters.size())
		return static_cast<ys_id_t>(index);

	gContext->counters.push_back(counterName);
	auto const id = static_cast<ys_id_t>(index);

	// construct the event
	ys_event_t ev;
	ev.add_counter.type = YS_EV_ADD_COUNTER;
	ev.add_counter.counterId = id;
	ev.add_counter.counterName = counterName;

	// tell all the sinks about the new counter
	for (auto sink : gContext->sinks)
		sink.second(sink.first, &ev);

	return ev.add_counter.counterId;
}

YS_API ys_id_t YS_CALL _ys_add_zone(const char* zoneName)
{
	// this may be a duplicate; return the existing one if so
	size_t const index = std::find_if(begin(gContext->zones), end(gContext->zones), [=](char const* str){ return std::strcmp(str, zoneName) == 0; }) - begin(gContext->zones);
	if (index < gContext->zones.size())
		return static_cast<ys_id_t>(index);

	gContext->zones.push_back(zoneName);
	ys_id_t const id = static_cast<ys_id_t>(index);

	// construct the event
	ys_event_t ev;
	ev.add_zone.type = YS_EV_ADD_ZONE;
	ev.add_zone.zoneId = id;
	ev.add_zone.zoneName = zoneName;

	// tell all the sinks about the new zone
	for (auto sink : gContext->sinks)
		sink.second(sink.first, &ev);

	return id;
}

YS_API void YS_CALL _ys_increment_counter(ys_id_t counterId, ys_id_t locationId, double amount)
{
	ys_event_t ev;
	ev.increment_counter.clockNow = gContext->ReadClockTicks();
	ev.increment_counter.type = YS_EV_INCREMENT_COUNTER;
	ev.increment_counter.counterId = counterId;
	ev.increment_counter.locationId = locationId;
	ev.increment_counter.amount = amount;

	for (auto sink : gContext->sinks)
		sink.second(sink.first, &ev);
}

YS_API void YS_CALL _ys_enter_zone(ys_id_t zoneId, ys_id_t locationId)
{
	ys_event_t ev;
	ev.enter_zone.clockNow = gContext->ReadClockTicks();
	ev.enter_zone.type = YS_EV_ENTER_ZONE;
	ev.enter_zone.zoneId = zoneId;
	ev.enter_zone.locationId = locationId;
	ev.enter_zone.depth = static_cast<ys_id_t>(gContext->zoneStack.size());

	// record the zone for handling the resulting exit zone
	gContext->zoneStack.emplace_back(zoneId, ev.enter_zone.clockNow);

	for (auto sink : gContext->sinks)
		sink.second(sink.first, &ev);
}

YS_API void YS_CALL _ys_exit_zone()
{
	auto const now = gContext->ReadClockTicks();

	auto const& entry = gContext->zoneStack.back();

	ys_event_t ev;
	ev.exit_zone.type = YS_EV_EXIT_ZONE;
	ev.exit_zone.zoneId = entry.first;
	ev.exit_zone.clockStart = entry.second;
	ev.exit_zone.clockElapsed = now - entry.second;

	// pop off the corresponding enter zone
	gContext->zoneStack.pop_back();
	ev.exit_zone.depth = static_cast<uint16_t>(gContext->zoneStack.size());

	for (auto sink : gContext->sinks)
		sink.second(sink.first, &ev);
}

YS_API void YS_CALL _ys_tick()
{
	ys_event_t ev;
	ev.tick.clockNow = gContext->ReadClockTicks();
	ev.tick.type = YS_EV_TICK;

	for (auto sink : gContext->sinks)
		sink.second(sink.first, &ev);
}

YS_API ys_error_t YS_CALL _ys_add_sink(void* userData, ys_event_cb callback)
{
	if (callback == nullptr)
		return YS_INVALID_PARAMETER;

	gContext->sinks.emplace_back(userData, callback);

	auto const now = gContext->ReadClockTicks();
	auto const freq = gContext->ReadClockFrequency();

	// start the sink
	ys_event_t ev;
	ev.start.type = YS_EV_START;
	ev.start.clockNow = now;
	ev.start.clockFrequency = freq;

	callback(userData, &ev);

	// register existing state

	ev.type = YS_EV_ADD_LOCATION;
	for (auto const& location : gContext->locations)
	{
		ev.add_location.locationId = static_cast<uint16_t>(&location - gContext->locations.data());
		ev.add_location.fileName = std::get<0>(location);
		ev.add_location.functionName = std::get<1>(location);
		ev.add_location.lineNumber = std::get<2>(location);
		callback(userData, &ev);
	}

	ev.type = YS_EV_ADD_COUNTER;
	for (auto const& counter : gContext->counters)
	{
		ev.add_counter.counterId = static_cast<uint16_t>(&counter - gContext->counters.data());
		ev.add_counter.counterName = counter;
		callback(userData, &ev);
	}

	ev.type = YS_EV_ADD_ZONE;
	for (auto const& zone : gContext->zones)
	{
		ev.add_zone.zoneId = static_cast<uint16_t>(&zone - gContext->zones.data());
		ev.add_zone.zoneName = zone;
		callback(userData, &ev);
	}

	ev.type = YS_EV_ENTER_ZONE;
	for (auto const& zone : gContext->zoneStack)
	{
		ev.enter_zone.zoneId = zone.first;
		ev.enter_zone.locationId = 0; // FIXME
		ev.enter_zone.clockNow = zone.second;
		ev.enter_zone.depth = static_cast<uint8_t>(&zone - gContext->zoneStack.data());
		callback(userData, &ev);
	}

	return YS_OK;
}

YS_API void YS_CALL _ys_remove_sink(void* userData, ys_event_cb callback)
{
	auto const sink = std::make_pair(userData, callback);

	// find the sink to remove it
	auto it = std::find(begin(gContext->sinks), end(gContext->sinks), sink);
	if (it == end(gContext->sinks))
		return;

	gContext->sinks.erase(it);

	// stop the sink
	ys_event_t ev;
	ev.stop.type = YS_EV_STOP;
	ev.stop.clockNow = gContext->ReadClockTicks();

	callback(userData, &ev);
}

} // extern "C"
