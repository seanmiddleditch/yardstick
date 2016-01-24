// Copyright (C) 2014-2016 Sean Middleditch, all rights reserverd.

#include "yardstick/yardstick.h"

#if !defined(NO_YS)

#include "ConcurrentQueue.h"
#include "ConcurrentCircularBuffer.h"
#include "Spinlock.h"
#include "Semaphore.h"
#include "Allocator.h"
#include "Algorithm.h"
#include "GlobalState.h"

#include <cstring>

// we need this on Windows for the default clock
#if defined(_WIN32)
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#	define YS_FORCEINLINE __forceinline
#else
#	define YS_FORCEINLINE __attribute__((force_inline))
#endif

using namespace _ys_;

namespace {
namespace _internal
{
	Semaphore gBufferCount;

	enum class EProtocol : std::uint8_t
	{
		Tick = 0,
		Region = 1,
		Counter = 2,
	};

	class ThreadState
	{
		ConcurrentCircularBuffer _buffer;

		template <typename T>
		static std::uint32_t calculate_size(T const& value) { return sizeof(value); }

		template <typename T, typename... Ts>
		static std::uint32_t calculate_size(T const& value, Ts const&... ts) { return calculate_size(value) + calculate_size(ts...); }

		template <typename T>
		static void write_value(void* out, T const& value) { std::memcpy(out, &value, sizeof(value)); }

		template <typename T, typename... Ts>
		static void write_value(void* out, T const& value, Ts const&... ts)
		{
			write_value(out, value);
			write_value(static_cast<char*>(out) + sizeof(value), ts...);
		}

	public:
		ThreadState() = default;
		ThreadState(ThreadState const&) = delete;
		ThreadState& operator=(ThreadState const&) = delete;

		template <typename... Ts>
		void Write(Ts const&... ts)
		{
			char buffer[128];
			std::uint32_t const size = calculate_size(ts...);
			YS_ASSERT(size <= sizeof(buffer), "buffer too small");
			write_value(buffer, ts...);
			if (_buffer.TryWrite(buffer, size))
				gBufferCount.Post();
		}

	};

	thread_local ThreadState gtState;

	/// Default clock tick reader.
	/// @internal
	YS_FORCEINLINE ysTime ReadClock();

	/// Default clock frequency reader.
	/// @internal
	YS_FORCEINLINE ysTime GetClockFrequency();
} // namespace _internal
} // anonymous namespace

using namespace _internal;

ysTime _internal::ReadClock()
{
#if defined(_WIN32) || defined(_WIN64)
	LARGE_INTEGER tmp;
	QueryPerformanceCounter(&tmp);
	return tmp.QuadPart;
#else
#		error "Platform unsupported"
#endif
}

ysTime _internal::GetClockFrequency()
{
#if defined(_WIN32) || defined(_WIN64)
	LARGE_INTEGER tmp;
	QueryPerformanceFrequency(&tmp);
	return tmp.QuadPart;
#else
#		error "Platform unsupported"
#endif
}

ysResult YS_API _ys_::initialize(ysAllocator allocator)
{
	if (allocator != nullptr)
		SetAllocator(allocator);
	return ysResult::Success;
}

ysResult YS_API _ys_::shutdown()
{
	return ysResult::Success;
}

YS_API ysLocationId YS_CALL _ys_::add_location(char const* fileName, int line, char const* functionName)
{
	return GlobalState::instance().RegisterLocation(fileName, line, functionName);
}

YS_API ysCounterId YS_CALL _ys_::add_counter(const char* counterName)
{
	return GlobalState::instance().RegisterCounter(counterName);
}

YS_API ysRegionId YS_CALL _ys_::add_region(const char* zoneName)
{
	return GlobalState::instance().RegisterRegion(zoneName);
}

YS_API void YS_CALL _ys_::emit_counter_add(ysCounterId counterId, ysLocationId locationId, double amount)
{
	auto const now = ReadClock();
	gtState.Write(EProtocol::Counter, counterId, locationId, amount, now);
}

YS_API void YS_CALL _ys_::emit_region(ysRegionId regionId, ysLocationId locationId, ysTime startTime, ysTime endTime)
{
	gtState.Write(EProtocol::Region, regionId, locationId, startTime, endTime);
}

YS_API ysTime YS_CALL _ys_::read_clock()
{
	return ReadClock();
}

YS_API ysResult YS_CALL _ys_::emit_tick()
{
	ysTime const now = ReadClock();
	gtState.Write(EProtocol::Tick, now);
	return ysResult::Success;
}

#endif // !defined(NO_YS)
