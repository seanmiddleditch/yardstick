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

class YsCounterView {
	constructor(ysState, options) {
		this._ys = ysState;
		this._options = options;
		
		this._timeSpan = 4;
		this._frametimes = [];
		this._counters = {};
		
		this._start = 0;
		this._period = 0;
		
		this._series = [{
			type: 'stepArea',
			dataPoints: this._frametimes,
			name: 'Frametime',
			showInLegend: true
		}];
		this._axis = {
			interlacedColor: '#EFEFEF',
			valueFormatString: 'O',
			labelFormatter: (e)=>((e.value - this._start) * this._period).toFixed(2) + 's'
		};
		this._chart = new CanvasJS.Chart(this._options.graph, {
			theme: 'theme2',
			zoomEnabled: false,
			animationEnabled: false,
			axisX: this._axis,
			legend: {
				verticalAlign: 'top',
				horizontalAlign: 'right',
				fontSize: 12,
				cursor: 'pointer',
				itemclick: function(e){
					if (typeof(e.dataSeries.visible) === "undefined" || e.dataSeries.visible)
						e.dataSeries.visible = false;
					else
						e.dataSeries.visible = true;
					chart.render();
				}
			},
			toolTip: {
				content: (e)=>'['+((e.entries[0].dataPoint.x-start)*period).toFixed(2)+'s] '+e.entries[0].dataSeries.name+': '+e.entries[0].dataPoint.y
			},
			data: this._series
		});
			
		var self = this;
		ysState.on('header', function(ev){
			self._axis.interval = ev.frequency;
			self._period = 1 / ev.frequency;
			self._start = ev.start;
		});
	}
	
	draw() {
		var startTime = this._ys.now - this._ys.frequency * this._timeSpan;
		
 		this._frametimes.length = 0;
		var startIndex = this._ys.frames.findIndexByTime(startTime);
		this._frametimes.push({x: startTime});
		for (var i = startIndex; i < this._ys.frames.length; ++i) {
			var frame = this._ys.frames.frame(i);
			this._frametimes.push({x: frame.start, y: frame.length * this._ys.period * 1000});
		}
		
		/*for (var counter of this._ys.counters) {
			var data;
			if (counter.id in this._counters) {
				data = this._counters[counter.id];
			} else {
				data = [];
				this._counters[counter.id] = data;
				this._series.push({
					visible: false,
					type: 'line',
					dataPoints: data,
					name: this._ys.tostr(counter.id),
					showInLegend: true
				});
			}
			
			data.length = 0;
			var startIndex = counter.findIndexByTime(startTime);
			data.push({x: startTime});
			for (var i = startIndex; i != counter.data.length; ++i)
				data.push({x: counter.data[i][0], y: counter.data[i][1]});
		} */
		
		this._chart.render();
	}
};