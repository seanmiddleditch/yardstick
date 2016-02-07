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

class YsCounter {
	constructor(id) {
		this._id = id;
		this._accum = 0;
		this._data = [];
	}
	
	get id() { return this._id; }
	get data() { return this._data; }
	[Symbol.iterator]() { return this._data; }
	get length() { return this._data.length; }
	
	setValue(time, value) {
		var i = this._data.length - 1;
		while (i > 0 && this._data[i][0] > time)
			--i;
		this._data.splice(i + 1, 0, [time, value]);
	}
	
	addAmount(amount) {
		this._accum += amount;
	}
	
	apply(time) {
		if (this._accum != 0) {
			this.setValue(time, this._accum);
			this._accum = 0;
		}
	}
	
	findIndexByTime(time) {
		var data = this._data;
		var start = 0;
		var count = data.length;
		
		while (count > 0) {
			var step = (count / 2)|0;
			var i = start + step;
			if (data[i][0] < time) {
				start = i + 1;
				count -= step + 1;
			} else {
				count = step;
			}
		}
		
		return start;
	}
}

class YsStateCounters {
	constructor() {
		this._counters = new Map();
	}
	
	get counters() { return this._counters.keys(); }
	[Symbol.iterator]() { return this._counters.values(); }

	createCounter(id) {
		var counter = this._counters.get(id);
		if (counter)
			return counter;
		
		counter = new YsCounter(id);
		this._counters.set(id, counter);
		return counter;
	}
	
	setCounter(id, time, value) {
		var counter = this.createCounter(id);
		counter.setValue(time, value);
	}

	addCounter(id, amount){
		var counter = this.createCounter(id);
		counter.addAmount(amount);
	}
	
	apply(time) {
		for (var counter of this._counters)
			counter[1].apply(time);
	}
};

class YsFrame {
	constructor(index, start) {
		this._index = index;
		this._start = start;
		this._length = 0;
	}
	
	get index() { return this._index; }
	get start() { return this._start; }
	get length() { return this._length; }
	
	get end() { return this._start + this._length; }
	set end(val) { this._length = val - this._start; }
};

class YsFrameSet {
	constructor() {
		this._frames = [];
		this._current = null;
	}
	
	get all() { return this._frames; }
	get length() { return this._frames.length; }
	
	frame(index) { return this._frames[index]; }
	
	startFrame(time) {
		this.endFrame(time);
		
		this._current = new YsFrame(this._frames.length, time);
		this._frames.push(this._current);
	}
	
	endFrame(time) {
		if (this._current) {
			this._current.end = time
			this._current = null;
		}
	}
	
	findIndexByTime(time) {
		var data = this._frames;
		var start = 0;
		var count = data.length;
		
		while (count > 0) {
			var step = (count / 2)|0;
			var i = start + step;
			if (data[i].start < time) {
				start = i + 1;
				count -= step + 1;
			} else {
				count = step;
			}
		}
		
		return start;
	}
}

class YsState {
	constructor() {
		var protocol = this._protocol = new YsProtocol();
		
		this._events = new Map();
		this._strings = new Map();
		this._callbacks = new Map();
		
		this._tickFrequency = 0;
		this._startTick = 0;
		this._lastTick = 0;
		this._tickDelta = 0;
		
		this._frames = new YsFrameSet();
		
		this._counters = new YsStateCounters();
		
		protocol.on('connect', () => this.emit('connected'));
		protocol.on('disconnect', (ev) => { this._frames.endFrame(this._lastTick); this.emit('disconnected', ev); });
		protocol.on('error', (e) => this.emit('error', e));
		protocol.on('event', (ev) => this.handleEvent(ev));
	}
	
	get counters() { return this._counters; }
	get stats() { return this._protocol.stats; }
	get frames() { return this._frames; }
	
	get connected() { return this._protocol.connected; }
	
	get frequency() { return this._tickFrequency; }
	get period() { return this._tickPeriod; }
	
	get now() { return this._lastTick; }
	
	connect(host) {
		this._protocol.connect(host);
	}
	
	disconnect() {
		this._protocol.disconnect();
	}
	
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

	tostr(id) {
		var str = this._strings.get(id);
		if (str === undefined)
			return '['+id+']';
		else
			return str;
	}
	
	handleEvent(ev) {
		switch (ev.type) {
		case 'header':
			this._startTick = this._lastTick = ev.start;
			this._tickFrequency = ev.frequency;
			this._tickPeriod  = 1 / ev.frequency;
			
			// starts a new frame
			this._frames.startFrame(ev.start);
			break;
		case 'tick':
			this._frames.startFrame(ev.when);
			this._counters.apply(ev.when);
		
			this._frameDelta = ev.when - this._lastTick;
			this._lastTick = ev.when;
			break;
		case 'counter_set':
			this._counters.setCounter(ev.name, ev.when, ev.value);
			break;
		case 'string':
			this._strings.set(ev.id, ev.string);
			break;
		case 'counter_add':
			this._counters.addCounter(ev.name, ev.amount);
			break;
		}
			
		this.emit(ev.type, ev);
	}
};
