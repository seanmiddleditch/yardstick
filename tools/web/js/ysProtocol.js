/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

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
		
		var ws = null;
		
		this.onconnect = function(){};
		this.ondisconnect = function(){};
		this.onerror = function(error){};
		this.onevent = function(type, ev){};
		
		function parseEvent(data, pos){
			++this.stats.events;
			
			var type = data.getUint8(pos);
			switch (type) {
				case 1 /*HEADER*/:
					this.onevent('header', {
						frequency: data.getUint64(pos + 1, true),
						start: data.getUint64(pos + 9, true)
					});
					return 17;
				case 2 /*TICK*/:
					this.onevent('tick', {
						when: data.getUint64(pos + 1, true)
					});
					return 9;
				case 3 /*REGION*/:
					this.onevent('region', {
						line: data.getUint32(pos + 1, true),
						name: data.getUint32(pos + 5, true),
						file: data.getUint32(pos + 9, true),
						start: data.getUint64(pos + 13, true),
						end: data.getUint64(pos + 21, true)
					});
					return 29;
				case 4 /*COUNTER_SET*/:
					this.onevent('counter_set', {
						line: data.getUint32(pos + 1, true),
						name: data.getUint32(pos + 5, true),
						file: data.getUint32(pos + 9, true),
						when: data.getUint64(pos + 13, true),
						value: data.getFloat64(pos + 21, true)
					});
					return 29;
				case 5 /*STRING*/:
					var id = data.getUint32(pos + 1, true);
					var len = data.getUint16(pos + 5, true);
					// yup, pretty terrible
					var str = '';
					for (var i = 0; i != len; ++i)
						str += String.fromCharCode(data.getUint8(pos + 7 + i));
					this.onevent('string', {
						id: id,
						size: len,
						string: str
					});
					return 7 + len;
				case 6 /*COUNTER_ADD*/:
					this.onevent('counter_add', {
						name: data.getUint32(pos + 1, true),
						amount: data.getFloat64(pos + 5, true)
					});
					return 13;
				default /*NONE or Unknown*/:
					this.error('protocol parse error: pos '+pos+' byte='+type);
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

			ws.onopen = function(){ self.onconnect(); };
			ws.onerror = function(err){ self.onerror(err); };
			ws.onclosed = function(){ self.ondisconnect(); };
			ws.onmessage = function(ev){ onMessage.call(self, ev.data); };
		};
		
		this.disconnect = function(){
			if (ws) {
				ws.close();
				ws = null;
				this.ondisconnect();
			}
		};
	};
})();