// Copyright (C) 2014 Sean Middleditch, all rights reserverd.

#if !defined(YARDSTICK_HH)
#define YARDSTICK_HH

#include "yardstick.h"

/// \brief Namespace containing all Yardstick classes.
namespace ys {

/// \brief An event sink
class ISink
{
#if YS_ENABLED
	/// \brief Callback to map the C event callbacks to C++ interfaces.
	/// Included in a header to avoid ABI mismatch issues (like old versions of the interface).
	/// \param userData The interface pointer.
	/// \param ev The event data.
	/// \internal
	static inline void YS_CALL callback(void* userData, ys_event_t const* ev)
	{
		ISink* sink = static_cast<ISink*>(userData);

		switch (ev->type)
		{
		case YS_EV_START: return sink->Start(ev->start.clockNow, ev->start.clockFrequency);
		case YS_EV_STOP: return sink->Stop(ev->stop.clockNow);
		case YS_EV_ADD_LOCATION: return sink->AddLocation(ev->add_location.locationId, ev->add_location.fileName, ev->add_location.lineNumber, ev->add_location.functionName);
		case YS_EV_ADD_COUNTER: return sink->AddCounter(ev->add_counter.counterId, ev->add_counter.counterName);
		case YS_EV_ADD_ZONE: return sink->AddZone(ev->add_zone.zoneId, ev->add_zone.zoneName);
		case YS_EV_INCREMENT_COUNTER: return sink->IncrementCounter(ev->increment_counter.counterId, ev->increment_counter.locationId, ev->increment_counter.clockNow, ev->increment_counter.amount);
		case YS_EV_ENTER_ZONE: return sink->EnterZone(ev->enter_zone.zoneId, ev->enter_zone.locationId, ev->enter_zone.clockNow, ev->enter_zone.depth);
		case YS_EV_EXIT_ZONE: return sink->ExitZone(ev->exit_zone.zoneId, ev->exit_zone.clockStart, ev->exit_zone.clockElapsed, ev->exit_zone.depth);
		case YS_EV_TICK: return sink->Tick(ev->tick.clockNow);
		default: return;
		}
	}
#endif // YS_ENABLED

protected:
	ISink() = default;
	virtual ~ISink() = default;

public:
	ISink(ISink const&) = delete;
	ISink& operator=(ISink const&) = delete;

	virtual void Start(ys_clock_t clockNow, ys_clock_t clockFrequency) = 0;
	virtual void Stop(ys_clock_t clockNow) = 0;
	virtual void AddLocation(uint16_t locatonId, char const* fileName, int line, char const* functionName) = 0;
	virtual void AddCounter(uint16_t counterId, char const* counterName) = 0;
	virtual void AddZone(uint16_t zoneId, char const* zoneName) = 0;
	virtual void IncrementCounter(uint16_t counterId, uint16_t locationId, uint64_t clockNow, double value) = 0;
	virtual void EnterZone(uint16_t zoneId, uint16_t locationId, ys_clock_t clockNow, uint16_t depth) = 0;
	virtual void ExitZone(uint16_t zoneId, ys_clock_t clockStart, uint64_t clockElapsed, uint16_t depth) = 0;
	virtual void Tick(ys_clock_t clockNow) = 0;

#if YS_ENABLED
	static inline ys_error_t AddSink(ISink* sink) { return ysAddSink(sink, &callback); }
	static inline void RemoveSink(ISink* sink) { return ysRemoveSink(sink, &callback); }
#else
	static inline ys_error_t AddSink(ISink*) { return YS_OK; }
	static inline void RemoveSink(ISink*) {}
#endif
};

#if YS_ENABLED
/// \brief Managed a scoped zone.
/// \internal
struct ScopedZone final
{
	ScopedZone(ys_id_t zoneId, ys_id_t locationId) { _ys_enter_zone(zoneId, locationId); }
	~ScopedZone() { _ys_exit_zone(); }

	ScopedZone(ScopedZone const&) = delete;
	ScopedZone& operator=(ScopedZone const&) = delete;
};
#endif // YS_ENABLED

} // namespace ys

#if YS_ENABLED
/// \brief Marks the current scope as being in a zone, and automatically closes the zone at the end of the scope.
#	define ysZone(name) \
	static ys_id_t const YS_CAT(_ys_zone_id, __LINE__) = ::_ys_add_zone(("" name)); \
	static ys_id_t const YS_CAT(_ys_location_id, __LINE__) = ::_ys_add_location(__FILE__, __LINE__, __FUNCTION__); \
	::ys::ScopedZone YS_CAT(_ys_zone, __LINE__)(YS_CAT(_ys_zone_id, __LINE__), YS_CAT(_ys_location_id, __LINE__))
#else // YS_ENABLED
#	define ysZone(name) do{}while(false)
#endif // YS_ENABLED

#endif // YARDSTICK_HH