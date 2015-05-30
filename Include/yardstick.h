/* Copyright (C) 2014 Sean Middleditch, all rights reserverd. */

#if !defined(YARDSTICK_H)
#define YARDSTICK_H

#include <cstddef>
#include <cstdint>

// ---- Support Code ----

/// Configuration macros necessary for shared library exports.
#if defined(_WIN32)
#	define YS_CALL __cdecl
#	if defined(YARDSTICK_STATIC)
#		define YS_API extern
#	elif defined(YARDSTICK_EXPORT)
#		define YS_API __declspec(dllexport)
#	else
#		define YS_API __declspec(dllimport)
#	endif
#else
#	error "Unsupported platform"
#endif

/// \brief Helper needed to concatenate string.
#define YS_CAT2(a, b) a##b
#define YS_CAT(a, b) YS_CAT2(a,b)

// ---- Types ----

namespace ys
{
	/// \brief Type returned by the high-resolution timer.
	using Clock = std::uint64_t;

	/// \brief Unique handle to a location.
	enum class LocationId : std::uint32_t {};

	/// \brief Unique handle to a zone.
	enum class ZoneId : std::uint16_t {};

	/// \brief Unique handle to a counter.
	enum class CounterId : std::uint16_t {};

	/// \brief Reads the high-resolution timer.
	/// \returns The current timer value, akin to x86 rtdsc.
	using ReadTicksCallback = Clock(YS_CALL*)();

	/// \brief Reads the high-resolution timer's frequency.
	/// \returns The frequency, akin to x86's rtdsc frequency.
	using ReadFrequencyCallback = Clock(YS_CALL*)();

	/// \brief Memory allocation callback.
	/// Follows the rules of realloc(), except that it will only be used to allocate or free.
	using AllocatorCallback = void*(YS_CALL*)(void* block, std::size_t bytes);

	/// \brief Error return codes.
	enum class ErrorCode : std::uint8_t
	{
		/// \brief Success result.
		Success,
		/// \brief One or more parameters contain invalid values.
		InvalidParameter,
		/// \brief Memory or disk space was required but resources are exhausted.
		ResourcesExhausted,
		/// \brief A failure was detected but the cause is not known.
		UnknownError,
		/// \brief The user attempted to do something more than once that can only be done once (like initialize the library).
		Duplicate,
		/// \brief A system error occurred and the system error reporting facilities may contain more information.
		SystemFailure,
		/// \brief Yardstick was not initialized before API function call.
		UninitializedLibrary,
		/// \brief Yardstick support has been disabled.
		DisabledLibrary
	};

	/// \brief An event sink.
	class ISink
	{
	protected:
		ISink() = default;

	public:
		virtual ~ISink() = default;

		ISink(ISink const&) = delete;
		ISink& operator=(ISink const&) = delete;

		virtual void YS_CALL Start(Clock clockNow, Clock clockFrequency) = 0;
		virtual void YS_CALL Stop(Clock clockNow) = 0;
		virtual void YS_CALL AddLocation(LocationId locatonId, char const* fileName, int line, char const* functionName) = 0;
		virtual void YS_CALL AddCounter(CounterId counterId, char const* counterName) = 0;
		virtual void YS_CALL AddZone(ZoneId zoneId, char const* zoneName) = 0;
		virtual void YS_CALL IncrementCounter(CounterId counterId, LocationId locationId, uint64_t clockNow, double value) = 0;
		virtual void YS_CALL EnterZone(ZoneId zoneId, LocationId locationId, Clock clockNow, uint16_t depth) = 0;
		virtual void YS_CALL ExitZone(ZoneId zoneId, Clock clockStart, uint64_t clockElapsed, uint16_t depth) = 0;
		virtual void YS_CALL Tick(Clock clockNow) = 0;
	};
} // namespace ys

#if !defined(NO_YS)

// ---- Functions ----

namespace _ys_internal
{
	using namespace ys;

	/// \brief Initializes the Yardstick library.
	/// Must be called before any other Yardstick function.
	/// \param allocator An allocation function (must not be nullptr).
	/// \param readClock Function to read a clock value from the system.
	/// \param readFrequency Function to read a clock frequency from the system.
	/// \returns YS_OK on success, or another value on error.
	YS_API ErrorCode YS_CALL Initialize(AllocatorCallback allocator, ReadTicksCallback readClock, ReadFrequencyCallback readFrequency);

	/// \brief Shuts down the Yardstick library and frees any resources.
	/// Yardstick functions cannot be called after this point without reinitializing it.
	YS_API void YS_CALL Shutdown();

	/// \brief Registers a new sink.
	/// \param sink The sink that is invoked on events.
	/// \return YS_OK on success, or another value on error.
	YS_API ErrorCode YS_CALL AddSink(ISink* sink);

	/// \brief Removes a registerd sink.
	/// \param sink The sink that is invoked on events.
	YS_API ErrorCode YS_CALL RemoveSink(ISink* sink);

	/// \brief Call once per frame.
	YS_API ErrorCode YS_CALL Tick();

	/// \brief Registers a location to be used for future calls.
	/// \internal
	YS_API LocationId YS_CALL AddLocation(char const* fileName, int line, char const* functionName);

	/// \brief Registers a counter to be used for future calls.
	/// \internal
	YS_API CounterId YS_CALL AddCounter(const char* counterName);

	/// \brief Registers a zone to be used for future calls.
	/// \internal
	YS_API ZoneId YS_CALL AddZone(const char* zoneName);

	/// \brief Adds a value to a counter.
	/// \internal
	YS_API void YS_CALL IncrementCounter(CounterId counterId, LocationId locationId, double amount);

	/// \brief Managed a scoped zone.
	/// \internal
	struct ScopedProfileZone final
	{
		YS_API ScopedProfileZone(ZoneId zoneId, LocationId locationId);
		YS_API ~ScopedProfileZone();

		ScopedProfileZone(ScopedProfileZone const&) = delete;
		ScopedProfileZone& operator=(ScopedProfileZone const&) = delete;
	};
} // namespace _ys_internal

#endif // !defined(NO_YS)

// ---- Public Interface ----

#if !defined(NO_YS)
#	define ysInitialize(allocator, readClock, readFrequency) (::_ys_internal::Initialize((allocator), (readClock), (readFrequency)))
#	define ysShutdown() (::_ys_internal::Shutdown())
#	define ysAddSink(sink) (::_ys_internal::AddSink((sink)))
#	define ysRemoveSink(sink) (::_ys_internal::RemoveSink((sink)))
#	define ysTick() (::_ys_internal::Tick())
#else // !defined(NO_YS)
#	define ysInitialize(allocator, readClock, readFrequency) (ErrorCode::DisabledLibrary)
#	define ysShutdown() (ErrorCode::DisabledLibrary)
#	define ysAddSink(sink) (ErrorCode::DisabledLibrary)
#	define ysRemoveSink(sink) (ErrorCode::DisabledLibrary)
#	define ysTick() (ErrorCode::DisabledLibrary)
#endif // !defined(NO_YS)

// ---- Macros ----

#if !defined(NO_YS)

/// \brief Marks the current scope as being in a zone, and automatically closes the zone at the end of the scope.
#	define ysZone(name) \
	static auto const YS_CAT(_ys_zone_id, __LINE__) = ::_ys_internal::AddZone(("" name)); \
	static auto const YS_CAT(_ys_location_id, __LINE__) = ::_ys_internal::AddLocation(__FILE__, __LINE__, __FUNCTION__); \
	::_ys_internal::ScopedProfileZone YS_CAT(_ys_zone, __LINE__)(YS_CAT(_ys_zone_id, __LINE__), YS_CAT(_ys_location_id, __LINE__))

#	define ysCount(name, amount) \
	do{ \
		static auto const YS_CAT(_ys_counter_id, __LINE__) = ::_ys_internal::AddCounter(("" name)); \
		static auto const YS_CAT(_ys_location_id, __LINE__) = ::_ys_internal::AddLocation(__FILE__, __LINE__, __FUNCTION__); \
		::_ys_internal::IncrementCounter(YS_CAT(_ys_counter_id, __LINE__), YS_CAT(_ys_location_id, __LINE__), (amount)); \
	}while(false)


#else // !defined(NO_YS)

#	define ysZone(name) do{}while(false)
#	define ysCount(name, amount) do{}while(false)

#endif // !defined(NO_YS)

#endif // YARDSTICK_H
