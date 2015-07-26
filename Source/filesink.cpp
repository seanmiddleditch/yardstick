// Copyright (C) 2014 Sean Middleditch, all rights reserverd.

#include "yardstick.h"

#if !defined(NO_YS)

#include <type_traits>
#include <cstring>
#include <cstdio>

namespace
{
	/// File sink.
	/// @internal
	class ProfileFileSink final : public ysSink
	{
		static size_t const kBufferSize = 32 * 1024;

		FILE* _file = nullptr;
		char* _buffer = nullptr;
		size_t _cursor = 0;
		size_t _capacity = 0;

		void Flush();
		void WriteBuffer(void const* bytes, size_t size);
		void WriteString(char const* string);
		void WriteFloat64(double value) { WriteBuffer(&value, sizeof(value)); }
		void Write64(uint64_t value) { WriteBuffer(&value, sizeof(value)); }
		void Write32(uint32_t value) { WriteBuffer(&value, sizeof(value)); }
		void Write16(uint16_t value) { WriteBuffer(&value, sizeof(value)); }
		void Write8(uint8_t value) { WriteBuffer(&value, 1); }

		void WriteId(ysZoneId id) { Write16(static_cast<std::underlying_type_t<ysZoneId>>(id)); }
		void WriteId(ysCounterId id) { Write16(static_cast<std::underlying_type_t<ysCounterId>>(id)); }
		void WriteId(ysLocationId id) { Write16(static_cast<std::underlying_type_t<ysLocationId>>(id)); }

	public:
		ProfileFileSink(size_t capacity = kBufferSize) : _capacity(capacity) {}

		bool OpenFile(char const* fileName);
		void CloseFile();

		bool IsOpen() const { return _file != nullptr; }

		void YS_CALL BeginProfile(ysTime clockNow, ysTime clockFrequence) override;
		void YS_CALL EndProfile(ysTime clockNow) override;
		void YS_CALL AddLocation(ysLocationId id, char const* file, int line, char const* function) override;
		void YS_CALL AddCounter(ysCounterId id, char const* name) override;
		void YS_CALL AddZone(ysZoneId id, char const* name) override;
		void YS_CALL IncrementCounter(ysCounterId id, ysLocationId loc, uint64_t time, double value) override;
		void YS_CALL RecordZone(ysZoneId id, ysLocationId loc, uint64_t start, uint64_t end) override;
		void YS_CALL Tick(ysTime clockNow) override;
	};

	void ProfileFileSink::Flush()
	{
		if (_file != nullptr)
			fwrite(_buffer, _cursor, 1, _file);
		_cursor = 0;
	}

	void ProfileFileSink::WriteBuffer(void const* bytes, size_t size)
	{
		if (_buffer != nullptr)
		{
			if (_cursor + size > _capacity)
				Flush();

			std::memcpy(_buffer + _cursor, bytes, size);
			_cursor += size;
		}
	}

	void ProfileFileSink::WriteString(char const* string)
	{
		uint16_t const len = static_cast<uint16_t>(std::strlen(string));
		Write16(len);
		WriteBuffer(string, len);
	}

	bool ProfileFileSink::OpenFile(char const* fileName)
	{
		if (_file == nullptr)
			_file = fopen(fileName, "wb");

		if (_file == nullptr)
			return false;

		if (_buffer == nullptr)
			_buffer = (char*)ysAlloc(_capacity);

		if (_buffer == nullptr)
		{
			fclose(_file);
			_file = nullptr;
			return false;
		}

		return true;
	}

	void ProfileFileSink::CloseFile()
	{
		Flush();

		if (_file != nullptr)
			fclose(_file);
		_file = nullptr;

		ysFree(_buffer);
		_buffer = nullptr;
	}

	void ProfileFileSink::BeginProfile(ysTime clockNow, ysTime clockFrequency)
	{
		// simple enough header
		WriteBuffer("PROF0104", 8);
		Write64(clockNow);
		Write64(clockFrequency);
	}

	void ProfileFileSink::EndProfile(ysTime clockNow)
	{
		Write8('E');
		Write64(clockNow);
		CloseFile();
	}

	void ProfileFileSink::AddLocation(ysLocationId id, char const* file, int line, char const* function)
	{
		Write8('L'); // new location header
		WriteId(id);
		Write16((uint16_t)line);
		WriteString(file);
		WriteString(function);
	}

	void ProfileFileSink::AddCounter(ysCounterId id, char const* name)
	{
		Write8('C'); // new counter header
		WriteId(id);
		WriteString(name);
	}

	void ProfileFileSink::AddZone(ysZoneId id, char const* name)
	{
		Write8('Z'); // new zone header
		WriteId(id);
		WriteString(name);
	}

	void ProfileFileSink::IncrementCounter(ysCounterId id, ysLocationId loc, ysTime time, double value)
	{
		Write8('I'); // counter value header
		WriteId(id);
		WriteId(loc);
		Write64(time);
		WriteFloat64(value);
	}

	void ProfileFileSink::RecordZone(ysZoneId id, ysLocationId loc, ysTime start, ysTime end)
	{
		Write8('R'); // zone span header
		WriteId(id);
		WriteId(loc);
		Write64(start);
		Write64(end);
	}

	void ProfileFileSink::Tick(ysTime clockNow)
	{
		Write8('T'); // tick header
		Write64(clockNow);
	}
}

#endif // !defined(NO_YS)
