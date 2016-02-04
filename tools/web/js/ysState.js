/* Copyright (C) 2016 Sean Middleditch, all rights reserverd. */

window.YsState = function(){
	var self = this;
	var ws = null;
	var protocol = new YsProtocol();
	var events = {};
	var strings = {};

	var tickFrequency = 0;
	var startTick = 0;
	var lastTick = 0;
	
	var frame = null;
	var frames = [];
	
	protocol.onconnect = function(){ self.emit('connected'); };
	protocol.ondisconnect = function(){ self.emit('disconnected'); };
	protocol.onerror = function(e){ self.emit('error', e); };
	
	this.stats = protocol.stats;
	
	this.connect = function(host){
		protocol.connect(host);
	};
	this.disconnect = function(){
		protocol.disconnect();
	};
	
	this.on = function(ev, cb){
		if (ev in events)
			events[ev].push(cb);
		else
			events[ev] = [cb];
	};
	
	this.emit = function(ev, data){
		if (ev in events) {
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
	
	protocol.onevent = function(type, ev){
		switch (type) {
		case 'header':
			startTick = lastTick = ev.start;
			tickFrequency = ev.frequency;
			
			// starts a new frame
			frame = {
				index: frames.length,
				start: ev.start,
				end: ev.start,
				get dt(){ return (this.end - this.start) / tickFrequency; }
			};
			frames.push(frame);
			break;
		case 'tick':
			// finalize last frame
			frame.end = ev.when;
			frame.dt = (frame.end - frame.start) / tickFrequency;
			
			// start a new frame
			frame = {
				index: frames.length,
				start: ev.when,
				end: ev.when,
				get dt(){ return (this.end - this.start) / tickFrequency; }
			};
			frames.push(frame);
		
			dt = ev.when - lastTick;
			lastTick = ev.when;
			break;
		case 'string':
			strings[ev.id] = ev.string;
			break;
		}
			
		self.emit(type, ev);
	};
	
	this.getFrameByIndex = function(index){
		if (index > 0 && index < frames.length)
			return frames[index];
	};
	
	this.getFrameByTime = function(when){
		for (var i = 0; i != frames.length; ++i)
			if (frames[i].start >= when && when < frames[i].end)
				return frames[i];
	};
};