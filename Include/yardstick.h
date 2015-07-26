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

/// Helper needed to concatenate string.
#define YS_CAT2(a, b) a##b
#define YS_CAT(a, b) YS_CAT2(a,b)

// ---- Public API ----

/// Type returned by the high-resolution timer.
using ysTime = std::uint64_t;

/// Unique handle to a location.
enum class ysLocationId : std::uint32_t { None = 0 };

/// Unique handle to a zone.
enum class ysZoneId : std::uint16_t { None = 0 };

/// Unique handle to a counter.
enum class ysCounterId : std::uint16_t { None = 0 };

/// Reads the high-resolution timer.
/// @returns The current timer value, akin to x86 rtdsc.
using ysReadClock = ysTime(YS_CALL*)();

/// Reads the high-resolution timer's frequency.
/// @returns The frequency, akin to x86's rtdsc frequency.
using ysReadFrequency = ysTime(YS_CALL*)();

/// Memory allocation callback.
/// Follows the rules of realloc(), except that it will only be used to allocate or free.
using ysAllocator = void*(YS_CALL*)(void* block, std::size_t bytes);

/// Error return codes.
enum class ysErrorCode : std::uint8_t
{
	/// Success result.
	Success,
	/// One or more parameters contain invalid values.
	InvalidParameter,
	/// Memory or disk space was required but resources are exhausted.
	ResourcesExhausted,
	/// A failure was detected but the cause is not known.
	UnknownError,
	/// The user attempted to do something more than once that can only be done once (like initialize the library).
	Duplicate,
	/// A system error occurred and the system error reporting facilities may contain more information.
	SystemFailure,
	/// Yardstick was not initialized before API function call.
	UninitializedLibrary,
	/// Yardstick support has been disabled.
	DisabledLibrary
};

/// An event sink.
class ysSink
{
protected:
	ysSink() = default;

public:
	virtual ~ysSink() = default;

	ysSink(ysSink const&) = delete;
	ysSink& operator=(ysSink const&) = delete;

	virtual void YS_CALL BeginProfile(ysTime clockNow, ysTime clockFrequency) = 0;
	virtual void YS_CALL EndProfile(ysTime clockNow) = 0;

	virtual void YS_CALL AddLocation(ysLocationId locatonId, char const* fileName, int line, char const* functionName) = 0;
	virtual void YS_CALL AddCounter(ysCounterId counterId, char const* counterName) = 0;
	virtual void YS_CALL AddZone(ysZoneId zoneId, char const* zoneName) = 0;

	virtual void YS_CALL IncrementCounter(ysCounterId counterId, ysLocationId locationId, ysTime clockNow, double value) = 0;

	virtual void YS_CALL RecordZone(ysZoneId zoneId, ysLocationId, ysTime clockStart, ysTime clockEnd) = 0;

	virtual void YS_CALL Tick(ysTime clockNow) = 0;
};

/// Configuration object used to initialize the library.
struct ysConfiguration
{
	YS_API ysConfiguration();

	ysTime(YS_CALL* readClock)();
	ysTime(YS_CALL* readFrequency)();
	void*(YS_CALL* allocator)(void* block, std::size_t bytes);
	ysSink* sink;
};

// ---- Public Macros ----

#if !defined(NO_YS)

#	define ysInitialize(config) (::_ysInitialize((config)))
#	define ysShutdown() (::_ysShutdown())
#	define ysTick() (::_ysTick())
#	define ysAlloc(size) (::_ysAlloc((size)))
#	define ysFree(ptr) (::_ysFree((ptr)))

  /// Marks the current scope as being in a zone, and automatically closes the zone at the end of the scope.
#	define ysZone(name) \
	static auto const YS_CAT(_ys_zone_id, __LINE__) = ::_ysAddZone(("" name)); \
	static auto const YS_CAT(_ys_location_id, __LINE__) = ::_ysAddLocation(__FILE__, __LINE__, __FUNCTION__); \
	::_ysScopedProfileZone YS_CAT(_ys_zone, __LINE__)(YS_CAT(_ys_zone_id, __LINE__), YS_CAT(_ys_location_id, __LINE__))

#	define ysCount(name, amount) \
	do{ \
		static auto const YS_CAT(_ys_counter_id, __LINE__) = ::_ysAddCounter(("" name)); \
		static auto const YS_CAT(_ys_location_id, __LINE__) = ::_ysAddLocation(__FILE__, __LINE__, __FUNCTION__); \
		::_ysIncrementCounter(YS_CAT(_ys_counter_id, __LINE__), YS_CAT(_ys_location_id, __LINE__), (amount)); \
	}while(false)

#else // !defined(NO_YS)

#	define ysInitialize(config) (::ys::ErrorCode::DisabledLibrary)
#	define ysShutdown() (::ys::ErrorCode::DisabledLibrary)
#	define ysTick() (::ys::ErrorCode::DisabledLibrary)
#	define ysAlloc(size) (nullptr)
#	define ysFree(ptr) do{}while(false)
#	define ysZone(name) do{}while(false)
#	define ysCount(name, amount) do{}while(false)

#endif // !defined(NO_YS)

#if !defined(NO_YS)

// ---- Private Implementation ----

/// Initializes the Yardstick library.
/// Must be called before any other Yardstick function.
/// @param config Configuration values for the context.
/// @param sink The sink where all events will be written.
/// @param readClock Function to read a clock value from the system.
/// @param readFrequency Function to read a clock frequency from the system.
/// @returns YS_OK on success, or another value on error.
YS_API ysErrorCode YS_CALL _ysInitialize(ysConfiguration const& config);

/// Shuts down the Yardstick library and frees any resources.
/// Yardstick functions cannot be called after this point without reinitializing it.
YS_API void YS_CALL _ysShutdown();

/// Use the configured allocator to retrieve memory.
/// @param size Size in bytes of memory to allocate.
/// @internal
YS_API void* YS_CALL _ysAlloc(std::size_t size);

/// Use the configured allocator to free memory.
/// @param ptr Pointer to memory to free
/// @internal
YS_API void* YS_CALL _ysFree(void* ptr);

/// Call once per frame.
YS_API ysErrorCode YS_CALL _ysTick();

/// Registers a location to be used for future calls.
/// @internal
YS_API ysLocationId YS_CALL _ysAddLocation(char const* fileName, int line, char const* functionName);

/// Registers a counter to be used for future calls.
/// @internal
YS_API ysCounterId YS_CALL _ysAddCounter(const char* counterName);

/// Registers a zone to be used for future calls.
/// @internal
YS_API ysZoneId YS_CALL _ysAddZone(const char* zoneName);

/// Adds a value to a counter.
/// @internal
YS_API void YS_CALL _ysIncrementCounter(ysCounterId counterId, ysLocationId locationId, double amount);

/// Managed a scoped zone.
/// @internal
struct _ysScopedProfileZone final
{
	YS_API _ysScopedProfileZone(ysZoneId zoneId, ysLocationId locationId);
	YS_API ~_ysScopedProfileZone();

	_ysScopedProfileZone(_ysScopedProfileZone const&) = delete;
	_ysScopedProfileZone& operator=(_ysScopedProfileZone const&) = delete;

	ysTime startTime;
	ysZoneId zoneId;
	ysLocationId locationId;
};

#endif // !defined(NO_YS)

#endif // YARDSTICK_H
