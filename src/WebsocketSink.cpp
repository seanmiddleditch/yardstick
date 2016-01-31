/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#include "WebsocketSink.h"
#include "Clock.h"
#include <cstring>

#ifdef _WIN32
#	include <winsock2.h>
#endif

using namespace _ys_;

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
	WSACleanup();
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

int WebsocketSink::webby_connect(struct WebbyConnection* connection)
{
	WebsocketSink& sink = *static_cast<WebsocketSink*>(connection->user_data);

	return 0;
}

void WebsocketSink::webby_connected(struct WebbyConnection* connection)
{
	WebsocketSink& sink = *static_cast<WebsocketSink*>(connection->user_data);

	sink._connection = connection;

	sink.WriteHeader();
}

void WebsocketSink::WriteHeader()
{
	ysEvent ev;
	ev.type = ysEvent::TypeHeader;
	ev.header.frequency = GetClockFrequency();

	char buffer[32];
	std::size_t length;
	write_event(buffer, sizeof(buffer), ev, length);

	WebbyBeginSocketFrame(_connection, WEBBY_WS_OP_BINARY_FRAME);
	WebbyWrite(_connection, buffer, length);
	WebbyEndSocketFrame(_connection);
}

void WebsocketSink::webby_closed(struct WebbyConnection* connection)
{
	WebsocketSink& sink = *static_cast<WebsocketSink*>(connection->user_data);

	if (connection == sink._connection)
		sink._connection = nullptr;
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

ysResult WebsocketSink::WriteEventStream(int numBuffers, void const** buffers, std::uint32_t const* sizes)
{
	if (_server != nullptr)
	{
		if (_connection != nullptr)
		{
			WebbyBeginSocketFrame(_connection, WEBBY_WS_OP_BINARY_FRAME);
			for (int i = 0; i != numBuffers; ++i)
				WebbyWrite(_connection, buffers[i], sizes[i]);
			WebbyEndSocketFrame(_connection);
		}
		return ysResult::Success;
	}
	else
	{
		return ysResult::Uninitialized;
	}
}
