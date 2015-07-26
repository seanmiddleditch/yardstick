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

	/// Default realloc() wrapper.
	/// @internal
	void* YS_CALL SystemAllocator(void* block, std::size_t bytes)
	{
		return realloc(block, bytes);
	}

	/// Default clock tick reader.
	/// @internal
	ysTime YS_CALL ReadSystemClock()
	{
#if defined(_WIN32) || defined(_WIN64)
		LARGE_INTEGER tmp;
		QueryPerformanceCounter(&tmp);
		return tmp.QuadPart;
#else
#		error "Platform unsupported"
#endif
	}

	/// Default clock frequency reader.
	/// @internal
	ysTime YS_CALL ReadSystemFrequency()
	{
#if defined(_WIN32) || defined(_WIN64)
		LARGE_INTEGER tmp;
		QueryPerformanceFrequency(&tmp);
		return tmp.QuadPart;
#else
#		error "Platform unsupported"
#endif
	}

	/// Default sink if none if provided, does nothing
	/// @internal
	class NullSink : public ysSink
	{
	public:
		void YS_CALL BeginProfile(ysTime clockNow, ysTime clockFrequency) override {}
		void YS_CALL EndProfile(ysTime clockNow) override {}
		void YS_CALL AddLocation(ysLocationId locatonId, char const* fileName, int line, char const* functionName) override {}
		void YS_CALL AddCounter(ysCounterId counterId, char const* counterName) override {}
		void YS_CALL AddZone(ysZoneId zoneId, char const* zoneName) override {}
		void YS_CALL IncrementCounter(ysCounterId counterId, ysLocationId locationId, uint64_t clockNow, double value) override {}
		void YS_CALL RecordZone(ysZoneId zoneId, ysLocationId, ysTime clockStart, uint64_t clockEnd) override {}
		void YS_CALL Tick(ysTime clockNow) override {}
	} gNullSink;

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
		ysAllocator allocator = SystemAllocator;
		ysReadClock readClock = ReadSystemClock;
		ysReadFrequency readFrequency = ReadSystemFrequency;
		ysSink* sink = nullptr;

		std::vector<Location, YsAllocator<Location>> locations;
		std::vector<char const*, YsAllocator<char const*>> counters;
		std::vector<char const*, YsAllocator<char const*>> zones;
	};

	/// The currently active context;
	/// @internal
	Context* gContext = nullptr;
}

template <typename T>
T* YsAllocator<T>::allocate(std::size_t count)
{
	return static_cast<T*>(gContext->allocator(nullptr, count * sizeof(T)));
}

template <typename T>
void YsAllocator<T>::deallocate(T* block, std::size_t)
{
	gContext->allocator(block, 0U);
}

ysErrorCode YS_API _ysInitialize(ysConfiguration const& config)
{
	// only allow initializing once per process
	if (gContext != nullptr)
		return ysErrorCode::Duplicate;

	// validate arguments
	if (config.allocator == nullptr)
		return ysErrorCode::InvalidParameter;
	if (config.readClock == nullptr)
		return ysErrorCode::InvalidParameter;
	if (config.readFrequency == nullptr)
		return ysErrorCode::InvalidParameter;
	if (config.sink == nullptr)
		return ysErrorCode::InvalidParameter;

	// allocate and initialize the current context
	auto ctx = new (config.allocator(nullptr, sizeof(Context))) Context();
	if (ctx == nullptr)
		return ysErrorCode::ResourcesExhausted;

	// initialize the context with the user's configuration, if any
	ctx->allocator = config.allocator;
	ctx->sink = config.sink;
	ctx->readClock = config.readClock;
	ctx->readFrequency = config.readFrequency;

	// install the context
	gContext = ctx;

	return ysErrorCode::Success;
}

void YS_API _ysShutdown()
{
	// check the context to ensure it's valid
	auto ctx = gContext;
	if (ctx == nullptr)
		return;

	ctx->locations.clear();
	ctx->counters.clear();
	ctx->zones.clear();

	ctx->locations.shrink_to_fit();
	ctx->counters.shrink_to_fit();
	ctx->zones.shrink_to_fit();

	// release the context
	gContext->~Context();
	gContext->allocator(ctx, 0U);

	// mark the global context as 'finalized' so it can never be reallocated.
	// FIXME: this should be done sooner than later, but the shink_to_fit calls above need gContext to deallocate memory
	std::memset(&gContext, 0xDD, sizeof(gContext));
}

YS_API void* YS_CALL _ysAlloc(std::size_t size)
{
	if (gContext == nullptr)
		return nullptr;

	return gContext->allocator(nullptr, size);
}

YS_API void* YS_CALL _ysFree(void* ptr)
{
	if (gContext == nullptr)
		return nullptr;

	return gContext->allocator(ptr, 0);
}

YS_API ysLocationId YS_CALL _ysAddLocation(char const* fileName, int line, char const* functionName)
{
	// if we have no context, we can't record this location
	if (gContext == nullptr)
		return ysLocationId::None;

	Location const location{ fileName, functionName, line };

	size_t const index = std::find(begin(gContext->locations), end(gContext->locations), location) - begin(gContext->locations);
	auto const id = ysLocationId(index + 1);
	if (index < gContext->locations.size())
		return id;

	gContext->locations.push_back(location);

	gContext->sink->AddLocation(id, fileName, line, functionName);

	return id;
}

YS_API ysCounterId YS_CALL _ysAddCounter(const char* counterName)
{
	// if we have no context, we can't record this location
	if (gContext == nullptr)
		return ysCounterId::None;

	// this may be a duplicate; return the existing one if so
	size_t const index = std::find_if(begin(gContext->counters), end(gContext->counters), [=](char const* str){ return std::strcmp(str, counterName) == 0; }) - begin(gContext->counters);
	auto const id = ysCounterId(index + 1);
	if (index < gContext->counters.size())
		return id;

	gContext->counters.push_back(counterName);

	gContext->sink->AddCounter(id, counterName);

	return id;
}

YS_API ysZoneId YS_CALL _ysAddZone(const char* zoneName)
{
	// if we have no context, we can't record this location
	if (gContext == nullptr)
		return ysZoneId::None;

	// this may be a duplicate; return the existing one if so
	size_t const index = std::find_if(begin(gContext->zones), end(gContext->zones), [=](char const* str){ return std::strcmp(str, zoneName) == 0; }) - begin(gContext->zones);
	auto const id = ysZoneId(index + 1);
	if (index < gContext->zones.size())
		return id;

	gContext->zones.push_back(zoneName);

	gContext->sink->AddZone(id, zoneName);

	return id;
}

YS_API void YS_CALL _ysIncrementCounter(ysCounterId counterId, ysLocationId locationId, double amount)
{
	// if we have no context, we can't record this counter
	if (gContext == nullptr)
		return;

	auto const now = gContext->readClock();

	gContext->sink->IncrementCounter(counterId, locationId, now, amount);
}

YS_API ::ysConfiguration::ysConfiguration() : allocator(SystemAllocator), readClock(ReadSystemClock), readFrequency(ReadSystemFrequency), sink(&gNullSink)
{
}

YS_API _ysScopedProfileZone::_ysScopedProfileZone(ysZoneId zoneId, ysLocationId locationId) : zoneId(zoneId), locationId(locationId)
{
	// if we have no context, we can't record this zone
	if (gContext == nullptr)
		return;

	startTime = gContext->readClock();
}

YS_API _ysScopedProfileZone::~_ysScopedProfileZone()
{
	// if we have no context, we can't record this zone exit
	if (gContext == nullptr)
		return;

	auto const now = gContext->readClock();

	gContext->sink->RecordZone(zoneId, this->locationId, this->startTime, now);
}

YS_API ysErrorCode YS_CALL _ysTick()
{
	// can't tick without a context
	if (gContext == nullptr)
		return ysErrorCode::UninitializedLibrary;

	auto const now = gContext->readClock();

	gContext->sink->Tick(now);

	return ysErrorCode::Success;
}

#endif // !defined(NO_YS)
