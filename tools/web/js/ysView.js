(function(){
	window.YsView = function(ys){
		var statsDiv = document.getElementById('stats');
		var countersDiv = document.getElementById('counters');
		var timersDiv = document.getElementById('timers');

		var counters = {}
		var timers = {}

		var lastTick = 0;

		function writeTable(stats, block){
			var html = '';
			for (k in stats)
				html += '<div><span>' + k + '</span><span> = </span><span>' + stats[k] + '</span></div>';
			block.innerHTML = html;
		}
		
		ys.on('header', function(ev){
			timers.frequencyNS = ev.frequency;
			timers.frequency = 1 / ev.frequency;
		});

		ys.on('tick', function(ev){
			timers.frameNS = ev.when - lastTick;
			lastTick = ev.when;
			timers.dt = timers.frameNS * timers.frequency;
			
			writeTable(ys.stats, statsDiv);
			writeTable(timers, timersDiv);
			writeTable(counters, countersDiv);
		});
		
		ys.on('counter', function(ev){
			counters[ys.tostr(ev.name)] = ev.value;
		});
	};
})();