// Copyright (C) 2014 Sean Middleditch, all rights reserverd.

#if !defined(YARDSTICK_H)
#define YARDSTICK_H

#include "stdint.h"

#if defined(_WIN32)
#	define YS_API __cdecl
#else
#	define YS_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ys_event_t ys_event_t;

/// \brief Type returned by the high-resolution timer.
typedef unsigned long long ys_clock_t;

/// \brief Reads the high-resolution timer.
/// \returns The current timer value, akin to x86 rtdsc.
typedef ys_clock_t(YS_API *ys_read_clock_ticks_cb)(void);

/// \brief Reads the high-resolution timer's frequency.
/// \returns The frequency, akin to x86's rtdsc frequency.
typedef ys_clock_t(YS_API *ys_read_clock_frequency_cb)(void);

/// \brief Receives profile events and should log or record them.
/// \param ev The event to record.
typedef void(YS_API *ys_event_cb)(ys_event_t const* ev);

/// \brief Register custom timer callbacks.
/// \param clock_cb The clock callback that reads the high-resolution clock.
/// \param frequency_cb The frequency of the high-resolution clock.
void YS_API ys_register_clock_callbacks(ys_read_clock_ticks_cb ticks, ys_read_clock_frequency_cb frequency);

/// \brief The event structure.
struct _ys_event_t
{
	uint8_t type;
};

/// \brief Read the high-resolution clock.
ys_clock_t YS_API ys_read_clock_ticks(void);

/// \brief Read the frequency of the high-resolution clock.
ys_clock_t YS_API ys_read_clock_frequency(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // YARDSTICK_H