/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

#include "Spinlock.h"
#include "Allocator.h"

#include <cstring>

namespace _ys_ {

class GlobalState
{
	struct Location
	{
		char const* file;
		char const* func;
		int line;

		bool operator==(Location const& rhs) const
		{
			return line == rhs.line &&
				0 == std::strcmp(file, rhs.file) &&
				0 == std::strcmp(func, rhs.func);
		}
	};

	Spinlock _lock;
	Vector<Location> _locations;
	Vector<char const*> _counters;
	Vector<char const*> _regions;

public:
	GlobalState() = default;
	GlobalState(GlobalState const&) = delete;
	GlobalState& operator=(GlobalState const&) = delete;

	inline static GlobalState& instance();

	ysLocationId RegisterLocation(char const* file, int line, char const* function);
	ysCounterId RegisterCounter(char const* name);
	ysRegionId RegisterRegion(char const* name);
};

GlobalState& GlobalState::instance()
{
	static GlobalState state;
	return state;
}

} // namespace _ys_
