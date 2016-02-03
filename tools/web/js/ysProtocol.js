(function(){
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

	window.YsProtocol = function(){
		this.stats = {
			events: 0,
			frames: 0,
			bytes: 0
		};
		
		var events = {};
		var strings = {};
		
		var ws = null;
			
		this.on = function(ev, cb){
			if (ev in events)
				events[ev].push(cb);
			else
				events[ev] = [cb];
		};
		
		this.emit = function(ev, data){
			if (ev in events)
			{
				cbs = events[ev];
				for (var i = 0; i != cbs.length; ++i)
					cbs[i](data);
			}
		};
		
		this.tostr = function(id){
			if (id in strings)
				return strings[id];
			else
				return '['+id+']';
		};
		
		function parseEvent(data, pos){
			++this.stats.events;
			
			var type = data.getUint8(pos);
			switch (type) {
				case 1 /*HEADER*/:
					var ev = {
						frequency: data.getUint64(pos + 1, true),
						start: data.getUint64(pos + 9, true)
					};
					this.emit('header', ev);
					return 17;
				case 2 /*TICK*/:
					var ev = {
						when: data.getUint64(pos + 1, true)
					}
					this.emit('tick', ev);
					return 9;
				case 3 /*REGION*/:
					var ev = {
						line: data.getUint32(pos + 1, true),
						name: data.getUint32(pos + 5, true),
						file: data.getUint32(pos + 9, true),
						start: data.getUint64(pos + 13, true),
						end: data.getUint64(pos + 21, true)
					};
					this.emit('region', ev);
					return 29;
				case 4 /*RECORD*/:
					var ev = {
						line: data.getUint32(pos + 1, true),
						name: data.getUint32(pos + 5, true),
						file: data.getUint32(pos + 9, true),
						when: data.getUint64(pos + 13, true),
						value: data.getFloat64(pos + 21, true)
					};
					this.emit('record', ev);
					return 29;
				case 5 /*STRING*/:
					var id = data.getUint32(pos + 1, true);
					var len = data.getUint16(pos + 5, true);
					// yup, pretty terrible
					var str = '';
					for (var i = 0; i != len; ++i)
						str += String.fromCharCode(data.getUint8(pos + 7 + i));
					strings[id] = str;
					return 7 + len;
				case 6 /*COUNT*/:
					var ev = {
						name: data.getUint32(pos + 1, true),
						amount: data.getFloat64(pos + 5, true)
					};
					this.emit('count', ev);
					return 13;
				default /*NONE or Unknown*/:
					console.log('UNKNOWN(pos=', pos, ' type=', type, ')');
					return data.byteLength;
			}
		}

		function onMessage(data){
			++this.stats.frames;
			this.stats.bytes += data.byteLength;
			
			var data = new DataView(data);
			var pos = 0;
			while (pos < data.byteLength)
				pos += parseEvent.call(this, data, pos);
		}
		
		this.connect = function(host){
			var self = this;
			
			ws = new WebSocket('ws://' + host);
			ws.binaryType = 'arraybuffer';

			ws.onopen = function(){ self.emit('connected'); };
			ws.onerror = function(error){ self.emit('disconnected', error); };
			ws.onclosed = function(){ self.emit('disconnected', 'connection closed'); };
			ws.onmessage = function(ev){ onMessage.call(self, ev.data); };
		};
		
		this.disconnect = function(){
			if (ws) {
				ws.close();
				ws = null;
				this.emit('disconnected');
			}
		};
	};
})();