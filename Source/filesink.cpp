// Copyright (C) 2014 Sean Middleditch, all rights reserverd.

#include "yardstick.h"
#include "yardstick.hpp"

#include <cstring>
#include <cstdio>

namespace
{
	/// File sink.
	/// \internal
	class ProfileFileSink final : public ys::ISink
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

		bool OpenFile(char const* fileName);
		void Stop();

		bool IsOpen() const;

		void Start(ys_clock_t clockNow, ys_clock_t clockFrequence) override;
		void AddLocation(uint16_t id, char const* file, int line, char const* function) override;
		void AddCounter(uint16_t id, char const* name) override;
		void AddZone(uint16_t id, char const* name) override;
		void IncrementCounter(uint16_t id, uint16_t loc, uint64_t time, double value) override;
		void EnterZone(uint16_t id, uint16_t loc, uint64_t start, uint16_t depth) override {} // ignore
		void ExitZone(uint16_t id, uint64_t start, uint64_t ticks, uint16_t depth) override;
		void Tick(ys_clock_t clockNow) override;
		void Stop(ys_clock_t clockNow) override;
	};

	void ProfileFileSink::Flush()
	{
		if (m_File != nullptr)
			fwrite(m_Buffer, m_Cursor, 1, m_File);
		m_Cursor = 0;
	}

	void ProfileFileSink::WriteBuffer(void const* bytes, size_t size)
	{
		if (m_Buffer != nullptr)
		{
			if (m_Cursor + size > m_Capacity)
				Flush();

			std::memcpy(m_Buffer + m_Cursor, bytes, size);
			m_Cursor += size;
		}
	}

	void ProfileFileSink::WriteString(char const* string)
	{
		uint16_t const len = static_cast<uint16_t>(std::strlen(string));
		Write16(len);
		WriteBuffer(string, len);
	}

	void ProfileFileSink::WriteFloat64(double value)
	{
		WriteBuffer(&value, sizeof(value));
	}

	void ProfileFileSink::Write64(uint64_t value)
	{
	#if SDL_BIG_ENDIAN
		value = SDL_SwapLE64(value);
	#endif
		WriteBuffer(&value, sizeof(value));
	}

	void ProfileFileSink::Write32(uint32_t value)
	{
	#if SDL_BIG_ENDIAN
		value = SDL_SwapLE32(value);
	#endif
		WriteBuffer(&value, sizeof(value));
	}

	void ProfileFileSink::Write16(uint16_t value)
	{
	#if SDL_BIG_ENDIAN
		value = SDL_SwapLE16(value);
	#endif
		WriteBuffer(&value, sizeof(value));
	}

	bool ProfileFileSink::OpenFile(char const* fileName)
	{
		if (m_File == nullptr)
			m_File = fopen(fileName, "wb");

		if (m_File == nullptr)
			return false;

		if (m_Buffer == nullptr)
			m_Buffer = new char[m_Capacity];

		if (m_Buffer == nullptr)
		{
			fclose(m_File);
			m_File = nullptr;
			return false;
		}

		return true;
	}

	void ProfileFileSink::Start(ys_clock_t clockNow, ys_clock_t clockFrequency)
	{
		// simple enough header
		WriteBuffer("PROF0102\n", 8);
		Write64(clockNow);
		Write64(clockFrequency);
	}

	void ProfileFileSink::Stop()
	{
		Flush();

		if (m_File != nullptr)
			fclose(m_File);
		m_File = nullptr;

		delete[] m_Buffer;
		m_Buffer = nullptr;
	}

	bool ProfileFileSink::IsOpen() const
	{
		return m_File != nullptr;
	}

	void ProfileFileSink::AddLocation(uint16_t id, char const* file, int line, char const* function)
	{
		Write8(1); // new location header
		Write16(id);
		Write16((uint16_t)line);
		WriteString(file);
		WriteString(function);
	}

	void ProfileFileSink::AddCounter(uint16_t id, char const* name)
	{
		Write8(2); // new counter header
		Write16(id);
		WriteString(name);
	}

	void ProfileFileSink::AddZone(uint16_t id, char const* name)
	{
		Write8(3); // new zone header
		Write16(id);
		WriteString(name);
	}

	void ProfileFileSink::IncrementCounter(uint16_t id, uint16_t loc, uint64_t time, double value)
	{
		Write8(4); // counter value header
		Write16(id);
		Write16(loc);
		Write64(time);
		WriteFloat64(value);
	}

	void ProfileFileSink::ExitZone(uint16_t id, uint64_t start, uint64_t ticks, uint16_t depth)
	{
		Write8(6); // zone span header
		Write16(id);
		Write64(start);
		Write64(ticks);
		Write16(depth);
	}

	void ProfileFileSink::Tick(ys_clock_t clockNow)
	{
		Write8(7); // tick header
		Write64(clockNow);
	}
}