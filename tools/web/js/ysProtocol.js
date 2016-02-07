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
'use strict';

// extend DataView to support reading 64-bit unsigned ints,
// cast into native JS numbers (doubles). This isn't going to
// work for every U64 one might need, but it works for our
// current needs.
DataView.prototype.getUint64 = function(pos, endian){
	if (endian) {
		var lo = this.getUint32(pos, endian);
		var hi = this.getUint32(pos + 4, endian);
		return hi * Math.pow(2, 32) + lo;
	} else {
		var hi = this.getUint32(pos, endian);
		var lo = this.getUint32(pos + 4, endian);
		return hi * Math.pow(2, 32) + lo;
	}
}

class YsEventReader {
	constructor(data, pos) {
		this._data = data;
		this._pos = pos;
		this._error = null;
	}
	
	[Symbol.iterator]() { return this; }
	
	next() {
		var ev = this.read();
		return ev ? { done: false, value: ev } : { done: true };
	}
	
	read() {
		var pos = this._pos;
		var data = this._data;
		
		if (pos >= data.byteLength)
			return null;
		
		var type = data.getUint8(pos);
		switch (type) {
		case 1 /*HEADER*/:
			this._pos += 17;
			return {
				type: 'header',
				frequency: data.getUint64(pos + 1, true),
				start: data.getUint64(pos + 9, true)
			};
		case 2 /*TICK*/:
			this._pos += 9;
			return {
				type: 'tick',
				when: data.getUint64(pos + 1, true)
			};
		case 3 /*REGION*/:
			this._pos += 29;
			return {
				type: 'region',
				line: data.getUint32(pos + 1, true),
				name: data.getUint32(pos + 5, true),
				file: data.getUint32(pos + 9, true),
				start: data.getUint64(pos + 13, true),
				end: data.getUint64(pos + 21, true)
			};
		case 4 /*COUNTER_SET*/:
			this._pos += 29;
			return {
				type: 'counter_set',
				line: data.getUint32(pos + 1, true),
				name: data.getUint32(pos + 5, true),
				file: data.getUint32(pos + 9, true),
				when: data.getUint64(pos + 13, true),
				value: data.getFloat64(pos + 21, true)
			};
		case 5 /*STRING*/:
			var id = data.getUint32(pos + 1, true);
			var len = data.getUint16(pos + 5, true);
			// yup, pretty terrible
			var str = '';
			for (var i = 0; i != len; ++i)
				str += String.fromCharCode(data.getUint8(pos + 7 + i));
			
			this._pos += 7 + len;
			return {
				type: 'string',
				id: id,
				size: len,
				string: str
			};
		case 6 /*COUNTER_ADD*/:
			this._pos += 13;
			return {
				type: 'counter_add',
				name: data.getUint32(pos + 1, true),
				amount: data.getFloat64(pos + 5, true)
			};
		default /*NONE or Unknown*/:
			this._error = 'protocol parse error: pos '+pos+' byte='+type;
			this._pos += data.byteLength;
			return null;
		}
	}
}

class YsProtocol {
	constructor() {
		this._stats = {
			events: 0,
			frames: 0,
			bytes: 0
		};
		
		this._ws = null;
		
		this._callbacks = new Map();
	
		// public events
		this.onconnect = function(){};
		this.ondisconnect = function(){};
		this.onerror = function(error){};
		this.onevent = function(ev){};
	}
	
	get stats() { return this._stats; }
	get connected() { return this._ws != null; }
		
	on(ev, cb) {
		var cbs = this._callbacks.get(ev);
		if (cbs === undefined)
			this._callbacks.set(ev, [cb]);
		else
			cbs.push(cb);
	}

	emit(ev, data) {
		var cbs = this._callbacks.get(ev);
		if (cbs !== undefined)
			for (var cb of cbs)
				cb(data);
	}

	connect(host) {
		var ws = this._ws = new WebSocket('ws://' + host);
		ws.binaryType = 'arraybuffer';
		
		ws.onopen = () => this.emit('connect');
		ws.onerror = (err) => { this.emit('error', err); this._ws = null; };
		ws.onclose = (ev) => { this.emit('disconnect', ev); this._ws = null; };
		ws.onmessage = (msg) => {
			++this._stats.frames;
			this._stats.bytes += msg.data.byteLength;
			var reader = new YsEventReader(new DataView(msg.data), 0)
			for (var ev of reader) {
				++this._stats.events;
				this.emit('event', ev);
			}
			if (reader._error)
				this.emit('error', reader._error);
		};
	}
	
	disconnect() {
		if (this._ws) {
			this._ws.close();
			this._ws = null;
			this.emit('disconnect');
		}
	}
}