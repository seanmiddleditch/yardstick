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
#	define YS_INLINE __forceinline
#	if defined(YARDSTICK_STATIC)
#		define YS_API extern
#	elif defined(YARDSTICK_EXPORT)
#		define YS_API __declspec(dllexport)
#	else
#		define YS_API __declspec(dllimport)
#	endif
#else
#	define YS_CALL
#	define YS_INLINE __attribute__((always_inline))
#	define YS_API __attribute__((visibility ("default")))
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

/// Type used to represent unique string identifiers.
using ysStringHandle = std::uint64_t;

/// Unique handle to a location.
enum class ysLocationId : std::uint32_t { None = 0 };

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
			int line;
			ysStringHandle name;
			ysStringHandle file;
			ysTime begin;
			ysTime end;
		} region;
		struct
		{
			int line;
			ysStringHandle name;
			ysStringHandle file;
			ysTime when;
			double value;
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
		::_ys_::ScopedRegion YS_CAT(_ys_region, __LINE__)(("" name), __FILE__, __LINE__)

#	define ysCounter(name, value) \
		(::_ys_::emit_counter(::_ys_::read_clock(), (value), ("" name), __FILE__, __LINE__))

#else // !defined(NO_YS)

#	define ysInitialize(allocator) (YS_IGNORE((allocator)),::ys::ErrorCode::Disabled)
#	define ysShutdown() (::ys::ErrorCode::Disabled)
#	define ysTick() (::ys::ErrorCode::Disabled)
#	define ysProfile(name) do{YS_IGNORE((name));}while(false)
#	define ysCounter(name, value) do{YS_IGNORE((name));YS_IGNORE((value));}while(false)

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
	YS_API ysLocationId YS_CALL add_location(char const* fileName, int line);

	/// Emit a counter.
	/// @internal
	YS_API void YS_CALL emit_counter(ysTime when, double value, char const* name, char const* file, int line);

	/// Emit a region.
	/// @internal
	YS_API void YS_CALL emit_region(ysTime startTime, ysTime endTime, char const* name, char const* file, int line);

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
		YS_INLINE ScopedRegion(char const* name, char const* file, int line) : _startTime(read_clock()), _name(name), _file(file), _line(line) {}
		YS_INLINE ~ScopedRegion() { emit_region(_startTime, read_clock(), _name, _file, _line); }

		ScopedRegion(ScopedRegion const&) = delete;
		ScopedRegion& operator=(ScopedRegion const&) = delete;

		ysTime _startTime;
		char const* _name;
		char const* _file;
		int _line;
	};

} // namespace _ys_

#endif // !defined(NO_YS)

#endif // YARDSTICK_H
