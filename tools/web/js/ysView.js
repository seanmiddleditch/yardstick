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

window.YsView = function(ysState, options){
	var timeSpan = 4;
		
	var frametimes = [];
	var counters = {};
	var start = 0;
	var period = 0;
	
	var series = [{
		type: 'stepArea',
		dataPoints: frametimes,
		name: 'Frametime',
		showInLegend: true
	}];
	var axis = {
		interlacedColor: '#EFEFEF',
		valueFormatString: 'O',
		labelFormatter: (e)=>((e.value - start) * period).toFixed(2) + 's'
	};
	var chart = new CanvasJS.Chart(options.graph, {
		theme: 'theme2',
		zoomEnabled: false,
		animationEnabled: false,
		axisX: axis,
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
		data: series
	});
	
	ysState.on('header', function(ev){
		axis.interval = ev.frequency;
		period = 1 / ev.frequency;
		start = ev.start;
	});

	ysState.on('tick', function(ev){
		var startTime = ysState.now - ysState.frequency * timeSpan;
		
		frametimes.length = 0;
		var startIndex = ysState.frames.findIndexByTime(startTime);
		for (var i = startIndex; i != ysState.frames.length; ++i) {
			var frame = ysState.frames.frame(i);
			frametimes.push({x: frame.start, y: frame.length * ysState.period * 1000});
		}
		
		for (var counter of ysState.counters) {
			var data;
			if (counter.id in counters) {
				data = counters[counter.id];
			} else {
				data = [];
				counters[counter.id] = data;
				series.push({
					visible: false,
					type: 'line',
					dataPoints: data,
					name: ysState.tostr(counter.id),
					showInLegend: true
				});
			}
			
			data.length = 0;
			var startIndex = counter.findIndexByTime(startTime);
			for (var i = startIndex; i != counter.data.length; ++i)
				data.push({x: counter.data[i][0], y: counter.data[i][1]});
		}
		
		chart.render();
	});
};