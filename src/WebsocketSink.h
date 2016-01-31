/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

#include <yardstick/yardstick.h>

#include "ConcurrentCircularBuffer.h"
#include "webby/webby.h"

namespace _ys_ {

class WebsocketSink
{
	ysAllocator _allocator = nullptr;
	unsigned short _port = 0;

	struct WebbyServer* _server = nullptr;
	struct WebbyConnection* _connection = nullptr;
	void* _memory = nullptr;

	static void webby_log(const char* text);
	static int webby_dispatch(struct WebbyConnection *connection);
	static int webby_connect(struct WebbyConnection *connection);
	static void webby_connected(struct WebbyConnection *connection);
	static void webby_closed(struct WebbyConnection *connection);
	static int webby_frame(struct WebbyConnection *connection, const struct WebbyWsFrame *frame);

	void WriteHeader();

public:
	WebsocketSink();
	~WebsocketSink();

	WebsocketSink(WebsocketSink const&) = delete;
	WebsocketSink& operator=(WebsocketSink const&) = delete;

	ysResult Listen(unsigned short port, ysAllocator alloc);
	ysResult Close();
	
	ysResult Update();
	ysResult WriteEventStream(int numBuffers, void const** buffers, std::uint32_t const* sizes);
	ysResult Flush();
};

}
