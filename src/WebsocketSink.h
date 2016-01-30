/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

#include <yardstick/yardstick.h>

#include "ConcurrentCircularBuffer.h"
#include "webby/webby.h"

namespace _ys_ {

class WebsocketSink
{
	ConcurrentCircularBuffer<4096> _output;

public:
	ysResult Listen(unsigned short port);
	ysResult Close();
	
	ysResult Update();
	ysResult WriteEventStream(void const* buffer, std::uint32_t len);
	ysResult Flush();
};

}
