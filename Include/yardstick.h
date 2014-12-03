/* Copyright (C) 2014 Sean Middleditch, all rights reserverd. */

#if !defined(YARDSTICK_H)
#define YARDSTICK_H

#include <stdint.h>

/* default to enabled if not disable */
#if !defined(NO_YS)
#	define YS_ENABLED 1
#else
#	define YS_ENABLED 0
#endif

/* configure macros necessary for shared library exports */
#if defined(_WIN32)
#	define YS_CALL __cdecl
#	if defined(YARDSTICK_EXPORT)
#		define YS_API __declspec(dllexport)
#	else
#		define YS_API __declspec(dllimport)
#	endif
#else
#	error "Unsupported platform"
#endif

/* helper needed to concatenate string */
#define YS_CAT2(a, b) a##b
#define YS_CAT(a, b) YS_CAT2(a,b)

#ifdef __cplusplus
extern "C" {
#endif

/** \brief Type returned by the high-resolution timer.
 */
typedef uint64_t ys_clock_t;

/** \brief Type of any unique identifier.
 */
typedef uint16_t ys_id_t;

/** \brief Reads the high-resolution timer.
 *  \returns The current timer value, akin to x86 rtdsc.
 */
typedef ys_clock_t(YS_CALL *ys_read_clock_ticks_cb)(void);

/** \brief Reads the high-resolution timer's frequency.
 *  \returns The frequency, akin to x86's rtdsc frequency.
 */
typedef ys_clock_t(YS_CALL *ys_read_clock_frequency_cb)(void);

/** \brief Memory allocation callback.
 *  Follows the rules of realloc(), except that it will only be used to allocate or free.
 */
typedef void*(YS_CALL *ys_alloc_cb)(void* block, size_t bytes);

/** \brief List of possible event types from the event callback system. */
typedef enum _ys_ev_type_t
{
	YS_EV_START,
	YS_EV_STOP,
	YS_EV_ADD_LOCATION,
	YS_EV_ADD_COUNTER,
	YS_EV_ADD_ZONE,
	YS_EV_INCREMENT_COUNTER,
	YS_EV_ENTER_ZONE,
	YS_EV_EXIT_ZONE,
	YS_EV_TICK,
} ys_ev_type_t;

typedef union _ys_event_t
{
	ys_ev_type_t type;
	struct { ys_ev_type_t type; ys_clock_t clockNow; ys_clock_t clockFrequency; } start;
	struct { ys_ev_type_t type; ys_clock_t clockNow; } stop;
	struct { ys_ev_type_t type; ys_id_t locationId; char const* fileName; int lineNumber; char const* functionName; } add_location;
	struct { ys_ev_type_t type; ys_id_t counterId; char const* counterName; } add_counter;
	struct { ys_ev_type_t type; ys_id_t zoneId; char const* zoneName; } add_zone;
	struct { ys_ev_type_t type; ys_id_t counterId; ys_id_t locationId; ys_clock_t clockNow; double amount; } increment_counter;
	struct { ys_ev_type_t type; ys_id_t zoneId; ys_id_t locationId; ys_clock_t clockNow; uint16_t depth; } enter_zone;
	struct { ys_ev_type_t type; ys_id_t zoneId; ys_clock_t clockStart; ys_clock_t clockElapsed; uint16_t depth; } exit_zone;
	struct { ys_ev_type_t type; ys_clock_t clockNow; } tick;
} ys_event_t;

/** \brief Callback that receives events when they are triggered.
 *  \param userData User-provided pointer registered with the callback.
 *  \param ev The event data.
 */
typedef void(YS_CALL *ys_event_cb)(void* userData, ys_event_t const* ev);

/** \brief Error return codes.
 */
typedef enum _ys_error_t
{
	/** \brief Success result.
	 */
	YS_OK,
	/** \brief One or more parameters contain invalid values.
	 */
	YS_INVALID_PARAMETER,
	/** \brief Memory or disk space was required but resources are exhausted.
	 */
	YS_RESOURCES_EXHAUSTED,
	/** \brief A failure was detected but the cause is not known.
	 */
	YS_UNKNOWN_ERROR,
	/** \brief The user attempted to do something more than once that can only be done once (like initialize the library).
	 */
	YS_ONCE_ONLY,
	/** \brief A system error occurred and the system error reporting facilities may contain more information.
	 */
	YS_SYSTEM_FAILURE,
} ys_error_t;

/** \brief Initialization data for Yardstick.
 *  Passed to ys_initialize().
 *  Set fields to NULL to get the default behavior.
 */
typedef struct _ys_configuration_t
{
	ys_alloc_cb alloc;
	ys_read_clock_ticks_cb read_clock_ticks;
	ys_read_clock_frequency_cb read_clock_frequency;
} ys_configuration_t;

/** \brief Used to initialize a ys_configuration_t to all default values;
 */
#define YS_DEFAULT_CONFIGURATION {0}

#if YS_ENABLED
	/** \brief Initializes the Yardstick library.
	 *  Must be called before any other Yardstick function.
	 *  \param config A configuration structure (must not be NULL);
	 *  \param size The size of the structure pointed to by config (used for versioning).
	 *  \returns YS_OK on success, or another value on error.
	 */
	YS_API ys_error_t YS_CALL _ys_initialize(ys_configuration_t const* config, size_t size);

	/** \brief Shuts down the Yardstick library and frees any resources.
	 *  Yardstick functions cannot be called after this point without reinitializing it.
	 */
	YS_API void YS_CALL _ys_shutdown(void);

	/** \brief Registers a new sink.
	 *  \param userData A user-defined pointer that will be passed to sink callbacks.
	 *  \param callback The callback handed events.
	 *  \return YS_OK on success, or another value on error.
	 */
	YS_API ys_error_t YS_CALL _ys_add_sink(void* userData, ys_event_cb callback);

	/** \brief Removes a registerd sink.
	 *  \param userData A user-defined pointer that will be passed to sink callbacks.
	 *  \param callback The callback handed events.
	 */
	YS_API void YS_CALL _ys_remove_sink(void* userData, ys_event_cb callback);

	/** \brief Registers a location to be used for future calls.
	 */
	YS_API ys_id_t YS_CALL _ys_add_location(char const* fileName, int line, char const* functionName);

	/** \brief Registers a counter to be used for future calls.
	 */
	YS_API ys_id_t YS_CALL _ys_add_counter(const char* counterName);

	/** \brief Registers a zone to be used for future calls.
	 */
	YS_API ys_id_t YS_CALL _ys_add_zone(const char* zoneName);

	/** \brief Begins recording time for a zone.
	 */
	YS_API void YS_CALL _ys_enter_zone(ys_id_t zoneId, ys_id_t locationId);

	/** \brief Stops recording time on a zone.
	 */
	YS_API void YS_CALL _ys_exit_zone();

	/** \brief Adds a value to a counter.
	 */
	YS_API void YS_CALL _ys_increment_counter(ys_id_t counterId, ys_id_t locationId, double amount);

	/** \brief Call once per frame.
	 */
	YS_API void YS_CALL _ys_tick();
#endif /* YS_ENABLED */

#if YS_ENABLED
#	define ysInitialize(configPtr, configSize) (_ys_initialize((configPtr), (configSize)))
#	define ysShutdown() (_ys_shutdown())

#	define ysCount(name, count) \
	do{ \
		static ys_id_t const _ys_counter_id = _ys_add_counter(("" name)); \
		static ys_id_t const _ys_location_id = _ys_add_location(__FILE__, __LINE__, __FUNCTION__); \
		_ys_increment_counter(_ys_counter_id, _ys_location_id, double(count)); \
	}while(0)

#	define ysEnter(name) \
	do{ \
		static ys_id_t const _ys_zone_id = _ys_add_zone(("" name)); \
		static ys_id_t const _ys_location_id = _ys_add_location(__FILE__, __LINE__, __FUNCTION__); \
		_ys_enter_zone(_ys_zone_id, _ys_location_id, double(count)); \
	}while(0);
#	define ysExit() (_ys_exit_zone())

#	define ysTick() (_ys_tick())

#	define ysAddSink(userData, callback) (_ys_add_sink((userData), (callback)))
#	define ysRemoveSink(userData, callback) (_ys_remove_sink((userData), (callback)))
#else /* YS_ENABLED */
#	define ysInitialize(configPtr, configSize) (YS_OK)
#	define ysShutdown() ((void)0)

#	define ysCount(name, count) do{}while(0)

#	define ysEnter(name) do{}while(0)
#	define ysExit() ((void)0)

#	define ysTick() ((void)0)

#	define ysAddSink(userData, callback) (YS_OK)
#	define ysRemoveSink(userData, callback) ((void)0)
#endif /* YS_ENABLED */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* YARDSTICK_H */
