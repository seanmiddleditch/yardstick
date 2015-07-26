/* Copyright (C) 2014 Sean Middleditch, all rights reserverd. */

#if !defined(YARDSTICK_H)
#define YARDSTICK_H

#include <cstddef>
#include <cstdint>
#include <cassert>

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

#if !defined(YS_ASSERT)
#	define YS_ASSERT(expr, msg) assert((expr) && (msg))
#endif

/// Type returned by the high-resolution timer.
using ysTime = std::uint64_t;

/// Unique handle to a location.
enum class ysLocationId : std::uint32_t { None = 0 };

/// Unique handle to a region.
enum class ysRegionId : std::uint16_t { None = 0 };

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
	/// Memory has been exhausted.
	NoMemory,
	/// A failure was detected but the cause is not known.
	Unknown,
	/// A system error occurred and the system error reporting facilities may contain more information.
	System,
	/// Yardstick was not initialized before API function call.
	Uninitialized,
	/// Yardstick support has been disabled.
	Disabled,
	/// Yardstick is not currently capturing a profile.
	NotCapturing,
	/// Yardstick has already been initialized.
	AlreadyInitialized,
	/// Yardstick is already capturing a profile.
	AlreadyCapturing,
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
	virtual void YS_CALL AddRegion(ysRegionId regionId, char const* regionName) = 0;

	virtual void YS_CALL IncrementCounter(ysCounterId counterId, ysLocationId locationId, ysTime clockNow, double value) = 0;

	virtual void YS_CALL EmitRegion(ysRegionId regionId, ysLocationId, ysTime clockStart, ysTime clockEnd) = 0;

	virtual void YS_CALL Tick(ysTime clockNow) = 0;
};

/// Configuration object used to initialize the library.
struct ysConfiguration
{
	void*(YS_CALL* allocator)(void* block, std::size_t bytes) = nullptr;
	ysTime(YS_CALL* readClock)() = nullptr;
	ysTime(YS_CALL* readFrequency)() = nullptr;
	ysSink* sink = nullptr;
};

// ---- Public Macros ----

#if !defined(NO_YS)

#	define ysInitialize(config) (::_ysInitialize((config)))
#	define ysShutdown() (::_ysShutdown())
#	define ysStartProfile() (::_ysStartProfile())
#	define ysStopProfile() (::_ysStopProfile())
#	define ysTick() (::_ysTick())
#	define ysAlloc(size) (::_ysAlloc((size)))
#	define ysFree(ptr) (::_ysFree((ptr)))

  /// Marks the current scope as being in a region, and automatically closes the region at the end of the scope.
#	define ysProfile(name) \
	static auto const YS_CAT(_ys_region_id, __LINE__) = ::_ysAddRegion(("" name)); \
	static auto const YS_CAT(_ys_location_id, __LINE__) = ::_ysAddLocation(__FILE__, __LINE__, __FUNCTION__); \
	::_ysScopedRegion YS_CAT(_ys_region, __LINE__)(YS_CAT(_ys_region_id, __LINE__), YS_CAT(_ys_location_id, __LINE__))

#	define ysCount(name, amount) \
	do{ \
		static auto const YS_CAT(_ys_counter_id, __LINE__) = ::_ysAddCounter(("" name)); \
		static auto const YS_CAT(_ys_location_id, __LINE__) = ::_ysAddLocation(__FILE__, __LINE__, __FUNCTION__); \
		::_ysIncrementCounter(YS_CAT(_ys_counter_id, __LINE__), YS_CAT(_ys_location_id, __LINE__), (amount)); \
	}while(false)

#else // !defined(NO_YS)

#	define ysInitialize(config) (::ys::ErrorCode::Disabled)
#	define ysShutdown() (::ys::ErrorCode::Disabled)
#	define ysStartProfile() (::ys::ErrorCode::Disabled)
#	define ysStopProfile() (::ys::ErrorCode::Disabled)
#	define ysTick() (::ys::ErrorCode::Disabled)
#	define ysAlloc(size) (nullptr)
#	define ysFree(ptr) (::ys::ErrorCode::Disabled)
#	define ysProfile(name) (::ys::ErrorCode::Disabled)
#	define ysCount(name, amount) (::ys::ErrorCode::Disabled)

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
YS_API ysErrorCode YS_CALL _ysShutdown();

/// Start capturing a profile if we're not already.
YS_API ysErrorCode YS_CALL _ysStartProfile();

/// Stop capturing a profile if one is active.
YS_API ysErrorCode YS_CALL _ysStopProfile();

/// Use the configured allocator to retrieve memory.
/// @param size Size in bytes of memory to allocate.
/// @internal
YS_API void* YS_CALL _ysAlloc(std::size_t size);

/// Use the configured allocator to free memory.
/// @param ptr Pointer to memory to free
/// @internal
YS_API void YS_CALL _ysFree(void* ptr);

/// Call once per frame.
YS_API ysErrorCode YS_CALL _ysTick();

/// Registers a location to be used for future calls.
/// @internal
YS_API ysLocationId YS_CALL _ysAddLocation(char const* fileName, int line, char const* functionName);

/// Registers a counter to be used for future calls.
/// @internal
YS_API ysCounterId YS_CALL _ysAddCounter(const char* counterName);

/// Registers a region to be used for future calls.
/// @internal
YS_API ysRegionId YS_CALL _ysAddRegion(const char* regionName);

/// Adds a value to a counter.
/// @internal
YS_API void YS_CALL _ysIncrementCounter(ysCounterId counterId, ysLocationId locationId, double amount);

/// Emit a region.
/// @internal
YS_API void YS_CALL _ysEmitRegion(ysRegionId regionId, ysLocationId locationId, ysTime startTime, ysTime endTime);

/// Read the current clock value.
/// @internal
YS_API ysTime YS_CALL _ysReadClock();

/// Managed a scoped region.
/// @internal
struct _ysScopedRegion final
{
	__forceinline _ysScopedRegion(ysRegionId regionId, ysLocationId locationId) : _regionId(regionId), _locationId(locationId), _startTime(_ysReadClock()) {}
	__forceinline ~_ysScopedRegion() { _ysEmitRegion(_regionId, _locationId, _startTime, _ysReadClock()); }

	_ysScopedRegion(_ysScopedRegion const&) = delete;
	_ysScopedRegion& operator=(_ysScopedRegion const&) = delete;

	ysRegionId _regionId;
	ysLocationId _locationId;
	ysTime _startTime;
};

#endif // !defined(NO_YS)

#endif // YARDSTICK_H
