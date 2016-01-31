(function(){
	window.Ys = {};
	
	function connect(port){
		var statsDiv = document.getElementById('stats');
		var countersDiv = document.getElementById('counters');
		var timersDiv = document.getElementById('timers');

		var stats = { events: 0, blocks: 0, bytes: 0 }
		var counters = {}
		var timers = {}

		var lastTick = 0;

		function writeTable(stats, block){
			var html = '';
			for (k in stats)
				html += '<div><span>' + k + '</span><span> = </span><span>' + stats[k] + '</span></div>';
			block.innerHTML = html;
		}

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

		function parseEvent(data, pos){
			var type = data.getUint8(pos);
			stats.events += 1;
			switch (type) {
				case 1 /*HEADER*/:
					timers.frequencyNanoseconds = data.getUint64(pos + 1, true);
					timers.frequencySeconds = 1 / timers.frequencyNanoseconds;
					console.log('HEADER(freq=', stats.freq, ')');
					return 9;
				case 2 /*TICK*/:
					var now = data.getUint64(pos + 1, true);
					timers.frameNanoseconds = now - lastTick;
					lastTick = now;
					timers.dt = timers.frameNanoseconds * timers.frequencySeconds;
					return 9;
				case 3 /*REGION*/:
					var line = data.getUint32(pos + 1, true);
					var file = data.getUint32(pos + 5, true);
					var name = data.getUint32(pos + 9, true);
					var start = data.getUint64(pos + 13, true);
					var end = data.getUint64(pos + 21, true);
					//console.log('REGION(file=', file, ' line=', line, ' name=', name, ' start=', start, ' end=', end, ')');
					return 29;
				case 4 /*COUNTER*/:
					var line = data.getUint32(pos + 1, true);
					var file = data.getUint32(pos + 5, true);
					var name = data.getUint32(pos + 9, true);
					var when = data.getUint64(pos + 13, true);
					var value = data.getFloat64(pos + 21, true);
					counters[name] = value;
					return 29;
				default /*NONE or Unknown*/:
					console.log('UNKNOWN(pos=', pos, ' type=', type);
					return data.byteLength;
			}
		}

		function onMessage(data){
			var data = new DataView(data);
			stats.blocks += 1;
			stats.bytes += data.byteLength;
			var pos = 0;
			while (pos < data.byteLength)
				pos += parseEvent(data, pos);
			writeTable(stats, statsDiv);
			writeTable(counters, countersDiv);
			writeTable(timers, timersDiv);
		}

		var ws = new WebSocket('ws://localhost:' + port);
		ws.binaryType = 'arraybuffer';

		ws.onopen = function(){ console.log('opened'); };
		ws.onerror = function(error){ console.log('error:', error); };
		ws.onclosed = function(){ console.log('closed'); };
		ws.onmessage = function(ev){ onMessage(ev.data); };
	}
	
	Ys.connect = connect;
})();