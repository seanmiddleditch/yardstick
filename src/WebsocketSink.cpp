/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#include "WebsocketSink.h"

using namespace _ys_;

ysResult WebsocketSink::Listen(unsigned short port)
{
	Close();
	return ysResult::Success;
}

ysResult WebsocketSink::Close()
{
	Flush();
	return ysResult::Success;
}

ysResult WebsocketSink::Update()
{
	return ysResult::Success;
}

ysResult WebsocketSink::WriteEventStream(void const* buffer, std::uint32_t len)
{
	if (!_output.TryWrite(buffer, len))
	{
		Flush();
		if (!_output.TryWrite(buffer, len))
			return ysResult::NoMemory;
	}

	return ysResult::Success;
}

ysResult WebsocketSink::Flush()
{
	char buffer[4096];
	auto const len = _output.Read(buffer, sizeof(buffer));
	if (len != 0)
	{

	}
	return ysResult::Success;
}
