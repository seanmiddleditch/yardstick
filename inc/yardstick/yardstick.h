/* Yardstick
 * Copyright (c) 2014-1016 Sean Middleditch
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 * associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#if !defined(YARDSTICK_H)
#define YARDSTICK_H

// ---- Version Information ----

#define YS_VERSION_MAJOR "16"
#define YS_VERSION_MINOR "02"
#define YS_VERSION_PATCH "a"
#define YS_VERSION_NUMERIC 0x00160200

#define YS_VERSION YS_VERSION_MAJOR "." YS_VERSION_MINOR YS_VERSION_PATCH

// ---- Public Dependencies ----

#include <cstddef>
#include <cstdint>
#include <cassert>

// ---- Support Code ----

/// Configuration macros necessary for shared library exports.
#if defined(_WIN32)
#	define YS_CALL __fastcall
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

#define YS_TRY(expr) for(ysResult YS_CAT(_ys_result,__LINE__)=(expr);YS_CAT(_ys_result,__LINE__)!=ysResult::Success;){return YS_CAT(_ys_result,__LINE__);}

/// Type returned by the high-resolution timer.
using ysTime = std::uint64_t;

/// Type used to represent unique string identifiers.
using ysStringHandle = std::uint32_t;

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
	/// Yardstick has already been initialized.
	AlreadyInitialized,
};

// ---- Public Macros ----

#if !defined(NO_YS)

#	define ysEnabled() (::ysResult::Success)
#	define ysInitialize(config) (::_ys_::initialize((config)))
#	define ysShutdown() (::_ys_::shutdown())
#	define ysTick() (::_ys_::tick())
#	define ysListenWeb(port) (::_ys_::listen_web((port)))

  /// Marks the current scope as being in a region, and automatically closes the region at the end of the scope.
#	define ysProfile(name) \
		::_ys_::ScopedRegion YS_CAT(_ys_region, __LINE__)(("" name), __FILE__, __LINE__)

#	define ysCounterSet(name, value) \
		(::_ys_::emit_record(::_ys_::read_clock(), (value), ("" name), __FILE__, __LINE__))

#	define ysCounterAdd(name, amount) \
		(::_ys_::emit_count((amount), ("" name)))

#else // !defined(NO_YS)

#	define ysEnabled() (::ysResult::Disabled)
#	define ysInitialize(allocator) (YS_IGNORE((allocator)),::ysResult::Disabled)
#	define ysShutdown() (::ysResult::Disabled)
#	define ysTick() (::ysResult::Disabled)
#	define ysProfile(name) do{YS_IGNORE((name));}while(false)
#	define ysCounterSet(name, value) (YS_IGNORE((name)),YS_IGNORE((value)),::ysResult::Disabled)
#	define ysCounterAdd(name, amount) (YS_IGNORE((name)),YS_IGNORE((amount)),::ysResult::Disabled)
#	define ysListenWeb(port) (YS_IGNORE((port)),::ysResult::Disabled)

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
	YS_API ysResult YS_CALL tick();

	/// <summary> Listens for incoming Yardstick tool connections on the given port.  </summary>
	/// <param name="port"> The port to listen on. </param>
	/// <returns> Success or error code. </returns>
	YS_API ysResult YS_CALL listen_web(unsigned short port);

	/// Emit a record.
	/// @internal
	YS_API ysResult YS_CALL emit_record(ysTime when, double value, char const* name, char const* file, int line);

	/// Emit a counter.
	/// @internal
	YS_API ysResult YS_CALL emit_count(double amount, char const* name);

	/// Emit a region.
	/// @internal
	YS_API ysResult YS_CALL emit_region(ysTime startTime, ysTime endTime, char const* name, char const* file, int line);

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
