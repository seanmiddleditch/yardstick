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

typedef void(YS_CALL *ys_sink_start_cb)(void* userData, ys_clock_t clockNow, ys_clock_t clockFrequency);
typedef void(YS_CALL *ys_sink_stop_cb)(void* userData, ys_clock_t clockNow);
typedef void(YS_CALL *ys_sink_add_location_cb)(void* userData, ys_id_t locationId, char const* fileName, int lineNumber, char const* functionName);
typedef void(YS_CALL *ys_sink_add_counter_cb)(void* userData, ys_id_t counterId, char const* counterName);
typedef void(YS_CALL *ys_sink_add_zone_cb)(void* userData, ys_id_t zoneId, char const* counterName);
typedef void(YS_CALL *ys_sink_increment_counter_cb)(void* userData, ys_id_t counterId, ys_id_t locationId, ys_clock_t clockNow, double value);
typedef void(YS_CALL *ys_sink_enter_zone_cb)(void* userData, ys_id_t zoneId, ys_id_t locationId, ys_clock_t clockNow, uint16_t depth);
typedef void(YS_CALL *ys_sink_exit_zone_cb)(void* userData, ys_id_t zoneId, ys_clock_t clockStart, ys_clock_t clockElapsed, uint16_t depth);
typedef void(YS_CALL *ys_sink_tick_cb)(void* userData, ys_clock_t clockNow);

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

/** \brief Functional interface for Yardstick sinks.
 */
typedef struct _ys_sink_t
{
	ys_sink_start_cb start;
	ys_sink_stop_cb stop;
	ys_sink_add_location_cb add_location;
	ys_sink_add_counter_cb add_counter;
	ys_sink_add_zone_cb add_zone;
	ys_sink_increment_counter_cb increment_counter;
	ys_sink_enter_zone_cb enter_zone;
	ys_sink_exit_zone_cb exit_zone;
	ys_sink_tick_cb tick;
} ys_sink_t;

/** \brief Default initialization for a Yardstick sink.
 */
#define YS_DEFAULT_SINK {0}

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
	 *  \param userData A user-defined pointer that will be passed to sink callbacks (must not be NULL, and must be unique).
	 *  \param sink The sink description table that will be copied (must not be NULL).
	 *  \param size The size of the sink table passed in.
	 *  \return YS_OK on success, or another value on error.
	 */
	YS_API ys_error_t YS_CALL ys_add_sink(void* userData, ys_sink_t const* sink, size_t size);

	/** \brief Removes a registerd sink.
	 *  \param userData A user-defined pointer that will be used to identify the sink to remove.
	 */
	YS_API void YS_CALL ys_remove_sink(void const* userData);

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

#	define ysAddSink(sink) (_ys_add_sink())
#	define ysRemoveSink(sink) (_ys_remove_sink())
#else /* YS_ENABLED */
#	define ysInitialize(configPtr, configSize) (YS_OK)
#	define ysShutdown() ((void)0)

#	define ysCount(name, count) do{}while(0)

#	define ysEnter(name) do{}while(0)
#	define ysExit() ((void)0)

#	define ysTick() ((void)0)

#	define ysAddSink(sink) (YS_OK)
#	define ysRemoveSink(sink) ((void)0)
#endif /* YS_ENABLED */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* YARDSTICK_H */
