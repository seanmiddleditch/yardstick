/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

#include "WebsocketSink.h"
#include "Clock.h"
#include <cstring>

#ifdef _WIN32
#	include <winsock2.h>
#endif

using namespace _ys_;

static char const s_jslib[] =
"function ys(port){\n"
"	var statsDiv = document.getElementById('stats');\n"
"	var countersDiv = document.getElementById('counters');\n"
"	var timersDiv = document.getElementById('timers');\n"

"	var stats = { events: 0, blocks: 0, bytes: 0 }\n"
"	var counters = {}\n"
"	var timers = {}\n"

"	var lastTick = 0;\n"

"	function writeTable(stats, block){\n"
"		var html = '';\n"
"		for (k in stats)\n"
"			html += '<div><span>' + k + '</span><span> = </span><span>' + stats[k] + '</span></div>';\n"
"		block.innerHTML = html;\n"
"	}\n"

"	DataView.prototype.getUint64 = function(pos, endian){\n"
"		if (endian) {\n"
"			var lo = this.getUint32(pos, endian);\n"
"			var hi = this.getUint32(pos + 4, endian);\n"
"			return hi * Math.pow(2, 32) + lo;\n"
"		} else {\n"
"			var hi = this.getUint32(pos, endian);\n"
"			var lo = this.getUint32(pos + 4, endian);\n"
"			return hi * Math.pow(2, 32) + lo;\n"
"		}\n"
"	}\n"

"	function parseEvent(data, pos){"
"		var type = data.getUint8(pos);\n"
"		stats.events += 1;\n"
"		switch (type) {\n"
"			case 1 /*HEADER*/:\n"
"				timers.frequencyNanoseconds = data.getUint64(pos + 1, true);\n"
"				timers.frequencySeconds = 1 / timers.frequencyNanoseconds;\n"
"				console.log('HEADER(freq=', stats.freq, ')');\n"
"				return 9;\n"
"			case 2 /*TICK*/:\n"
"				var now = data.getUint64(pos + 1, true);\n"
"				timers.frameNanoseconds = now - lastTick;\n"
"				lastTick = now;\n"
"				timers.dt = timers.frameNanoseconds * timers.frequencySeconds;\n"
"				return 9;\n"
"			case 3 /*REGION*/:\n"
"				var line = data.getUint32(pos + 1, true);\n"
"				var file = data.getUint32(pos + 5, true);\n"
"				var name = data.getUint32(pos + 9, true);\n"
"				var start = data.getUint64(pos + 13, true);\n"
"				var end = data.getUint64(pos + 21, true);\n"
"				//console.log('REGION(file=', file, ' line=', line, ' name=', name, ' start=', start, ' end=', end, ')');\n"
"				return 29;\n"
"			case 4 /*COUNTER*/:\n"
"				var line = data.getUint32(pos + 1, true);\n"
"				var file = data.getUint32(pos + 5, true);\n"
"				var name = data.getUint32(pos + 9, true);\n"
"				var when = data.getUint64(pos + 13, true);\n"
"				var value = data.getFloat64(pos + 21, true);\n"
"				counters[name] = value;\n"
"				return 29;\n"
"			default /*NONE or Unknown*/:\n"
"				console.log('UNKNOWN(pos=', pos, ' type=', type);\n"
"				return data.byteLength;\n"
"		}\n"
"	}\n"

"	function onMessage(data){\n"
"		var data = new DataView(data);\n"
"		stats.blocks += 1;\n"
"		stats.bytes += data.byteLength;\n"
"		var pos = 0;\n"
"		while (pos < data.byteLength)\n"
"			pos += parseEvent(data, pos);\n"
"		writeTable(stats, statsDiv);\n"
"		writeTable(counters, countersDiv);\n"
"		writeTable(timers, timersDiv);\n"
"	}\n"

"	var ws = new WebSocket('ws://localhost:' + port);\n"
"	ws.binaryType = 'arraybuffer';\n"

"	ws.onopen = function(){ console.log('opened'); };\n"
"	ws.onerror = function(error){ console.log('error:', error); };\n"
"	ws.onclosed = function(){ console.log('closed'); };\n"
"	ws.onmessage = function(ev){ onMessage(ev.data); };\n"
"}\n"
;

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

	if (0 == std::strcmp(connection->request.uri, "/") && 0 == std::strcmp(connection->request.method, "GET"))
	{
		WebbyBeginResponse(connection, 200, -1, nullptr, 0);
		WebbyPrintf(connection,
			"<html><head><title>Yardstick</title>\n"
			"	<script src=\"yslib.js\" type=\"text/javascript\"></script>\n"
			"	<script type=\"text/javascript\">window.onload = function(){ ys(%u); }</script>\n"
			"</head><body><b>Timers</b><div id=\"timers\"></div><b>Counters</b><div id=\"counters\"></div><b>Stats</b><div id=\"stats\"></div></body></html>\n",
			sink._port);
		WebbyEndResponse(connection);
		return 0;
	}
	else if (0 == std::strcmp(connection->request.uri, "/yslib.js") && 0 == std::strcmp(connection->request.method, "GET"))
	{
		WebbyBeginResponse(connection, 200, sizeof(s_jslib) - 1/*NUL*/, nullptr, 0);
		WebbyWrite(connection, s_jslib, sizeof(s_jslib) - 1/*NUL*/);
		WebbyEndResponse(connection);
		return 0;
	}
	else
	{
		return 1;
	}
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
