// Copyright (C) 2014 Sean Middleditch, all rights reserverd.

#if !defined(YS_PRIVATE_H)
#define YS_PRIVATE_H

// we need this on Windows, and since there's tricks to including it minimally (not yet applied), just do it here
#if defined(_WIN32)
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// \brief The current configuration.
/// \internal
extern ys_configuration_t _ys_context;

/// \brief Default clock function.
/// \internal
ys_clock_t _ys_default_read_clock_ticks(void);

/// \brief Default clock frequency function.
/// \internal
ys_clock_t _ys_default_read_clock_frequency(void);

/// \brief Default allocation function.
/// \internal
void* _ys_default_alloc(void*, size_t);

/// \brief Current allocation function.
/// \internal
extern ys_alloc_cb _ys_alloc_callback;

/// \brief Current clock ticks read function.
/// \internal
extern ys_read_clock_ticks_cb _ys_read_clock_ticks_callback;

/// \brief Current clock frequency read function.
/// \internal
extern ys_read_clock_frequency_cb _ys_read_clock_frequency_callback;

/// \brief Read the high-resolution clock.
/// \internal
ys_clock_t _ys_read_clock_ticks(void);

/// \brief Read the frequency of the high-resolution clock.
/// \internal
ys_clock_t _ys_read_clock_frequency(void);

/// \brief Allocate memory using the registered allocator.
/// \param bytes The minimum number of bytes for the allocation.
/// \internal
void* _ys_alloc(size_t bytes);

/// \brief Free a block of memory.
/// \param block The memory to free (NULL is allowed).
/// \internal
void _ys_free(void* block);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // YS_PRIVATE_H