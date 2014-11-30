// Copyright (C) 2014 Sean Middleditch, all rights reserverd.

#include "yardstick.h"
#include "ys_private.h"

ys_configuration_t _ys_context = YS_DEFAULT_CONFIGURATION;

ys_error_t YS_API ys_initialize(ys_configuration_t const* config, size_t size)
{
	if (config == NULL)
		return YS_INVALID_PARAMETER;

	if (size != sizeof(ys_configuration_t))
		return YS_INVALID_PARAMETER;

	// install configuration
	memcpy(&_ys_context, config, size);

	// set defaults for any unset values
	if (_ys_context.alloc == NULL)
		_ys_context.alloc = _ys_default_alloc;
	if (_ys_context.read_clock_ticks == NULL)
		_ys_context.read_clock_ticks = _ys_default_read_clock_ticks;
	if (_ys_context.read_clock_frequency == NULL)
		_ys_context.read_clock_frequency = _ys_default_read_clock_frequency;

	return YS_OK;
}