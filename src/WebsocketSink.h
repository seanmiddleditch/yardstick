/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

#include <yardstick/yardstick.h>

#include "webby/webby.h"

namespace _ys_ {

class WebsocketSink
{
	struct Session;

	ysAllocator _allocator = nullptr;
	unsigned short _port = 0;

	struct WebbyServer* _server = nullptr;
	void* _memory = nullptr;

	Session* _sessions = nullptr;

	static void webby_log(const char* text);
	static int webby_dispatch(struct WebbyConnection *connection);
	static int webby_connect(struct WebbyConnection *connection);
	static void webby_connected(struct WebbyConnection *connection);
	static void webby_closed(struct WebbyConnection *connection);
	static int webby_frame(struct WebbyConnection *connection, const struct WebbyWsFrame *frame);

	Session* CreateSession(WebbyConnection* connection);
	void DestroySession(WebbyConnection* connection);

	ysResult WriteEvent(Session* session, ysEvent const& ev);

public:
	WebsocketSink();
	~WebsocketSink();

	WebsocketSink(WebsocketSink const&) = delete;
	WebsocketSink& operator=(WebsocketSink const&) = delete;

	ysResult Listen(unsigned short port, ysAllocator alloc);
	ysResult Close();
	
	ysResult Update();
	ysResult WriteEvent(ysEvent const& ev);
	ysResult Flush();
};

}
