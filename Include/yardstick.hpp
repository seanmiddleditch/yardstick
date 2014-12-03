// Copyright (C) 2014 Sean Middleditch, all rights reserverd.

#if !defined(YARDSTICK_HH)
#define YARDSTICK_HH

#include "yardstick.h"

/// \brief Namespace containing all Yardstick classes.
namespace ys {

/// \brief An event sink
class ISink
{
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
};

#if YS_ENABLED
/// \brief Register a C++ sink.
/// \param sink The sink to register.
/// \return YS_OK on success.
YS_API ys_error_t YS_CALL AddSink(ISink* sink);

/// \brief Remove a registered sink.
/// \param sink A sink previously passed to AddSink.
YS_API void YS_CALL RemoveSink(ISink const* sink);

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
#	define ysZone(name) \
	static ys_id_t const YS_CAT(_ys_zone_id, __LINE__) = ::_ys_add_zone(("" name)); \
	static ys_id_t const YS_CAT(_ys_location_id, __LINE__) = ::_ys_add_location(__FILE__, __LINE__, __FUNCTION__); \
	::ys::ScopedZone YS_CAT(_ys_zone, __LINE__)(YS_CAT(_ys_zone_id, __LINE__), YS_CAT(_ys_location_id, __LINE__))
#else // YS_ENABLED
#	define ysZone(name) do{}while(false)
#endif // YS_ENABLED

#endif // YARDSTICK_HH