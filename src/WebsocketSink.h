/* Yardstick
 * Copyright (c) 2014-1016 Sean Middleditch
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 * associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

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
