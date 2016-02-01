/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#include "WebsocketSink.h"
#include "Clock.h"
#include <cstring>
#include <allocators>

#ifdef _WIN32
#	include <winsock2.h>
#endif

using namespace _ys_;

struct WebsocketSink::Session
{
	// note: no constructor or destructor is called for this struct!
	Session* _prev;
	Session* _next;
	WebsocketSink* _sink;
	WebbyConnection* _connection;
};

WebsocketSink::WebsocketSink()
{
#if defined(_WIN32)
	WORD wsa_version = MAKEWORD(2, 2);
	WSADATA wsa_data;
	WSAStartup(wsa_version, &wsa_data);
#endif
}

WebsocketSink::~WebsocketSink()
{
#if defined(_WIN32)
	WSACleanup();
#endif
}

void WebsocketSink::webby_log(const char* text)
{
	// #FIXME - we need a log mechanism in Yardstick
}

int WebsocketSink::webby_dispatch(struct WebbyConnection* connection)
{
	WebsocketSink& sink = *static_cast<WebsocketSink*>(connection->user_data);

	// we support no static files or paths with this tool
	return 1;
}

int WebsocketSink::webby_connect(struct WebbyConnection*)
{
	return 0;
}

void WebsocketSink::webby_connected(struct WebbyConnection* connection)
{
	WebsocketSink& sink = *static_cast<WebsocketSink*>(connection->user_data);

	Session* session = sink.CreateSession(connection);
	// #FIXME - if this fails, we want to boot the connection, but how?
	if (session == nullptr)
		return;

	ysEvent ev;
	ev.type = ysEvent::TypeHeader;
	ev.header.frequency = GetClockFrequency();

	sink.WriteEvent(session, ev);
}

void WebsocketSink::webby_closed(struct WebbyConnection* connection)
{
	WebsocketSink& sink = *static_cast<WebsocketSink*>(connection->user_data);
	sink.DestroySession(connection);
}

int WebsocketSink::webby_frame(struct WebbyConnection* connection, const struct WebbyWsFrame* frame)
{
	WebsocketSink& sink = *static_cast<WebsocketSink*>(connection->user_data);

	char buffer[1024];
	auto const len = WebbyRead(connection, buffer, sizeof(buffer));
	if (len >= 0)
	{
		// #FIXME: need a callback/command mechanism in Yardstick
	}
	return 0;
}

WebsocketSink::Session* WebsocketSink::CreateSession(WebbyConnection* connection)
{
	Session* session = (Session*)_allocator(nullptr, sizeof(Session));
	if (session == nullptr)
		return nullptr;

	session->_next = _sessions;
	session->_prev = nullptr;
	session->_sink = this;
	session->_connection = connection;

	_sessions = session;

	return session;
}

void WebsocketSink::DestroySession(WebbyConnection* connection)
{
	Session* session = _sessions;
	while (session != nullptr && session->_connection == connection)
		session = session->_next;

	if (session != nullptr)
	{
		if (session->_next != nullptr)
			session->_next->_prev = session->_prev;
		if (session->_prev != nullptr)
			session->_prev->_next = session->_next;
		if (_sessions == session)
			_sessions = session->_next;

		_allocator(session, 0);
	}
}

ysResult WebsocketSink::WriteEvent(Session* session, ysEvent const& ev)
{
	char buffer[64];
	std::size_t length;
	YS_TRY(write_event(buffer, sizeof(buffer), ev, length));

	WebbyBeginSocketFrame(session->_connection, WEBBY_WS_OP_BINARY_FRAME);
	WebbyWrite(session->_connection, buffer, length);
	WebbyEndSocketFrame(session->_connection);

	return ysResult::Success;
}

ysResult WebsocketSink::Listen(unsigned short port, ysAllocator allocator)
{
	Close();

	_allocator = allocator;
	_port = port;

	struct WebbyServerConfig config;
	std::memset(&config, 0, sizeof(config));

	config.bind_address = "127.0.0.1";
	config.listening_port = port;
	config.flags = WEBBY_SERVER_WEBSOCKETS;
	config.connection_max = 4;
	config.request_buffer_size = 2048;
	config.io_buffer_size = 8192;
	config.dispatch = &webby_dispatch;
	config.log = &webby_log;
	config.ws_connect = &webby_connect;
	config.ws_connected = &webby_connected;
	config.ws_closed = &webby_closed;
	config.ws_frame = &webby_frame;
	config.user_data = this;

	auto const size = WebbyServerMemoryNeeded(&config);
	_memory = _allocator(nullptr, size);
	if (_memory == nullptr)
		return ysResult::NoMemory;

	_server = WebbyServerInit(&config, _memory, size);
	if (_server == nullptr)
	{
		_allocator(_memory, 0);
		_memory = nullptr;

		return ysResult::Unknown;
	}
	else
	{
		return ysResult::Success;
	}
}

ysResult WebsocketSink::Close()
{
	if (_server != nullptr)
	{
		WebbyServerShutdown(_server);
		_server = nullptr;
	}

	if (_allocator != nullptr)
		_allocator(_memory, 0);

	while (_sessions != nullptr)
	{
		Session* tmp = _sessions->_next;
		_allocator(_sessions, 0);
		_sessions = tmp;
	}

	return ysResult::Success;
}

ysResult WebsocketSink::Update()
{
	if (_server != nullptr)
	{
		WebbyServerUpdate(_server);
		return ysResult::Success;
	}
	else
	{
		return ysResult::Uninitialized;
	}
}

ysResult WebsocketSink::WriteEvent(ysEvent const& ev)
{
	for (Session* session = _sessions; session != nullptr; session = session->_next)
		WriteEvent(session, ev);

	return ysResult::Success;
}
