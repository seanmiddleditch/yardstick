window.YsView = function(ysProtocol, options){
	var lastTick = 0;
	var frequency = 0;
	var width = 0;
		
	var frametimes = [];
	var counters = {}
	var start = 0;
	
	var series = [{
		type: 'line',
		dataPoints: frametimes,
		name: 'Frametime',
		showInLegend: true
	}];
	var chart = new CanvasJS.Chart(options.graph, {
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
	
	ysProtocol.on('header', function(ev){
		frequency = 1 / ev.frequency;
		width = 10 * ev.frequency;
		start = lastTick = ev.start;
	});

	ysProtocol.on('tick', function(ev){
		var dt = ev.when - lastTick;
		lastTick = ev.when;
		
		add(frametimes, {x: ev.when, y: dt * frequency * 1000});
		chart.render();
	});
	
	ysProtocol.on('counter', function(ev){
		if (ev.name in counters) {
			add(counters[ev.name], {x: ev.when, y: ev.value});
		} else {
			var values = [{x: ev.when, y: ev.value}];
			counters[ev.name] = values;
			series.push({
				type: 'line',
				dataPoints: values,
				name: ysProtocol.tostr(ev.name),
				showInLegend: true
			});
		}
	});
	
	ysProtocol.on('disconnected', function(ev){
		add(frametimes, {x: lastTick, y: null});
		for (var name in counters)
			add(counters[name], {x: lastTick, y: null});
	});
};