
/**
 *
 * This file defines types for the PIX trace. 
 *
 */

#ifndef _PIXTYPESH_
#define _PIXTYPESH_

#include "GPUType.h"

#ifndef PIX_X64
    typedef U32 PIXPointer;
#else
    typedef U64 PIXPointer
#endif    

#endif 

