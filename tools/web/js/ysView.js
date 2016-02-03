window.YsView = function(ysProtocol, options){
	var lastTick = 0;
	var frequency = 0;
	var width = 0;
		
	var frametimes = [];
	var counters = {};
	var start = 0;
	
	var series = [{
		type: 'stepArea',
		dataPoints: frametimes,
		name: 'Frametime',
		showInLegend: true
	}];
	var chart = new CanvasJS.Chart(options.graph, {
		theme: 'theme2',
		zoomEnabled: true,
		animationEnabled: false,
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
            }
		},
		toolTip: {
			content: '{name}: {y}'
		},
		data: series
	});

	function add(arr, val){
		while (arr.length != 0 && arr[0].x < lastTick - width)
			arr.shift();
		arr.push(val);
	}
	
	function add_counter(ysProtocol, ev){
		var data = [];
		var counter = counters[ev.name] = {
			last: ev.when,
			accum: 0,
			data: data
		};
		series.push({
			visible: false,
			type: 'line',
			dataPoints: data,
			name: ysProtocol.tostr(ev.name),
			showInLegend: true
		});
		return counter;
	}
	
	ysProtocol.on('header', function(ev){
		frequency = 1 / ev.frequency;
		width = 4 * ev.frequency;
		start = lastTick = ev.start;
	});

	ysProtocol.on('tick', function(ev){
		var dt = ev.when - lastTick;
		lastTick = ev.when;
		
		add(frametimes, {x: ev.when, y: dt * frequency * 1000});
		
		for (var name in counters) {
			var counter = counters[name];
			if (counter.accum != 0) {
				add(counter.data, {x: ev.when, y: counter.accum});
				counter.accum = 0;
			}
		}
		
		chart.render();
	});

	ysProtocol.on('record', function(ev){
		if (ev.name in counters) {
			add(counters[ev.name], {x: ev.when, y: ev.value});
		} else {
			var counter = add_counter(ysProtocol, ev);
			if (counter.accum != 0) {
				add(counter.data, {x: ev.when, y: counter.accum});
				counter.accum = 0;
			}
			add(counter.data, {x: ev.when, y: ev.value});
		}
	});
	
	ysProtocol.on('count', function(ev){
		if (ev.name in counters) {
			counters[ev.name].accum += ev.amount;
		} else {
			var counter = add_counter(ysProtocol, ev);
			ev.accum += ev.amount;
		}
	});
	
	ysProtocol.on('disconnected', function(ev){
		add(frametimes, {x: lastTick, y: null});
		for (var name in counters)
			add(counters[name].data, {x: lastTick, y: null});
			add(counters[name].data, {x: lastTick, y: null});
	});
};