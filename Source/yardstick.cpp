// Copyright (C) 2014 Sean Middleditch, all rights reserverd.

#include "yardstick.h"
#include "yardstick.hpp"

#include <cstring>
#include <vector>
#include <tuple>
#include <algorithm>

// we need this on Windows, and since there's tricks to including it minimally (not yet applied), just do it here
#if defined(_WIN32)
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#endif

namespace
{
	/// Definition of a location.
	/// \internal
	using Location = std::tuple<char const*, char const*, int>;

	/// Default realloc() wrapper.
	/// \internal
	void* YS_CALL DefaultAllocator(void* block, size_t bytes);

	/// Default clock tick reader.
	/// \internal
	ys_clock_t YS_CALL DefaultReadClockTicks();

	/// Default clock frequency reader.
	/// \internal
	ys_clock_t YS_CALL DefaultReadClockFrequency();

	void YS_CALL DefaultSinkStart(void* userData, ys_clock_t clockNow, ys_clock_t clockFrequency) {}
	void YS_CALL DefaultSinkStop(void* userData, ys_clock_t clockNow) {}
	void YS_CALL DefaultSinkAddLocation(void* userData, uint16_t locatonId, char const* fileName, int line, char const* functionName) {}
	void YS_CALL DefaultSinkAddCounter(void* userData, uint16_t counterId, char const* counterName) {}
	void YS_CALL DefaultSinkAddZone(void* userData, uint16_t zoneId, char const* zoneName) {}
	void YS_CALL DefaultSinkIncrementCounter(void* userData, uint16_t counterId, uint16_t locationId, uint64_t clockNow, double value) {}
	void YS_CALL DefaultSinkEnterZone(void* userData, uint16_t zoneId, uint16_t locationId, ys_clock_t clockNow, uint16_t depth) {}
	void YS_CALL DefaultSinkExitZone(void* userData, uint16_t zoneId, ys_clock_t clockStart, uint64_t clockElapsed, uint16_t depth) {}
	void YS_CALL DefaultSinkTick(void* userData, ys_clock_t clockNow) {}

	/// Allocator using the main context.
	/// \internal
	template <typename T>
	struct YsAllocator
	{
		using value_type = T;

		T* allocate(std::size_t bytes);
		void deallocate(T* block, std::size_t);
	};


	/// Sink wrapper for C++ interfaces.
	class SinkWrapper final : public ys::ISink
	{
		void* m_UserData = nullptr;
		ys_sink_t m_Sink;

	public:
		SinkWrapper(void* userData, ys_sink_t const& sink);
		SinkWrapper(SinkWrapper const& src);
		SinkWrapper& operator=(SinkWrapper const& src);

		void const* GetUserData() const;

		void Start(ys_clock_t clockNow, ys_clock_t clockFrequency) override;
		void Stop(ys_clock_t clockNow) override;
		void AddLocation(uint16_t locatonId, char const* fileName, int line, char const* functionName) override;
		void AddCounter(uint16_t counterId, char const* counterName) override;
		void AddZone(uint16_t zoneId, char const* zoneName) override;
		void IncrementCounter(uint16_t counterId, uint16_t locationId, uint64_t clockNow, double value) override;
		void EnterZone(uint16_t zoneId, uint16_t locationId, ys_clock_t clockNow, uint16_t depth) override;
		void ExitZone(uint16_t zoneId, ys_clock_t clockStart, uint64_t clockElapsed, uint16_t depth) override;
		void Tick(ys_clock_t clockNow) override;
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
		std::vector<std::pair<ys_id_t, ys_clock_t>, YsAllocator<std::pair<ys_id_t, ys_clock_t>>> zoneStack;
		std::vector<ys::ISink*, YsAllocator<ys::ISink*>> sinks;
		std::vector<SinkWrapper, YsAllocator<SinkWrapper>> sinkWrappers;
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

	SinkWrapper::SinkWrapper(void* userData, ys_sink_t const& sink) : m_UserData(userData), m_Sink(sink) {}

	SinkWrapper::SinkWrapper(SinkWrapper const& src) : m_UserData(src.m_UserData), m_Sink(src.m_Sink) {}

	SinkWrapper& SinkWrapper::operator=(SinkWrapper const& src)
	{
		if (this != &src)
		{
			m_UserData = src.m_UserData;
			m_Sink = src.m_Sink;
		}
		return *this;
	}

	void const* SinkWrapper::GetUserData() const { return m_UserData; }
	void SinkWrapper::Start(ys_clock_t clockNow, ys_clock_t clockFrequency) { return m_Sink.start(m_UserData, clockNow, clockFrequency); }
	void SinkWrapper::Stop(ys_clock_t clockNow) { return m_Sink.stop(m_UserData, clockNow); }
	void SinkWrapper::AddLocation(uint16_t locatonId, char const* fileName, int line, char const* functionName) { return m_Sink.add_location(m_UserData, locatonId, fileName, line, functionName); }
	void SinkWrapper::AddCounter(uint16_t counterId, char const* counterName) { return m_Sink.add_counter(m_UserData, counterId, counterName); }
	void SinkWrapper::AddZone(uint16_t zoneId, char const* zoneName) { return m_Sink.add_zone(m_UserData, zoneId, zoneName); }
	void SinkWrapper::IncrementCounter(uint16_t counterId, uint16_t locationId, uint64_t clockNow, double value) { return m_Sink.increment_counter(m_UserData, counterId, locationId, clockNow, value); }
	void SinkWrapper::EnterZone(uint16_t zoneId, uint16_t locationId, ys_clock_t clockNow, uint16_t depth) { return m_Sink.enter_zone(m_UserData, zoneId, locationId, clockNow, depth); }
	void SinkWrapper::ExitZone(uint16_t zoneId, ys_clock_t clockStart, uint64_t clockElapsed, uint16_t depth) { return m_Sink.exit_zone(m_UserData, zoneId, clockStart, clockElapsed, depth); }
	void SinkWrapper::Tick(ys_clock_t clockNow) { return m_Sink.tick(m_UserData, clockNow); }
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
	ctx->sinkWrappers.clear();

	ctx->locations.shrink_to_fit();
	ctx->counters.shrink_to_fit();
	ctx->zones.shrink_to_fit();
	ctx->zoneStack.shrink_to_fit();
	ctx->sinks.shrink_to_fit();
	ctx->sinkWrappers.shrink_to_fit();

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

	// tell all the sinks about the new counter
	for (auto sink : gContext->sinks)
		if (sink != nullptr)
			sink->AddLocation(id, fileName, line, functionName);

	return id;
}

YS_API ys_id_t YS_CALL _ys_add_counter(const char* counterName)
{
	// this may be a duplicate; return the existing one if so
	size_t const index = std::find_if(begin(gContext->counters), end(gContext->counters), [=](char const* str){ return std::strcmp(str, counterName) == 0; }) - begin(gContext->counters);
	if (index < gContext->counters.size())
		return static_cast<ys_id_t>(index);

	gContext->counters.push_back(counterName);
	ys_id_t const id = static_cast<ys_id_t>(index);

	// tell all the sinks about the new counter
	for (auto sink : gContext->sinks)
		sink->AddCounter(id, counterName);

	return id;
}

YS_API ys_id_t YS_CALL _ys_add_zone(const char* zoneName)
{
	// this may be a duplicate; return the existing one if so
	size_t const index = std::find_if(begin(gContext->zones), end(gContext->zones), [=](char const* str){ return std::strcmp(str, zoneName) == 0; }) - begin(gContext->zones);
	if (index < gContext->zones.size())
		return static_cast<ys_id_t>(index);

	gContext->zones.push_back(zoneName);
	ys_id_t const id = static_cast<ys_id_t>(index);

	// tell all the sinks about the new counter
	for (auto sink : gContext->sinks)
		sink->AddZone(id, zoneName);

	return id;
}

YS_API void YS_CALL _ys_increment_counter(ys_id_t counterId, ys_id_t locationId, double amount)
{
	auto const now = gContext->ReadClockTicks();

	for (auto sink : gContext->sinks)
		if (sink != nullptr)
			sink->IncrementCounter(counterId, locationId, now, amount);
}

YS_API void YS_CALL _ys_enter_zone(ys_id_t zoneId, ys_id_t locationId)
{
	auto const now = gContext->ReadClockTicks();
	auto const depth = static_cast<ys_id_t>(gContext->zoneStack.size());

	gContext->zoneStack.emplace_back(zoneId, now);

	for (auto sink : gContext->sinks)
		sink->EnterZone(zoneId, locationId, now, depth);
}

YS_API void YS_CALL _ys_exit_zone()
{
	auto const now = gContext->ReadClockTicks();

	auto const& entry = gContext->zoneStack.back();
	auto const zoneId = entry.first;
	auto const start = entry.second;
	auto const elapsed = now - start;

	gContext->zoneStack.pop_back();
	auto const depth = static_cast<uint16_t>(gContext->zoneStack.size());

	for (auto sink : gContext->sinks)
		sink->ExitZone(zoneId, start, elapsed, depth);
}

YS_API void YS_CALL _ys_tick()
{
	auto const now = gContext->ReadClockTicks();

	for (auto sink : gContext->sinks)
		sink->Tick(now);
}

YS_API ys_error_t YS_CALL ys_add_sink(void* userData, ys_sink_t const* sink, size_t size)
{
	if (sink == nullptr)
		return YS_INVALID_PARAMETER;

	if (size != sizeof(ys_sink_t))
		return YS_INVALID_PARAMETER;

	// initialize any missing sink functions to do-nothing callbacks; avoids branches at runtime
	// handle incomplete sink structures for users who compiled aginst old versions of the library
	ys_sink_t tmp = YS_DEFAULT_SINK;
	std::memcpy(&tmp, sink, size);

	if (tmp.start == nullptr)
		tmp.start = DefaultSinkStart;
	if (tmp.stop == nullptr)
		tmp.stop = DefaultSinkStop;
	if (tmp.add_location == nullptr)
		tmp.add_location = DefaultSinkAddLocation;
	if (tmp.add_counter == nullptr)
		tmp.add_counter = DefaultSinkAddCounter;
	if (tmp.add_zone == nullptr)
		tmp.add_zone = DefaultSinkAddZone;
	if (tmp.increment_counter == nullptr)
		tmp.increment_counter = DefaultSinkIncrementCounter;
	if (tmp.enter_zone == nullptr)
		tmp.enter_zone = DefaultSinkEnterZone;
	if (tmp.exit_zone == nullptr)
		tmp.exit_zone = DefaultSinkExitZone;
	if (tmp.tick == nullptr)
		tmp.tick = DefaultSinkTick;

	// register the wrapper
	gContext->sinkWrappers.emplace_back(SinkWrapper(userData, tmp));

	return YS_OK;
}

YS_API void YS_CALL ys_remove_sink(void const* userData)
{
	// find the sink to remove it
	auto it = std::find_if(begin(gContext->sinkWrappers), end(gContext->sinkWrappers), [=](SinkWrapper const& sink){ return sink.GetUserData() == userData; });
	if (it == end(gContext->sinkWrappers))
		return;

	SinkWrapper tmp = *it;
	gContext->sinkWrappers.erase(it);

	tmp.Stop(gContext->ReadClockTicks());
}

} // extern "C"

YS_API ys_error_t YS_CALL ys::AddSink(ISink* sink)
{
	if (sink == nullptr)
		return YS_INVALID_PARAMETER;

	gContext->sinks.push_back(sink);
	
	// tell the sink about existing counters and zones
	for (auto const& counter : gContext->counters)
		sink->AddCounter(static_cast<uint16_t>(&counter - gContext->counters.data()), counter);
	for (auto const& zone : gContext->zones)
		sink->AddCounter(static_cast<uint16_t>(&zone - gContext->zones.data()), zone);

	return YS_OK;
}

YS_API void YS_CALL ys::RemoveSink(ISink const* sink)
{
	if (sink == nullptr)
		return;

	auto it = std::find(begin(gContext->sinks), end(gContext->sinks), sink);
	if (it == end(gContext->sinks))
		return;

	ISink* tmp = *it;
	gContext->sinks.erase(it);

	tmp->Stop(gContext->ReadClockTicks());
}
