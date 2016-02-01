(function(){
	window.Ys = function(){
		this.events = {};
		this.stats = {
			events: 0,
			frames: 0,
			bytes: 0
		};
		this.strings = {};
	};
	
	var ys = window.Ys.prototype;
	
	ys.on = function(ev, cb){
		if (ev in this.events)
			this.events[ev].append(cb);
		else
			this.events[ev] = [cb];
	};
	
	ys.emit = function(ev, data){
		if (ev in this.events)
		{
			cbs = this.events[ev];
			for (var i = 0; i != cbs.length; ++i)
				cbs[i](data);
		}
	};
	
	ys.tostr = function(id){
		if (id in this.strings)
			return this.strings[id];
		else
			return '['+id+']';
	};
	
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
	
	function parseEvent(ys, data, pos){
		++ys.stats.events;
		
		var type = data.getUint8(pos);
		switch (type) {
			case 1 /*HEADER*/:
				var ev = {
					frequency: data.getUint64(pos + 1, true)
				};
				ys.emit('header', ev);
				return 9;
			case 2 /*TICK*/:
				var ev = {
					when: data.getUint64(pos + 1, true)
				}
				ys.emit('tick', ev);
				return 9;
			case 3 /*REGION*/:
				var ev = {
					line: data.getUint32(pos + 1, true),
					name: data.getUint32(pos + 5, true),
					file: data.getUint32(pos + 9, true),
					start: data.getUint64(pos + 13, true),
					end: data.getUint64(pos + 21, true)
				};
				ys.emit('region', ev);
				return 29;
			case 4 /*COUNTER*/:
				var ev = {
					line: data.getUint32(pos + 1, true),
					name: data.getUint32(pos + 5, true),
					file: data.getUint32(pos + 9, true),
					when: data.getUint64(pos + 13, true),
					value: data.getFloat64(pos + 21, true)
				};
				ys.emit('counter', ev);
				return 29;
			case 5 /*STRING*/:
				var id = data.getUint32(pos + 1, true);
				var len = data.getUint16(pos + 5, true);
				// yup, pretty terrible
				var str = '';
				for (var i = 0; i != len; ++i)
					str += String.fromCharCode(data.getUint8(pos + 7 + i));
				ys.strings[id] = str;
				return 7 + len;
			default /*NONE or Unknown*/:
				console.log('UNKNOWN(pos=', pos, ' type=', type, ')');
				return data.byteLength;
		}
	}

	function onMessage(ys, data){
		++ys.stats.frames;
		ys.stats.bytes += data.byteLength;
		
		var data = new DataView(data);
		var pos = 0;
		while (pos < data.byteLength)
			pos += parseEvent(ys, data, pos);
	}
	
	ys.connect = function(port){
		var self = this;
		
		var ws = new WebSocket('ws://localhost:' + port);
		ws.binaryType = 'arraybuffer';

		ws.onopen = function(){ console.log('opened'); };
		ws.onerror = function(error){ console.log('error:', error); };
		ws.onclosed = function(){ console.log('closed'); };
		ws.onmessage = function(ev){ onMessage(self, ev.data); };
	};
})();