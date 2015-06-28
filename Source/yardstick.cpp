// Copyright (C) 2014 Sean Middleditch, all rights reserverd.

#include "yardstick.h"

#if !defined(NO_YS)

#include <atomic>
#include <cstring>
#include <vector>
#include <tuple>
#include <algorithm>

// we need this on Windows for the default clock
#if defined(_WIN32)
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#endif

using namespace ys;
using namespace _ys_internal;

namespace
{
	/// Definition of a location.
	/// @internal
	struct Location
	{
		char const* file;
		char const* func;
		int line;
	};

	// we don't need actually identical locations; we just want to avoid sending strings too much
	bool operator==(Location const& lhs, Location const& rhs) { return lhs.file == rhs.file && lhs.func == rhs.func && lhs.line == rhs.line; }

	/// Entry for the zone stack.
	/// @internal
	struct Zone
	{
		ZoneId id;
		Clock start;
	};

	/// Default realloc() wrapper.
	/// @internal
	void* YS_CALL DefaultAllocator(void* block, std::size_t bytes)
	{
		return realloc(block, bytes);
	}

	/// Default clock tick reader.
	/// @internal
	Clock YS_CALL DefaultReadClock()
	{
#if defined(_WIN32)
		LARGE_INTEGER tmp;
		QueryPerformanceCounter(&tmp);
		return tmp.QuadPart;
#else
#		error "Platform unsupported"
#endif
	}

	/// Default clock frequency reader.
	/// @internal
	Clock YS_CALL DefaultReadFrequency()
	{
#if defined(_WIN32)
		LARGE_INTEGER tmp;
		QueryPerformanceFrequency(&tmp);
		return tmp.QuadPart;
#else
#		error "Platform unsupported"
#endif
	}

	/// Allocator using the main context.
	/// @internal
	template <typename T>
	struct YsAllocator
	{
		using value_type = T;

		T* allocate(std::size_t bytes);
		void deallocate(T* block, std::size_t);
	};

	/// An active context.
	/// @internal
	struct Context final
	{
		AllocatorCallback Alloc = DefaultAllocator;
		ReadTicksCallback ReadClockTicks = DefaultReadClock;
		ReadFrequencyCallback ReadClockFrequency = DefaultReadFrequency;

		std::vector<Location, YsAllocator<Location>> locations;
		std::vector<char const*, YsAllocator<char const*>> counters;
		std::vector<char const*, YsAllocator<char const*>> zones;
		std::vector<Zone, YsAllocator<Zone>> zoneStack;
		std::vector<ISink*, YsAllocator<ISink*>> sinks;
	};

	/// The currently active context;
	/// @internal
	Context* gContext = nullptr;
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

ErrorCode YS_API ::_ys_internal::Initialize(AllocatorCallback allocator, ReadTicksCallback readClock, ReadFrequencyCallback readFrequency)
{
	// only allow initializing once per process
	if (gContext != nullptr)
		return ErrorCode::Duplicate;

	// if the user supplied a configuration of function pointers, validate it
	if (allocator == nullptr)
		return ErrorCode::InvalidParameter;

	// determine which allocator to use
	if (allocator == nullptr)
		allocator = &DefaultAllocator;

	// allocate and initialize the current context
	auto ctx = new (allocator(nullptr, sizeof(Context))) Context();
	if (ctx == nullptr)
		return ErrorCode::ResourcesExhausted;

	// initialize the context with the user's configuration, if any
	ctx->Alloc = allocator;
	ctx->ReadClockTicks = readClock != nullptr ? readClock : DefaultReadClock;
	ctx->ReadClockFrequency = readFrequency != nullptr ? readFrequency : DefaultReadFrequency;

	// install the context
	gContext = ctx;

	return ErrorCode::Success;
}

void YS_API ::_ys_internal::Shutdown()
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

YS_API LocationId YS_CALL ::_ys_internal::AddLocation(char const* fileName, int line, char const* functionName)
{
	// if we have no context, we can't record this location
	if (gContext == nullptr)
		return LocationId(-1);

	Location const location{ fileName, functionName, line };

	size_t const index = std::find(begin(gContext->locations), end(gContext->locations), location) - begin(gContext->locations);
	if (index < gContext->locations.size())
		return static_cast<LocationId>(index);

	gContext->locations.push_back(location);
	auto const id = static_cast<LocationId>(index);

	// tell all the sinks about the new location
	for (auto sink : gContext->sinks)
		sink->AddLocation(id, fileName, line, functionName);

	return id;
}

YS_API CounterId YS_CALL ::_ys_internal::AddCounter(const char* counterName)
{
	// if we have no context, we can't record this location
	if (gContext == nullptr)
		return CounterId(-1);

	// this may be a duplicate; return the existing one if so
	size_t const index = std::find_if(begin(gContext->counters), end(gContext->counters), [=](char const* str){ return std::strcmp(str, counterName) == 0; }) - begin(gContext->counters);
	if (index < gContext->counters.size())
		return static_cast<CounterId>(index);

	gContext->counters.push_back(counterName);
	auto const id = static_cast<CounterId>(index);

	// tell all the sinks about the new counter
	for (auto sink : gContext->sinks)
		sink->AddCounter(id, counterName);

	return id;
}

YS_API ZoneId YS_CALL ::_ys_internal::AddZone(const char* zoneName)
{
	// if we have no context, we can't record this location
	if (gContext == nullptr)
		return ZoneId(-1);

	// this may be a duplicate; return the existing one if so
	size_t const index = std::find_if(begin(gContext->zones), end(gContext->zones), [=](char const* str){ return std::strcmp(str, zoneName) == 0; }) - begin(gContext->zones);
	if (index < gContext->zones.size())
		return static_cast<ZoneId>(index);

	gContext->zones.push_back(zoneName);
	auto const id = static_cast<ZoneId>(index);

	// tell all the sinks about the new zone
	for (auto sink : gContext->sinks)
		sink->AddZone(id, zoneName);

	return id;
}

YS_API void YS_CALL ::_ys_internal::IncrementCounter(CounterId counterId, LocationId locationId, double amount)
{
	// if we have no context, we can't record this counter
	if (gContext == nullptr)
		return;

	auto const now = gContext->ReadClockTicks();

	for (auto sink : gContext->sinks)
		sink->IncrementCounter(counterId, locationId, now, amount);
}

YS_API ::_ys_internal::ScopedProfileZone::ScopedProfileZone(ZoneId zoneId, LocationId locationId)
{
	// if we have no context, we can't record this zone
	if (gContext == nullptr)
		return;

	// if the id is invalid, record nothing
	if (zoneId == ZoneId(-1) || locationId == LocationId(-1))
		return;

	auto const now = gContext->ReadClockTicks();
	auto const depth = static_cast<std::uint16_t>(gContext->zoneStack.size());

	// record the zone for handling the resulting exit zone
	gContext->zoneStack.push_back({zoneId, now});

	for (auto sink : gContext->sinks)
		sink->EnterZone(zoneId, locationId, now, depth);
}

YS_API ::_ys_internal::ScopedProfileZone::~ScopedProfileZone()
{
	// if we have no context, we can't record this zone exit
	if (gContext == nullptr)
		return;

	// if the stack is empty, something is amiss
	if (gContext->zoneStack.empty())
		return;

	auto const& entry = gContext->zoneStack.back();
	auto const now = gContext->ReadClockTicks();
	auto const zoneId = entry.id;
	auto const start = entry.start;
	auto const elapsed = now - entry.start;

	// pop off the corresponding enter zone
	gContext->zoneStack.pop_back();
	auto const depth = static_cast<uint16_t>(gContext->zoneStack.size());

	for (auto sink : gContext->sinks)
		sink->ExitZone(zoneId, start, elapsed, depth);
}

YS_API ErrorCode YS_CALL ::_ys_internal::Tick()
{
	// can't tick without a context
	if (gContext == nullptr)
		return ErrorCode::UninitializedLibrary;

	auto const now = gContext->ReadClockTicks();

	for (auto sink : gContext->sinks)
		sink->Tick(now);

	return ErrorCode::Success;
}

YS_API ErrorCode YS_CALL ::_ys_internal::AddSink(ISink* sink)
{
	// can't add a sink without a context
	if (gContext == nullptr)
		return ErrorCode::UninitializedLibrary;

	if (sink == nullptr)
		return ErrorCode::InvalidParameter;

	gContext->sinks.push_back(sink);

	auto const now = gContext->ReadClockTicks();
	auto const freq = gContext->ReadClockFrequency();

	// start the sink
	sink->Start(now, freq);

	// register existing state

	for (auto const& location : gContext->locations)
		sink->AddLocation(static_cast<LocationId>(&location - gContext->locations.data()), location.file, location.line, location.func);

	for (auto const& counter : gContext->counters)
		sink->AddCounter(static_cast<CounterId>(&counter - gContext->counters.data()), counter);

	for (auto const& zone : gContext->zones)
		sink->AddZone(static_cast<ZoneId>(&zone - gContext->zones.data()), zone);

	for (auto const& zone : gContext->zoneStack)
		sink->EnterZone(zone.id, LocationId(-1)/* FIXME */, zone.start, static_cast<uint16_t>(&zone - gContext->zoneStack.data()));

	return ErrorCode::Success;
}

YS_API ErrorCode YS_CALL ::_ys_internal::RemoveSink(ISink* sink)
{
	// can't remove a sink without a context
	if (gContext == nullptr)
		return ErrorCode::UninitializedLibrary;

	if (sink == nullptr)
		return ErrorCode::InvalidParameter;

	// find the sink to remove it
	auto it = std::find(begin(gContext->sinks), end(gContext->sinks), sink);
	if (it == end(gContext->sinks))
		return ErrorCode::InvalidParameter;

	gContext->sinks.erase(it);

	// stop the sink
	auto const now = gContext->ReadClockTicks();
	sink->Stop(now);

	return ErrorCode::Success;
}

#endif // !defined(NO_YS)
