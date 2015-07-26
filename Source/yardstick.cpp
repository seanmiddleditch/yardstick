// Copyright (C) 2014 Sean Middleditch, all rights reserverd.

#include "yardstick.h"

#if !defined(NO_YS)

#include <atomic>
#include <cstring>
#include <vector>
#include <mutex>

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
	bool operator==(Location const& lhs, Location const& rhs)
	{
		return 0 == std::strcmp(lhs.file, rhs.file) &&
			0 == std::strcmp(lhs.func, rhs.func) &&
			lhs.line == rhs.line;
	}

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

	/// Find the index of a value in an array.
	template <typename T>
	std::size_t FindValue(T const* first, T const* last, T const& value)
	{
		for (T const* it = first; it != last; ++it)
			if (*it == value)
				return it - first;
		return last - first;
	}

	/// Find the first occurance of a predicate in an array
	template <typename T, typename Pred>
	std::size_t FindIf(T const* first, T const* last, Pred const& pred)
	{
		for (T const* it = first; it != last; ++it)
			if (pred(*it))
				return it - first;
		return last - first;
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
		void YS_CALL AddRegion(ysRegionId regionId, char const* zoneName) override {}
		void YS_CALL IncrementCounter(ysCounterId counterId, ysLocationId locationId, uint64_t clockNow, double value) override {}
		void YS_CALL EmitRegion(ysRegionId regionId, ysLocationId, ysTime clockStart, uint64_t clockEnd) override {}
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

	/// Default configuration.
	/// @internal
	struct DefaultConfiguration : ysConfiguration
	{
		DefaultConfiguration()
		{
			allocator = SystemAllocator;
			readClock = ReadSystemClock;
			readFrequency = ReadSystemFrequency;
			sink = &gNullSink;
		}
	};

	bool gInitialized = false;
	bool gCapturing = false;
	DefaultConfiguration gConfig;
	ysSink* gActiveSink = &gNullSink;
	std::vector<Location, YsAllocator<Location>> gLocations;
	std::vector<char const*, YsAllocator<char const*>> gCounters;
	std::vector<char const*, YsAllocator<char const*>> gRegions;
	std::mutex gMutex;
}

template <typename T>
T* YsAllocator<T>::allocate(std::size_t count)
{
	return static_cast<T*>(gConfig.allocator(nullptr, count * sizeof(T)));
}

template <typename T>
void YsAllocator<T>::deallocate(T* block, std::size_t)
{
	gConfig.allocator(block, 0U);
}

ysErrorCode YS_API _ysInitialize(ysConfiguration const& config)
{
	std::unique_lock<std::mutex> lock(gMutex);

	// only allow initializing once per process
	if (gInitialized)
		return ysErrorCode::AlreadyInitialized;

	// initialize the context with the user's configuration, if any
	if (config.allocator != nullptr)
		gConfig.allocator = config.allocator;

	if (config.sink != nullptr)
		gConfig.sink = config.sink;

	if (config.readClock != nullptr)
		gConfig.readClock = config.readClock;

	if (config.readFrequency != nullptr)
		gConfig.readFrequency = config.readFrequency;

	gInitialized = true;

	return ysErrorCode::Success;
}

ysErrorCode YS_API _ysShutdown()
{
	// the user possibly didn't bother with StopProfile first
	_ysStopProfile();

	std::unique_lock<std::mutex> lock(gMutex);

	if (!gInitialized)
		return ysErrorCode::Uninitialized;

	// do this first, so checks from here on out "just work"
	gInitialized = false;

	gLocations.clear();
	gCounters.clear();
	gRegions.clear();

	gLocations.shrink_to_fit();
	gCounters.shrink_to_fit();
	gRegions.shrink_to_fit();

	// note that we must free memory above before restoring allocator below
	gConfig = DefaultConfiguration();

	return ysErrorCode::Success;
}

YS_API ysErrorCode YS_CALL _ysStartProfile()
{
	std::unique_lock<std::mutex> lock(gMutex);

	if (!gInitialized)
		return ysErrorCode::Uninitialized;

	if (gCapturing)
		return ysErrorCode::AlreadyCapturing;

	gActiveSink = gConfig.sink;

	// notify the sink about all the currently known locations, counters, and regions
	for (auto const& loc : gLocations)
		gActiveSink->AddLocation(ysLocationId(&loc - gLocations.data() + 1), loc.file, loc.line, loc.func);

	for (auto const& name : gCounters)
		gActiveSink->AddCounter(ysCounterId(&name - gCounters.data() + 1), name);

	for (auto const& name : gRegions)
		gActiveSink->AddRegion(ysRegionId(&name - gRegions.data() + 1), name);

	return ysErrorCode::Success;
}

YS_API ysErrorCode YS_CALL _ysStopProfile()
{
	std::unique_lock<std::mutex> lock(gMutex);

	if (!gCapturing)
		return ysErrorCode::NotCapturing;

	gActiveSink = &gNullSink;
	return ysErrorCode::Success;
}

YS_API void* YS_CALL _ysAlloc(std::size_t size)
{
	return gConfig.allocator(nullptr, size);
}

YS_API void YS_CALL _ysFree(void* ptr)
{
	gConfig.allocator(ptr, 0);
}

YS_API ysLocationId YS_CALL _ysAddLocation(char const* fileName, int line, char const* functionName)
{
	std::unique_lock<std::mutex> lock(gMutex);

	YS_ASSERT(gInitialized, "Cannot register a location without initializing Yardstick first");

	Location const location{ fileName, functionName, line };

	size_t const index = FindValue(gLocations.data(), gLocations.data() + gLocations.size(), location);
	auto const id = ysLocationId(index + 1);
	if (index < gLocations.size())
		return id;

	gLocations.push_back(location);

	gActiveSink->AddLocation(id, fileName, line, functionName);

	return id;
}

YS_API ysCounterId YS_CALL _ysAddCounter(const char* counterName)
{
	std::unique_lock<std::mutex> lock(gMutex);

	YS_ASSERT(gInitialized, "Cannot register a counter without initializing Yardstick first");

	// this may be a duplicate; return the existing one if so
	size_t const index = FindIf(gCounters.data(), gCounters.data() + gCounters.size(), [=](char const* str){ return std::strcmp(str, counterName) == 0; });
	auto const id = ysCounterId(index + 1);
	if (index < gCounters.size())
		return id;

	gCounters.push_back(counterName);

	gActiveSink->AddCounter(id, counterName);
	
	return id;
}

YS_API ysRegionId YS_CALL _ysAddRegion(const char* zoneName)
{
	std::unique_lock<std::mutex> lock(gMutex);

	YS_ASSERT(gInitialized, "Cannot register a region without initializing Yardstick first");

	// this may be a duplicate; return the existing one if so
	size_t const index = FindIf(gRegions.data(), gRegions.data() + gRegions.size(), [=](char const* str){ return std::strcmp(str, zoneName) == 0; });
	auto const id = ysRegionId(index + 1);
	if (index < gRegions.size())
		return id;

	gRegions.push_back(zoneName);

	gActiveSink->AddRegion(id, zoneName);

	return id;
}

YS_API void YS_CALL _ysIncrementCounter(ysCounterId counterId, ysLocationId locationId, double amount)
{
	auto const now = gConfig.readClock();
	gActiveSink->IncrementCounter(counterId, locationId, now, amount);
}

YS_API void YS_CALL _ysEmitRegion(ysRegionId regionId, ysLocationId locationId, ysTime startTime, ysTime endTime)
{
	gActiveSink->EmitRegion(regionId, locationId, startTime, endTime);
}

YS_API ysTime YS_CALL _ysReadClock()
{
	return gConfig.readClock();
}

YS_API ysErrorCode YS_CALL _ysTick()
{
	// can't tick without a context
	if (!gInitialized)
		return ysErrorCode::Uninitialized;

	auto const now = gConfig.readClock();

	gActiveSink->Tick(now);

	return ysErrorCode::Success;
}

#endif // !defined(NO_YS)
