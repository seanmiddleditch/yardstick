// Copyright (C) 2014 Sean Middleditch, all rights reserverd.

#pragma once

#define ENABLE_PROFILER 1

#if defined(ENABLE_PROFILER)

class IProfileSink;

enum class EProfileUnits : uint8_t
{
	Unknown,
	Seconds,
	Bytes,
};

namespace detail
{
	namespace profile
	{
		bool Initialize();
		void Shutdown();

		void AddSink(IProfileSink* sink);
		void RemoveSink(IProfileSink* sink);

		uint16_t RegisterLocation(char const* file, int line, char const* function);

		uint16_t RegisterZone(const char* message);
		void StartZone(uint16_t id, uint16_t loc);
		void StopZone();

		uint16_t RegisterCounter(const char* name, EProfileUnits units);
		void IncrementCounter(uint16_t id, uint16_t loc, double count);

		void Tick();

		class Scope final
		{
		public:
			Scope(uint16_t id, uint16_t loc) { StartZone(id, loc); }
			~Scope() { StopZone(); }

			Scope(Scope const&) = delete;
			Scope& operator=(Scope const&) = delete;
		};
	}
}

#endif
#if defined(ENABLE_PROFILER)

#	define PROFILE_COUNT(name, count, ...) \
	do{ \
		static uint16_t const _profile_counter_id = ::detail::profile::RegisterCounter(("" name), EProfileUnits::Unknown); \
		static uint16_t const _profile_location_id = ::detail::profile::RegisterLocation(__FILE__, __LINE__, __FUNCTION__); \
		::detail::profile::IncrementCounter(_profile_counter_id, _profile_location_id, double(count)); \
	}while(false)

#	define PROFILE_SCOPE(name) \
	static uint16_t const BOOST_PP_CAT(_profile_zone_id, __LINE__) = ::detail::profile::RegisterZone(("" name)); \
	static uint16_t const BOOST_PP_CAT(_profile_location_id, __LINE__) = ::detail::profile::RegisterLocation(__FILE__, __LINE__, __FUNCTION__); \
	::detail::profile::Scope BOOST_PP_CAT(_profile_zone, __LINE__)(BOOST_PP_CAT(_profile_zone_id, __LINE__), BOOST_PP_CAT(_profile_location_id, __LINE__))

#	define PROFILE_TICK() (::detail::profile::Tick())
#	define PROFILE_INITIALIZE() (::detail::profile::Initialize())
#	define PROFILE_SHUTDOWN() (::detail::profile::Shutdown())
#	define PROFILE_ADD_SINK(sink) (::detail::profile::AddSink((sink)))
#	define PROFILE_REMOVE_SINK(sink) (::detail::profile::RemoveSink((sink)))

#else

#	define PROFILE_COUNT(name, count, ...) do{}while(false)
#	define PROFILE_SCOPE(name) do{}while(false)

#	define PROFILE_TICK() ((void)0)
#	define PROFILE_INITIALIZE() (false)
#	define PROFILE_SHUTDOWN() ((void)0)
#	define PROFILE_ADD_SINK(sink) ((void)0)
#	define PROFILE_REMOVE_SINK(sink) ((void)0)

#endif