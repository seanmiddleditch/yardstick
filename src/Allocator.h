/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

#include "yardstick/yardstick.h"

#include <vector>
#include <cstdlib>

namespace _ys_ {

template <typename T>
class Allocator
{
	ysAllocator _alloc;

	static void* YS_CALL sysalloc(void* block, std::size_t bytes) { return std::realloc(block, bytes); }

public:
	using value_type = T;

	Allocator() : _alloc(&sysalloc) {}
	Allocator(ysAllocator alloc) : _alloc(alloc) { if (_alloc == nullptr) _alloc = &sysalloc; }
	template <typename U> Allocator(Allocator<U> const& rhs) : _alloc(rhs._alloc) {}

	T* allocate(std::size_t count) { return static_cast<T*>(_alloc(nullptr, count * sizeof(T))); }
	void deallocate(T* block, std::size_t count) { _alloc(block, 0); }

	template <typename U> friend bool operator==(Allocator<T> const& lhs, Allocator<U> const& rhs) { return lhs._alloc == rhs._alloc; }
	template <typename U> friend bool operator!=(Allocator<T> const& lhs, Allocator<U> const& rhs) { return lhs._alloc != rhs._alloc; }

	template <typename U> friend class Allocator;
};


template <typename T>
using Vector = std::vector<T, Allocator<T>>;

} // namespace _ys_
