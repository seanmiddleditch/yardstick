/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

#include <atomic>
#include <cstdint>

namespace _ys_ {

static constexpr std::size_t kCachelineSize = 64;

template <typename T>
struct alignas(kCachelineSize) AlignedAtomic : std::atomic<T>
{
	using std::atomic<T>::atomic;

	char _padding[kCachelineSize - sizeof(std::atomic<T>)];
};

static_assert((kCachelineSize & (kCachelineSize - 1)) == 0, "kCachelineSize must be a power of 2");
static_assert(alignof(AlignedAtomic<std::uint32_t>) == kCachelineSize, "Failed to enforce alignment of AlignedAtomic");
static_assert(sizeof(AlignedAtomic<std::uint32_t>) == kCachelineSize, "Failed to enforce size of AlignedAtomic");

} // namespace _ys_
