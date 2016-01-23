// Copyright (C) 2014 Sean Middleditch, all rights reserverd.

#include "yardstick.h"

#if !defined(NO_YS)

#include <atomic>
#include <cstring>
#include <vector>
#include <type_traits>

// we need this on Windows for the default clock
#if defined(_WIN32)
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#	define YS_FORCEINLINE __forceinline
#else
#	define YS_FORCEINLINE __attribute__((force_inline))
#endif

namespace {
namespace _internal
{
	enum class EProtocol : std::uint8_t
	{
		Tick = 0,
	};

	/// Definition of a location.
	/// @internal
	struct Location
	{
		char const* file;
		char const* func;
		int line;

		YS_FORCEINLINE bool operator==(Location const& rhs) const;
	};

	/// Allocator using the main context.
	/// @internal
	template <typename T>
	struct StdAllocator
	{
		using value_type = T;

		inline T* allocate(std::size_t bytes);
		inline void deallocate(T* block, std::size_t);
	};

	using Lock = std::atomic<int>;

	class LockGuard
	{
		Lock& _lock;

	public:
		inline LockGuard(Lock& lock);
		inline ~LockGuard();

		LockGuard(LockGuard const&) = delete;
		LockGuard& operator=(LockGuard const&) = delete;

	};

	class Semaphore
	{
		std::atomic<int> _count = 0;

	public:
		Semaphore() = default;
		Semaphore(Semaphore const&) = delete;
		Semaphore& operator=(Semaphore const&) = delete;

		bool TryWait(int n = 1);
		void Wait(int n = 1);
		void Post(int n = 1);
	};

	class Buffer
	{
		std::uint32_t _size = 0;
		std::uint8_t _buffer[512 - sizeof(std::uint32_t)];

	public:
		Buffer() = default;

		std::uint32_t size() const { return _size; }
		std::uint32_t capacity() const { return sizeof(_buffer); }
		std::uint32_t available() const { return sizeof(_buffer) - _size; }

		std::uint8_t const* data() const { return _buffer; }

		void write(void const* d, unsigned n) { std::memcpy(_buffer + _size, d, n); _size += n; }
	};

	template <typename T>
	class ConcurrentQueue
	{
		static_assert(std::is_pod<T>::value, "ConcurrentQueue can only be used for PODs");

		static constexpr std::uint32_t kBufferSize = 512;
		static constexpr std::uint32_t kBufferMask = kBufferSize - 1;

		std::atomic_uint32_t _sequence[kBufferSize];
		T _buffer[kBufferSize];
		std::atomic_uint32_t _enque = 0;
		std::atomic_uint32_t _deque = 0;

	public:
		inline ConcurrentQueue();
		ConcurrentQueue(ConcurrentQueue const&) = delete;
		ConcurrentQueue& operator=(ConcurrentQueue const&) = delete;

		inline bool TryEnque(T const& value);
		inline void Enque(T const& value);
		inline bool TryDeque(T& out);
	};

	/// Default realloc() wrapper.
	/// @internal
	void* YS_CALL SystemAllocator(void* block, std::size_t bytes) { return realloc(block, bytes); }

	/// <summary> Writes a message into the outgoing buffer.  </summary>
	/// <param name="data"> The data. </param>
	/// <param name="len"> The length. </param>
	void WriteBuffer(void const* data, unsigned len);

	template <typename T>
	void Write(T const& value) { WriteBuffer(&value, sizeof(value)); }

	template <typename T, typename... Ts>
	void Write(T const& value, Ts const&... ts)
	{
		WriteBuffer(&value, sizeof(value));
		Write(ts...);
	}

	/// Default clock tick reader.
	/// @internal
	YS_FORCEINLINE ysTime ReadClock();

	/// Default clock frequency reader.
	/// @internal
	YS_FORCEINLINE ysTime GetClockFrequency();

	/// Find the index of a value in an array.
	template <typename T>
	YS_FORCEINLINE std::size_t FindValue(T const* first, T const* last, T const& value)
	{
		for (T const* it = first; it != last; ++it)
			if (*it == value)
				return it - first;
		return last - first;
	}

	/// Find the first occurance of a predicate in an array
	template <typename T, typename Pred>
	YS_FORCEINLINE std::size_t FindIf(T const* first, T const* last, Pred const& pred)
	{
		for (T const* it = first; it != last; ++it)
			if (pred(*it))
				return it - first;
		return last - first;
	}

	ysAllocator gAllocator = &SystemAllocator;

	Semaphore gBufferCount;
	ConcurrentQueue<Buffer*> gBufferQueue;
	ConcurrentQueue<Buffer*> gFreeBuffers;

	thread_local Buffer* gtBuffer = nullptr;

	Lock gGlobalLock;
	bool gInitialized = false;
	std::vector<Location, StdAllocator<Location>> gLocations;
	std::vector<char const*, StdAllocator<char const*>> gCounters;
	std::vector<char const*, StdAllocator<char const*>> gRegions;
} // namespace _internal
} // anonymous namespace

using namespace _internal;

bool _internal::Location::operator==(Location const& rhs) const
{
	return line == rhs.line &&
		0 == std::strcmp(file, rhs.file) &&
		0 == std::strcmp(func, rhs.func);
}

template <typename T>
T* _internal::StdAllocator<T>::allocate(std::size_t count)
{
	return static_cast<T*>(gAllocator(nullptr, count * sizeof(T)));
}

template <typename T>
void _internal::StdAllocator<T>::deallocate(T* block, std::size_t)
{
	gAllocator(block, 0U);
}

_internal::LockGuard::LockGuard(std::atomic<int>& lock) : _lock(lock)
{
	int expected = 0;
	while (!_lock.compare_exchange_weak(expected, 1, std::memory_order_acquire))
		expected = 0;
}

_internal::LockGuard::~LockGuard()
{
	_lock.store(0, std::memory_order_release);
}

bool _internal::Semaphore::TryWait(int n)
{
	int c = _count.load(std::memory_order_relaxed);
	return c >= n && _count.compare_exchange_weak(c, c - n, std::memory_order_acquire);
}

void _internal::Semaphore::Wait(int n)
{
	while (!TryWait(n))
		;
}

void _internal::Semaphore::Post(int n)
{
	_count.fetch_add(n, std::memory_order_release);
}

template <typename T>
_internal::ConcurrentQueue<T>::ConcurrentQueue() 
{
	for (std::uint32_t i = 0; i != kBufferSize; ++i)
		_sequence[i].store(i, std::memory_order_relaxed);
}

template <typename T>
bool _internal::ConcurrentQueue<T>::TryEnque(T const& value)
{
	std::uint32_t target = _enque.load(std::memory_order_relaxed);
	std::uint32_t id = _sequence[target & kBufferMask].load(std::memory_order_acquire);
	std::int32_t delta = id - target;

	while (!(delta == 0 && _enque.compare_exchange_weak(target, target + 1, std::memory_order_relaxed)))
	{
		if (delta < 0)
			return false;

		target = _enque.load(std::memory_order_relaxed);
		id = _sequence[target & kBufferMask].load(std::memory_order_acquire);
		delta = id - target;
	}

	_buffer[target & kBufferMask] = value;
	_sequence[target & kBufferMask].store(target + 1, std::memory_order_release);
	return true;
}

template <typename T>
void _internal::ConcurrentQueue<T>::Enque(T const& value)
{
	while (!TryEnque(value))
		;
}

template <typename T>
bool _internal::ConcurrentQueue<T>::TryDeque(T& out)
{
	std::uint32_t target = _enque.load(std::memory_order_relaxed);
	std::uint32_t id = _sequence[target & kBufferMask].load(std::memory_order_acquire);
	std::int32_t delta = id - (target + 1);

	while (!(delta == 0 && _deque.compare_exchange_weak(target, target + 1, std::memory_order_relaxed)))
	{
		if (delta < 0)
			return false;

		target = _deque.load(std::memory_order_relaxed);
		id = _sequence[target & kBufferMask].load(std::memory_order_acquire);
		delta = id - (target + 1);
	}

	out = _buffer[target & kBufferMask];
	_sequence[target & kBufferMask].store(target + kBufferMask + 1, std::memory_order_release);
	return true;
}

void _internal::WriteBuffer(void const* data, unsigned len)
{
	Buffer* buffer = gtBuffer;

	if (buffer == nullptr || buffer->available() < len)
	{
		gBufferQueue.Enque(buffer);
		if (!gFreeBuffers.TryDeque(buffer))
			buffer = new (ysAlloc(sizeof(Buffer))) Buffer;
		gtBuffer = buffer;
	}

	buffer->write(data, len);
}

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
	LockGuard _(gGlobalLock);

	// only allow initializing once per process
	if (gInitialized)
		return ysResult::AlreadyInitialized;

	if (allocator != nullptr)
		gAllocator = allocator;

	gInitialized = true;

	return ysResult::Success;
}

ysResult YS_API _ys_::shutdown()
{
	LockGuard _(gGlobalLock);

	if (!gInitialized)
		return ysResult::Uninitialized;

	// do this first, so checks from here on out "just work"
	gInitialized = false;

	gLocations.clear();
	gCounters.clear();
	gRegions.clear();

	gLocations.shrink_to_fit();
	gCounters.shrink_to_fit();
	gRegions.shrink_to_fit();

	return ysResult::Success;
}

YS_API void* YS_CALL _ys_::alloc(std::size_t size)
{
	return gAllocator(nullptr, size);
}

YS_API void YS_CALL _ys_::free(void* ptr)
{
	gAllocator(ptr, 0);
}

YS_API ysLocationId YS_CALL _ys_::add_location(char const* fileName, int line, char const* functionName)
{
	LockGuard _(gGlobalLock);

	YS_ASSERT(gInitialized, "Cannot register a location without initializing Yardstick first");

	Location const location{ fileName, functionName, line };

	size_t const index = FindValue(gLocations.data(), gLocations.data() + gLocations.size(), location);
	auto const id = ysLocationId(index + 1);
	if (index < gLocations.size())
		return id;

	gLocations.push_back(location);

	return id;
}

YS_API ysCounterId YS_CALL _ys_::add_counter(const char* counterName)
{
	LockGuard _(gGlobalLock);

	YS_ASSERT(gInitialized, "Cannot register a counter without initializing Yardstick first");

	// this may be a duplicate; return the existing one if so
	size_t const index = FindIf(gCounters.data(), gCounters.data() + gCounters.size(), [=](char const* str){ return std::strcmp(str, counterName) == 0; });
	auto const id = ysCounterId(index + 1);
	if (index < gCounters.size())
		return id;

	gCounters.push_back(counterName);
	
	return id;
}

YS_API ysRegionId YS_CALL _ys_::add_region(const char* zoneName)
{
	LockGuard _(gGlobalLock);

	YS_ASSERT(gInitialized, "Cannot register a region without initializing Yardstick first");

	// this may be a duplicate; return the existing one if so
	size_t const index = FindIf(gRegions.data(), gRegions.data() + gRegions.size(), [=](char const* str){ return std::strcmp(str, zoneName) == 0; });
	auto const id = ysRegionId(index + 1);
	if (index < gRegions.size())
		return id;

	gRegions.push_back(zoneName);

	return id;
}

YS_API void YS_CALL _ys_::emit_counter_add(ysCounterId counterId, ysLocationId locationId, double amount)
{
	auto const now = ReadClock();
}

YS_API void YS_CALL _ys_::emit_region(ysRegionId regionId, ysLocationId locationId, ysTime startTime, ysTime endTime)
{
}

YS_API ysTime YS_CALL _ys_::read_clock()
{
	return ReadClock();
}

YS_API ysResult YS_CALL _ys_::emit_tick()
{
	// can't tick without a context
	if (!gInitialized)
		return ysResult::Uninitialized;

	ysTime const now = ReadClock();

	Write(EProtocol::Tick, now);

	return ysResult::Success;
}

#endif // !defined(NO_YS)
