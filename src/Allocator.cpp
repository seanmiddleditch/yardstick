
#include "yardstick/yardstick.h"

#if !NO_YS

#include "Allocator.h"

#include <cstdlib>

namespace {

void* DefaultAllocator(void* block, std::size_t size) { return realloc(block, size); }

ysAllocator gAllocator = &DefaultAllocator;

} // anonymous namespace

void* _ys_::Allocate(void* block, std::size_t size)
{
	return gAllocator(block, size);
}

void _ys_::SetAllocator(ysAllocator alloc)
{
	gAllocator = alloc;
}

#endif // !NO_YS
