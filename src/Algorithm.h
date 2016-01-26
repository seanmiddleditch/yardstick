/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

#include <cstdint>

namespace _ys_ {

/// Find the index of a value in an array.
template <typename T>
inline std::size_t FindValue(T const* first, T const* last, T const& value)
{
	for (T const* it = first; it != last; ++it)
		if (*it == value)
			return it - first;
	return last - first;
}

/// Find the first occurance of a predicate in an array
template <typename T, typename Pred>
inline std::size_t FindIf(T const* first, T const* last, Pred const& pred)
{
	for (T const* it = first; it != last; ++it)
		if (pred(*it))
			return it - first;
	return last - first;
}

} // namespace _ys_
