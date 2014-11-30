// Copyright (C) 2014 Sean Middleditch, all rights reserverd.

#include "yardstick.h"
#include "ys_private.h"

#include <stdlib.h>

// wrapped as realloc() is a macro sometimes, or other evil nonsense, also debugging
void* _ys_default_alloc(void* block, size_t bytes)
{
	return realloc(block, bytes);
}

void* _ys_alloc(size_t bytes)
{
	return _ys_context.alloc(NULL, bytes);
}

void _ys_free(void* block)
{
	_ys_context.alloc(block, 0U);
}