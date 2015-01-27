// Copyright (C) 2014 Sean Middleditch, all rights reserverd.

#include "yardstick.h"

#if !defined(NO_YS)

#include <cstring>
#include <vector>
#include <tuple>
#include <algorithm>

// we need this on Windows for the default clock
#if defined(_WIN32)
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#endif

using namespace _ys__detail;

/// Definition of a location.
/// \internal
using Location = std::tuple<char const*, char const*, int>;

/// Entry for the zone stack.
/// \internal
using Zone = std::pair<ysIdT, ysClockT>;

/// A registered event sink.
/// \internal
using EventSink = std::pair<void*, ysISink*>;

namespace defaults
{
	/// Default realloc() wrapper.
	/// \internal
	static void* YS_CALL Allocator(void* block, std::size_t bytes);

	/// Default clock tick reader.
	/// \internal
	static ysClockT YS_CALL ReadClock();

	/// Default clock frequency reader.
	/// \internal
	static ysClockT YS_CALL ReadFrequency();
}

namespace
{
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
		ysAllocatorCB Alloc = defaults::Allocator;
		ysReadTicksCB ReadClockTicks = defaults::ReadClock;
		ysReadFrequencyCB ReadClockFrequency = defaults::ReadFrequency;

		std::vector<Location, YsAllocator<Location>> locations;
		std::vector<char const*, YsAllocator<char const*>> counters;
		std::vector<char const*, YsAllocator<char const*>> zones;
		std::vector<Zone, YsAllocator<Zone>> zoneStack;
		std::vector<ysISink*, YsAllocator<ysISink*>> sinks;
	};
}

/// The currently active context;
/// \internal
static Context* gContext = nullptr;

void* YS_CALL defaults::Allocator(void* block, size_t bytes)
{
	return realloc(block, bytes);
}

ysClockT defaults::ReadClock()
{
#if defined(_WIN32)
	LARGE_INTEGER tmp;
	QueryPerformanceCounter(&tmp);
	return tmp.QuadPart;
#else
#	err "Platform unsupported"
#endif
}

ysClockT defaults::ReadFrequency()
{
#if defined(_WIN32)
	LARGE_INTEGER tmp;
	QueryPerformanceFrequency(&tmp);
	return tmp.QuadPart;
#else
#	err "Platform unsupported"
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

ysError YS_API ysInitialize(ysAllocatorCB allocator, ysReadTicksCB readClock, ysReadFrequencyCB readFrequency)
{
	// only allow initializing once per process
	if (gContext != nullptr)
		return ysError::Duplicate;

	// if the user supplied a configuration of function pointers, validate it
	if (allocator == nullptr)
		return ysError::InvalidParameter;

	// determine which allocator to use
	if (allocator == nullptr)
		allocator = defaults::Allocator;

	// allocate and initialize the current context
	auto ctx = new (allocator(nullptr, sizeof(Context))) Context();
	if (ctx == nullptr)
		return ysError::ResourcesExhausted;

	// initialize the context with the user's configuration, if any
	ctx->Alloc = allocator;
	ctx->ReadClockTicks = readClock != nullptr ? readClock : defaults::ReadClock;
	ctx->ReadClockFrequency = readFrequency != nullptr ? readFrequency : defaults::ReadFrequency;

	// install the context
	gContext = ctx;

	return ysError::Success;
}

void YS_API ysShutdown()
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

YS_API ysIdT YS_CALL _ys__detail::_addLocation(char const* fileName, int line, char const* functionName)
{
	// if we have no context, we can't record this location
	if (gContext == nullptr)
		return -1;

	auto const location = std::make_tuple(fileName, functionName, line);

	size_t const index = std::find(begin(gContext->locations), end(gContext->locations), location) - begin(gContext->locations);
	if (index < gContext->locations.size())
		return static_cast<ysIdT>(index);

	gContext->locations.push_back(location);
	ysIdT const id = static_cast<ysIdT>(index);

	// tell all the sinks about the new location
	for (auto sink : gContext->sinks)
		sink->AddLocation(id, fileName, line, functionName);

	return id;
}

YS_API ysIdT YS_CALL _ys__detail::_addCounter(const char* counterName)
{
	// if we have no context, we can't record this location
	if (gContext == nullptr)
		return -1;

	// this may be a duplicate; return the existing one if so
	size_t const index = std::find_if(begin(gContext->counters), end(gContext->counters), [=](char const* str){ return std::strcmp(str, counterName) == 0; }) - begin(gContext->counters);
	if (index < gContext->counters.size())
		return static_cast<ysIdT>(index);

	gContext->counters.push_back(counterName);
	auto const id = static_cast<ysIdT>(index);

	// tell all the sinks about the new counter
	for (auto sink : gContext->sinks)
		sink->AddCounter(id, counterName);

	return id;
}

YS_API ysIdT YS_CALL _ys__detail::_addZone(const char* zoneName)
{
	// if we have no context, we can't record this location
	if (gContext == nullptr)
		return -1;

	// this may be a duplicate; return the existing one if so
	size_t const index = std::find_if(begin(gContext->zones), end(gContext->zones), [=](char const* str){ return std::strcmp(str, zoneName) == 0; }) - begin(gContext->zones);
	if (index < gContext->zones.size())
		return static_cast<ysIdT>(index);

	gContext->zones.push_back(zoneName);
	ysIdT const id = static_cast<ysIdT>(index);

	// tell all the sinks about the new zone
	for (auto sink : gContext->sinks)
		sink->AddZone(id, zoneName);

	return id;
}

YS_API void YS_CALL _ys__detail::_incrementCounter(ysIdT counterId, ysIdT locationId, double amount)
{
	// if we have no context, we can't record this counter
	if (gContext == nullptr)
		return;

	auto const now = gContext->ReadClockTicks();

	for (auto sink : gContext->sinks)
		sink->IncrementCounter(counterId, locationId, now, amount);
}

YS_API _ys__detail::_scopedZone::_scopedZone(ysIdT zoneId, ysIdT locationId)
{
	// if we have no context, we can't record this zone
	if (gContext == nullptr)
		return;

	// if the id is invalid, record nothing
	if (zoneId == -1 || locationId == -1)
		return;

	auto const now = gContext->ReadClockTicks();
	auto const depth = static_cast<ysIdT>(gContext->zoneStack.size());

	// record the zone for handling the resulting exit zone
	gContext->zoneStack.emplace_back(zoneId, now);

	for (auto sink : gContext->sinks)
		sink->EnterZone(zoneId, locationId, now, depth);
}

YS_API _ys__detail::_scopedZone::~_scopedZone()
{
	// if we have no context, we can't record this zone exit
	if (gContext == nullptr)
		return;

	// if the stack is empty, something is amiss
	if (gContext->zoneStack.empty())
		return;

	auto const& entry = gContext->zoneStack.back();
	auto const now = gContext->ReadClockTicks();
	auto const zoneId = entry.first;
	auto const start = entry.second;
	auto const elapsed = now - entry.second;

	// pop off the corresponding enter zone
	gContext->zoneStack.pop_back();
	auto const depth = static_cast<uint16_t>(gContext->zoneStack.size());

	for (auto sink : gContext->sinks)
		sink->ExitZone(zoneId, start, elapsed, depth);
}

YS_API ysError YS_CALL ysTick()
{
	// can't tick without a context
	if (gContext == nullptr)
		return ysError::UninitializedLibrary;

	auto const now = gContext->ReadClockTicks();

	for (auto sink : gContext->sinks)
		sink->Tick(now);

	return ysError::Success;
}

YS_API ysError YS_CALL ysAddSink(ysISink* sink)
{
	// can't add a sink without a context
	if (gContext == nullptr)
		return ysError::UninitializedLibrary;

	if (sink == nullptr)
		return ysError::InvalidParameter;

	gContext->sinks.push_back(sink);

	auto const now = gContext->ReadClockTicks();
	auto const freq = gContext->ReadClockFrequency();

	// start the sink
	sink->Start(now, freq);

	// register existing state

	for (auto const& location : gContext->locations)
		sink->AddLocation(static_cast<uint16_t>(&location - gContext->locations.data()), std::get<0>(location), std::get<2>(location), std::get<1>(location));

	for (auto const& counter : gContext->counters)
		sink->AddCounter(static_cast<uint16_t>(&counter - gContext->counters.data()), counter);

	for (auto const& zone : gContext->zones)
		sink->AddZone(static_cast<uint16_t>(&zone - gContext->zones.data()), zone);

	for (auto const& zone : gContext->zoneStack)
		sink->EnterZone(zone.first, 0/* FIXME */, zone.second, static_cast<uint8_t>(&zone - gContext->zoneStack.data()));

	return ysError::Success;
}

YS_API ysError YS_CALL ysRemoveSink(ysISink* sink)
{
	// can't remove a sink without a context
	if (gContext == nullptr)
		return ysError::UninitializedLibrary;

	if (sink == nullptr)
		return ysError::InvalidParameter;

	// find the sink to remove it
	auto it = std::find(begin(gContext->sinks), end(gContext->sinks), sink);
	if (it == end(gContext->sinks))
		return ysError::InvalidParameter;

	gContext->sinks.erase(it);

	// stop the sink
	auto const now = gContext->ReadClockTicks();
	sink->Stop(now);

	return ysError::Success;
}

#endif // !defined(NO_YS)
