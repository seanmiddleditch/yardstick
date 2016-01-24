/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#include "yardstick/yardstick.h"

#if !NO_YS

#include "GlobalState.h"
#include "Algorithm.h"

using namespace _ys_;

ysLocationId GlobalState::RegisterLocation(char const* file, int line, char const* function)
{
	LockGuard guard(_lock);

	Location const location{ file, function, line };

	size_t const index = FindValue(_locations.data(), _locations.data() + _locations.size(), location);
	auto const id = ysLocationId(index + 1);
	if (index < _locations.size())
		return id;

	_locations.push_back(location);

	return id;
}

ysCounterId GlobalState::RegisterCounter(char const* name)
{
	LockGuard guard(_lock);

	// this may be a duplicate; return the existing one if so
	size_t const index = FindIf(_counters.data(), _counters.data() + _counters.size(), [=](char const* str){ return std::strcmp(str, name) == 0; });
	auto const id = ysCounterId(index + 1);
	if (index < _counters.size())
		return id;

	_counters.push_back(name);

	return id;
}

ysRegionId GlobalState::RegisterRegion(char const* name)
{
	LockGuard guard(_lock);

	// this may be a duplicate; return the existing one if so
	size_t const index = FindIf(_regions.data(), _regions.data() + _regions.size(), [=](char const* str){ return std::strcmp(str, name) == 0; });
	auto const id = ysRegionId(index + 1);
	if (index < _regions.size())
		return id;

	_regions.push_back(name);

	return id;
}

#endif // !NO_YS