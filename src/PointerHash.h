/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

// Based on hashes from Thomas Wang
// https://gist.github.com/badboy/6267743

// Note: the original hashes used signed shifts, this is all using unsigned.
// Does that break the quality of the hash? Seems like it might. Need to test
// and better understand if so and why. #FIXME

#pragma once

#include <cstdint>

namespace _ys_ {

template <int PS> struct _hash_pointer_impl;

template <>
struct _hash_pointer_impl<4>
{
	static std::uint32_t hash(std::uint32_t key)
	{
		key = ~key + (key << 15); // key = (key << 15) - key - 1;
		key = key ^ (key >> 12);
		key = key + (key << 2);
		key = key ^ (key >> 4);
		key = key * 2057; // key = (key + (key << 3)) + (key << 11);
		key = key ^ (key >> 16);
		return key;
	}
};

template <>
struct _hash_pointer_impl<8>
{
	static std::uint32_t hash(std::uint64_t key)
	{
		key = (~key) + (key << 18); // key = (key << 18) - key - 1;
		key = key ^ (key >> 31);
		key = key * 21; // key = (key + (key << 2)) + (key << 4);
		key = key ^ (key >> 11);
		key = key + (key << 6);
		key = key ^ (key >> 22);
		return static_cast<std::uint32_t>(key);
	}
};

std::uint32_t hash_pointer(void const* ptr) { return _hash_pointer_impl<sizeof(ptr)>::hash(reinterpret_cast<std::size_t>(ptr)); }

} // namespace _ys_
