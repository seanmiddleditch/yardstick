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

/// \brief Type returned by the high-resolution timer.
using ysClockT = std::uint64_t;

/// \brief Type of any unique identifier. -1 is an invalid value.
using ysIdT = std::int16_t;

/// \brief Reads the high-resolution timer.
/// \returns The current timer value, akin to x86 rtdsc.
using ysReadTicksCB = ysClockT(YS_CALL*)();

/// \brief Reads the high-resolution timer's frequency.
/// \returns The frequency, akin to x86's rtdsc frequency.
using ysReadFrequencyCB = ysClockT(YS_CALL*)();

/// \brief Memory allocation callback.
/// Follows the rules of realloc(), except that it will only be used to allocate or free.
using ysAllocatorCB = void*(YS_CALL*)(void* block, std::size_t bytes);

/// \brief Error return codes.
enum class ysError : std::uint8_t
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
class ysISink
{
protected:
	ysISink() = default;

public:
	virtual ~ysISink() = default;

	ysISink(ysISink const&) = delete;
	ysISink& operator=(ysISink const&) = delete;

	virtual void YS_CALL Start(ysClockT clockNow, ysClockT clockFrequency) = 0;
	virtual void YS_CALL Stop(ysClockT clockNow) = 0;
	virtual void YS_CALL AddLocation(uint16_t locatonId, char const* fileName, int line, char const* functionName) = 0;
	virtual void YS_CALL AddCounter(uint16_t counterId, char const* counterName) = 0;
	virtual void YS_CALL AddZone(uint16_t zoneId, char const* zoneName) = 0;
	virtual void YS_CALL IncrementCounter(uint16_t counterId, uint16_t locationId, uint64_t clockNow, double value) = 0;
	virtual void YS_CALL EnterZone(uint16_t zoneId, uint16_t locationId, ysClockT clockNow, uint16_t depth) = 0;
	virtual void YS_CALL ExitZone(uint16_t zoneId, ysClockT clockStart, uint64_t clockElapsed, uint16_t depth) = 0;
	virtual void YS_CALL Tick(ysClockT clockNow) = 0;
};

#if !defined(NO_YS)

// ---- Functions ----

/// \brief Initializes the Yardstick library.
/// Must be called before any other Yardstick function.
/// \param allocator An allocation function (must not be nullptr).
/// \param readClock Function to read a clock value from the system.
/// \param readFrequency Function to read a clock frequency from the system.
/// \returns YS_OK on success, or another value on error.
YS_API ysError YS_CALL ysInitialize(ysAllocatorCB allocator, ysReadTicksCB readClock, ysReadFrequencyCB readFrequency);

/// \brief Shuts down the Yardstick library and frees any resources.
/// Yardstick functions cannot be called after this point without reinitializing it.
YS_API void YS_CALL ysShutdown();

/// \brief Registers a new sink.
/// \param sink The sink that is invoked on events.
/// \return YS_OK on success, or another value on error.
YS_API ysError YS_CALL ysAddSink(ysISink* sink);

/// \brief Removes a registerd sink.
/// \param sink The sink that is invoked on events.
YS_API ysError YS_CALL ysRemoveSink(ysISink* sink);

/// \brief Call once per frame.
YS_API ysError YS_CALL ysTick();

/// \brief Internal helper functions and classes.
/// \internal
namespace _ys__detail {

/// \brief Registers a location to be used for future calls.
/// \internal
YS_API ysIdT YS_CALL _addLocation(char const* fileName, int line, char const* functionName);

/// \brief Registers a counter to be used for future calls.
/// \internal
YS_API ysIdT YS_CALL _addCounter(const char* counterName);

/// \brief Registers a zone to be used for future calls.
/// \internal
YS_API ysIdT YS_CALL _addZone(const char* zoneName);

/// \brief Adds a value to a counter.
/// \internal
YS_API void YS_CALL _incrementCounter(ysIdT counterId, ysIdT locationId, double amount);

/// \brief Managed a scoped zone.
/// \internal
struct _scopedZone final
{
	YS_API _scopedZone(ysIdT zoneId, ysIdT locationId);
	YS_API ~_scopedZone();

	_scopedZone(_scopedZone const&) = delete;
	_scopedZone& operator=(_scopedZone const&) = delete;
};

} // namespace ys::_ys__detail

#else // !defined(NO_YS)

#	define ysInitialize(allocator, readClock, readFrequency) (err::DISABLED)
#	define ysShutdown() (err::DISABLED)
#	define ysAddSink(sink) (err::DISABLED)
#	define ysRemoveSink(sink) (err::DISABLED)
#	define ysTick() (err::DISABLED)

#endif // !defined(NO_YS)

// ---- Macros ----

#if !defined(NO_YS)

/// \brief Marks the current scope as being in a zone, and automatically closes the zone at the end of the scope.
#	define ysZone(name) \
	static ysIdT const YS_CAT(_ys_zone_id, __LINE__) = ::_ys__detail::_addZone(("" name)); \
	static ysIdT const YS_CAT(_ys_location_id, __LINE__) = ::_ys__detail::_addLocation(__FILE__, __LINE__, __FUNCTION__); \
	::_ys__detail::_scopedZone YS_CAT(_ys_zone, __LINE__)(YS_CAT(_ys_zone_id, __LINE__), YS_CAT(_ys_location_id, __LINE__))

#	define ysCount(name, amount) \
	do{ \
		static ysIdT const YS_CAT(_ys_counter_id, __LINE__) = ::_ys__detail::_addCounter(("" name)); \
		static ysIdT const YS_CAT(_ys_location_id, __LINE__) = ::_ys__detail::_addLocation(__FILE__, __LINE__, __FUNCTION__); \
		::_ys__detail::_incrementCounter(YS_CAT(_ys_counter_id, __LINE__), YS_CAT(_ys_location_id, __LINE__), (amount)); \
	}while(false)


#else // !defined(NO_YS)

#	define ysZone(name) do{}while(false)
#	define ysCount(name, amount) do{}while(false)

#endif // !defined(NO_YS)

#endif // YARDSTICK_H
