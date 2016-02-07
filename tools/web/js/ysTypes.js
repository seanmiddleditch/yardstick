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

class YsRegion {
	constructor(start, depth, name) {
		this.start = start;
		this.end = start;
		this.depth = depth;
		this.name = name;
	}
	
	get length() { return this.end - this.start; }
}