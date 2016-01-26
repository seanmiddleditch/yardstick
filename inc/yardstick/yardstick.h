/* Copyright (C) 2014-2016 Sean Middleditch, all rights reserverd. */

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

/// Helper to ignore values
#define YS_IGNORE(x) (sizeof((x)))

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

/// Memory allocation callback.
/// Follows the rules of realloc(), except that it will only be used to allocate or free.
using ysAllocator = void*(YS_CALL*)(void* block, std::size_t bytes);

/// Return codes.
enum class ysResult : std::uint8_t
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

/// Protocol event
struct ysEvent
{
	enum { TypeNone, TypeHeader, TypeTick, TypeRegion, TypeCounter } type;
	union
	{
		struct
		{
			ysTime frequency;
		} header;
		struct
		{
			ysTime when;
		} tick;
		struct
		{
			ysRegionId id;
			ysLocationId loc;
			ysTime begin;
			ysTime end;
		} region;
		struct
		{
			ysCounterId id;
			ysLocationId loc;
			ysTime when;
			double amount;
		} counter;
	};
};

// ---- Public Macros ----

#if !defined(NO_YS)

#	define ysInitialize(config) (::_ys_::initialize((config)))
#	define ysShutdown() (::_ys_::shutdown())
#	define ysTick() (::_ys_::emit_tick())

  /// Marks the current scope as being in a region, and automatically closes the region at the end of the scope.
#	define ysProfile(name) \
		static auto const YS_CAT(_ys_region_id, __LINE__) = ::_ys_::add_region(("" name)); \
		static auto const YS_CAT(_ys_location_id, __LINE__) = ::_ys_::add_location(__FILE__, __LINE__, __FUNCTION__); \
		::_ys_::ScopedRegion YS_CAT(_ys_region, __LINE__)(YS_CAT(_ys_region_id, __LINE__), YS_CAT(_ys_location_id, __LINE__))

#	define ysCount(name, amount) \
		do{ \
			static auto const YS_CAT(_ys_counter_id, __LINE__) = ::_ys_::add_counter(("" name)); \
			static auto const YS_CAT(_ys_location_id, __LINE__) = ::_ys_::add_location(__FILE__, __LINE__, __FUNCTION__); \
			::_ys_::emit_counter_add(YS_CAT(_ys_counter_id, __LINE__), YS_CAT(_ys_location_id, __LINE__), (amount)); \
		}while(false)

#else // !defined(NO_YS)

#	define ysInitialize(allocator) (YS_IGNORE((allocator)),::ys::ErrorCode::Disabled)
#	define ysShutdown() (::ys::ErrorCode::Disabled)
#	define ysTick() (::ys::ErrorCode::Disabled)
#	define ysProfile(name) do{YS_IGNORE((name));}while(false)
#	define ysCount(name, amount) do{YS_IGNORE((name));YS_IGNORE((amount));}while(false)

#endif // !defined(NO_YS)

#if !defined(NO_YS)

// ---- Private Implementation ----

namespace _ys_
{

	/// Initializes the Yardstick library.
	/// Must be called before any other Yardstick function.
	/// @param allocator Custom allocator to override the default.
	/// @returns YS_OK on success, or another value on error.
	YS_API ysResult YS_CALL initialize(ysAllocator allocator);

	/// Shuts down the Yardstick library and frees any resources.
	/// Yardstick functions cannot be called after this point without reinitializing it.
	YS_API ysResult YS_CALL shutdown();

	/// Call once per frame.
	YS_API ysResult YS_CALL emit_tick();

	/// Registers a location to be used for future calls.
	/// @internal
	YS_API ysLocationId YS_CALL add_location(char const* fileName, int line, char const* functionName);

	/// Registers a counter to be used for future calls.
	/// @internal
	YS_API ysCounterId YS_CALL add_counter(const char* counterName);

	/// Registers a region to be used for future calls.
	/// @internal
	YS_API ysRegionId YS_CALL add_region(const char* regionName);

	/// Adds a value to a counter.
	/// @internal
	YS_API void YS_CALL emit_counter_add(ysCounterId counterId, ysLocationId locationId, double amount);

	/// Emit a region.
	/// @internal
	YS_API void YS_CALL emit_region(ysRegionId regionId, ysLocationId locationId, ysTime startTime, ysTime endTime);

	/// Emits an event from the current thread.
	YS_API ysResult YS_CALL emit_event(ysEvent const& ev);

	/// Parses an event out of a buffer.
	YS_API ysResult YS_CALL read_event(ysEvent& out_ev, std::size_t& out_len, void const* buffer, std::size_t available);

	/// Read the current clock value.
	/// @internal
	YS_API ysTime YS_CALL read_clock();

	/// Managed a scoped region.
	/// @internal
	struct ScopedRegion final
	{
		__forceinline ScopedRegion(ysRegionId regionId, ysLocationId locationId) : _regionId(regionId), _locationId(locationId), _startTime(read_clock()) {}
		__forceinline ~ScopedRegion() { emit_region(_regionId, _locationId, _startTime, read_clock()); }

		ScopedRegion(ScopedRegion const&) = delete;
		ScopedRegion& operator=(ScopedRegion const&) = delete;

		ysRegionId _regionId;
		ysLocationId _locationId;
		ysTime _startTime;
	};

} // namespace _ys_

#endif // !defined(NO_YS)

#endif // YARDSTICK_H