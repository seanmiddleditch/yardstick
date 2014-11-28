// Copyright (C) 2014 Sean Middleditch, all rights reserverd.

#pragma once

#include <cstdint>

class IProfileSink
{
public:
	enum class EUnits : uint8_t
	{
		Unknown,
		Seconds,
		Bytes,
	};

	IProfileSink() = default;
	IProfileSink(IProfileSink const&) = delete;
	IProfileSink& operator=(IProfileSink const&) = delete;
	virtual ~IProfileSink() = default;

	virtual void CreateLocationId(uint16_t id, char const* file, int line, char const* function) = 0;
	virtual void CreateCounterId(uint16_t id, char const* name) = 0;
	virtual void CreateZoneId(uint16_t id, char const* name) = 0;
	virtual void WriteAdjustCounter(uint16_t id, uint16_t loc, uint64_t time, double value) = 0;
	virtual void WriteZoneStart(uint16_t id, uint16_t loc, uint64_t startTime, uint16_t depth) = 0;
	virtual void WriteZoneEnd(uint16_t id, uint64_t start, uint64_t ticks, uint16_t depth) = 0;
	virtual void Tick() = 0;
};