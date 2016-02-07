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

window.YsRegionView = function(ysState, options){
	class Time {
		constructor(ysState) {
			this._ys = ysState;
			this._end = ysState.now;
			this._sticky = true;
			this.span = 1;
		}
			
		get window() { return this.span * this._ys.frequency; }
		get start() { return this.end - this.span * this._ys.frequency; }
		get end() { return this._sticky ? this._ys.now : this._end; }
		get leftBound() { return this._ys.start + this.window; }
		get rightBound() { return this._ys.now; }
		get bounds() { return [this.leftBound, this.rightBound]; }
		
		get sticky() { return this._sticky; }
		
		set end(value) {
			var bounds = this.bounds;
			this._end = Math.max(bounds[0], Math.min(bounds[1], value));
			this._sticky = (this._end == bounds[1]);
		}
	}
	
	function enable_pan(ysState, time, elem) {
		var panning = false;
		var offsetX = 0;
		
		elem.onmousedown = function(ev){
			offsetX = ev.offsetX;
			panning = true;
			ev.preventDefault();
			ev.stopPropagation();
		};
		
		elem.onmousemove = function(ev){
			if (panning) {
				var deltaX = (ev.offsetX - offsetX) / elem.width;
				offsetX = ev.offsetX;
				console.log(deltaX);
				
				time.end -= Math.floor(deltaX * ysState.frequency * time.span);
				
				if (ev.which == 0)
					panning = false;
			}
		};
	}

	var time = new Time(ysState);
	
	var canvas = options.canvas;
	var ctx = canvas.getContext('2d');
	var regions = [];
	var stack = [];
	
	enable_pan(ysState, time, canvas);
	
	var status = options.status;
	
	var depth = 0;
	
	function showStatus() {
		status.innerHTML = 'start=' + time.start + ' span=' + time.span + ' end=' + time.end + ' sticky=' + time.sticky;
	}
	
	function draw() {
		showStatus();
		var startTime = time.start;
		var endTime = time.end;
		var xScale = canvas.width / time.span * ysState.period;
		
		var startIndex = 0;
		while (startIndex < regions.length && regions[startIndex].end < startTime)
			++startIndex;
		
		ctx.clearRect(0, 0, canvas.width, canvas.height);
		ctx.fillStyle = 'blue';
		
		for (var i = startIndex; i < regions.length && regions[i].start < endTime; ++i) {
			var region = regions[i];
			
			var x = Math.floor(Math.max(0, (region.start - startTime) * xScale));
			var y = (1 + region.depth) * 20;
			var w = Math.max(1, Math.floor(region.length * xScale));
			
			ctx.fillRect(x, y, w, 20);
		}
		
		ctx.fillStyle = 'green';
		
		var startIndex = ysState.frames.findIndexByTime(startTime);
		for (var i = startIndex; i < ysState.frames.length; ++i) {
			var frame = ysState.frames.frame(i);
			
			var x = Math.floor(Math.max(0, (frame.start - startTime) * xScale));
			var w = Math.max(1, Math.floor(frame.length * xScale));
			
			ctx.fillRect(x, 0, w, 20);
		}
	}
	this.draw = draw;
	
	ysState.on('enter_region', function(ev){
		var region = new YsRegion(ev.when, stack.length, ev.name);
		regions.push(region);
		stack.push(region);
	});
	
	ysState.on('leave_region', function(ev){
		if (stack.length) {
			stack[stack.length - 1].end = ev.when;
			stack.pop();
		}
	});
};