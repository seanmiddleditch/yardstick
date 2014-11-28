// Copyright (C) 2014 Sean Middleditch, all rights reserverd.

#pragma once

#include "IProfileSink.h"

#include <cstdio>

class ProfileFileSink final : public IProfileSink
{
	static size_t const kBufferSize = 32 * 1024;

	FILE* m_File = nullptr;
	char* m_Buffer = nullptr;
	size_t m_Cursor = 0;
	size_t m_Capacity = 0;

	void Flush();
	void WriteBuffer(void const* bytes, size_t size);
	void WriteString(char const* string);
	void WriteFloat64(double value);
	void Write64(uint64_t value);
	void Write32(uint32_t value);
	void Write16(uint16_t value);
	void Write8(uint8_t value) { WriteBuffer(&value, 1); }

public:
	ProfileFileSink(size_t capacity = kBufferSize) : m_Capacity(capacity) {}

	bool Open(char const* fileName);
	void Close();

	bool IsOpen() const;

	void CreateLocationId(uint16_t id, char const* file, int line, char const* function) override;
	void CreateCounterId(uint16_t id, char const* name) override;
	void CreateZoneId(uint16_t id, char const* name) override;
	void WriteAdjustCounter(uint16_t id, uint16_t loc, uint64_t time, double value) override;
	void WriteZoneStart(uint16_t id, uint16_t loc, uint64_t start, uint16_t depth) override {} // ignore
	void WriteZoneEnd(uint16_t id, uint64_t start, uint64_t ticks, uint16_t depth) override;
	void Tick() override;
};