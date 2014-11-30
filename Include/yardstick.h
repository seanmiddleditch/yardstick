// Copyright (C) 2014 Sean Middleditch, all rights reserverd.

#if !defined(YARDSTICK_H)
#define YARDSTICK_H

#include "stdint.h"

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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ys_event_t ys_event_t;

/// \brief Type returned by the high-resolution timer.
typedef unsigned long long ys_clock_t;

/// \brief Reads the high-resolution timer.
/// \returns The current timer value, akin to x86 rtdsc.
typedef ys_clock_t(YS_CALL *ys_read_clock_ticks_cb)(void);

/// \brief Reads the high-resolution timer's frequency.
/// \returns The frequency, akin to x86's rtdsc frequency.
typedef ys_clock_t(YS_CALL *ys_read_clock_frequency_cb)(void);

/// \brief Memory allocation callback.
/// Follows the rules of realloc(), except that it will only be used to allocate or free.
typedef void*(YS_CALL *ys_alloc_cb)(void* block, size_t bytes);

/// \brief Receives profile events and should log or record them.
/// \param ev The event to record.
typedef void(YS_CALL *ys_event_cb)(ys_event_t const* ev);

/// \brief The event structure.
struct _ys_event_t
{
	uint8_t type;
};

/// \brief Error return codes.
typedef enum _ys_error_t
{
	YS_OK,
	YS_INVALID_PARAMETER,
	YS_UNKNOWN_ERROR,
} ys_error_t;

/// \brief Initialization data for Yardstick.
/// Passed to ys_initialize().
/// Set fields to NULL to get the default behavior.
typedef struct _ys_configuration_t
{
	ys_alloc_cb alloc;
	ys_read_clock_ticks_cb read_clock_ticks;
	ys_read_clock_frequency_cb read_clock_frequency;
} ys_configuration_t;

/// \brief Used to initialize a ys_configuration_t to all default values;
#define YS_DEFAULT_CONFIGURATION {0}

/// \brief Initializes the Yardstick library.
/// Must be called before any other Yardstick function.
/// \param config A configuration structure (must not be NULL);
/// \param size The size of the structure pointed to by config (used for versioning).
/// \returns YS_OK on success, or another value on error.
YS_API ys_error_t YS_CALL ys_initialize(ys_configuration_t const* config, size_t size);

/// \brief Shuts down the Yardstick library and frees any resources.
/// Yardstick functions cannot be called after this point without reinitializing it.
YS_API void YS_CALL ys_shutdown(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // YARDSTICK_H
