/**************************************************************************
 *
 * Memory Space definition file.
 *
 */

/**
 *
 *  @file cmMemorySpace.h
 *
 *  Memory Space definition file.  Defines masks describing how memory is mapped on the
 *  GPU address space.
 *
 */

#ifndef _MEMORY_SPACE_

#define _MEMORY_SPACE_

#include "GPUType.h"


namespace arch
{

static const U32 GPU_ADDRESS_SPACE    = 0x00000000; //*  Address space for the GPU memory.
static const U32 SYSTEM_ADDRESS_SPACE = 0x80000000; //*  Address space for the system memory.
static const U32 ADDRESS_SPACE_MASK   = 0x80000000; //*  Address space mask.
static const U32 SPACE_ADDRESS_MASK   = 0x7fffffff; //*  Address space mask.

} // namespace arch

#endif
