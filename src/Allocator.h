/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

#include "yardstick/yardstick.h"

#include <vector>

namespace _ys_ {

void* Allocate(void* ptr, size_t size);
void SetAllocator(ysAllocator alloc);

template <typename T>
struct Allocator
{
	using value_type = T;

	Allocator() = default;
	template <class U> Allocator(Allocator<U> const&) {}

	T* allocate(std::size_t count) { return static_cast<T*>(Allocate(nullptr, count * sizeof(T))); }
	void deallocate(T* block, std::size_t count) { Allocate(block, count * sizeof(T)); }
};

template <class T, class U> bool operator==(Allocator<T> const&, Allocator<U> const&) { return true; }
template <class T, class U> bool operator!=(Allocator<T> const&, Allocator<U> const&) { return true; }

template <typename T>
using Vector = std::vector<T, Allocator<T>>;

} // namespace _ys_
