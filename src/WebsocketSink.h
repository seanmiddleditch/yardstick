/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#pragma once

#include <yardstick/yardstick.h>

#include "webby/webby.h"

namespace _ys_ {

struct EventData;

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
	Session* FindSession(WebbyConnection* connection);
	void DestroySession(Session* session);

	ysResult WriteSessionBytes(Session* session, void const* buffer, std::size_t size);
	ysResult WriteSessionString(Session* session, char const* str);
	ysResult WriteSessionEvent(Session* session, EventData const& ev);
	ysResult FlushSession(Session* session);

public:
	WebsocketSink();
	~WebsocketSink();

	WebsocketSink(WebsocketSink const&) = delete;
	WebsocketSink& operator=(WebsocketSink const&) = delete;

	ysResult Listen(unsigned short port, ysAllocator alloc);
	ysResult Close();
	
	ysResult Update();
	ysResult WriteEvent(EventData const& ev);
	ysResult Flush();
};

}
