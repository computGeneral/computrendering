/**************************************************************************
 *
 * ColorBlockStateInfo definition file.
 *
 */

/**
 *
 *  @file cmColorBlockStateInfo.h
 *
 *  This file defines the ColorBlockStateInfo class.
 *
 *  This class is used to transmit the color buffer block
 *  state memory to the DisplayController unit.
 *
 */

#ifndef _COLORBLOCKSTATEINFO_

#include "DynamicObject.h"
#include "cmColorCacheV2.h"

namespace arch
{

/**
 *
 *  The class is used to carry the color block state memory
 *  from the ColorWrite/ColorCache mdu to the DisplayController unit.
 *
 *  Inherits from the dynamic object class.
 *
 */

class ColorBlockStateInfo : public DynamicObject
{

private:

    ROPBlockState *stateMemory;         //  The color buffer state memory being transmited.  
    U32 blocks;                      //  Number of blocks of the state memory being transmited.  

public:

    /**
     *
     *  Constructor.
     *
     *  @param state Pointer to the color buffer block state memory to transmit.
     *  @param numBlocks  Number of block states to transmit.
     *
     *  @return A colorblockstateinfo object.
     *
     */

    ColorBlockStateInfo(ROPBlockState *state, U32 numBlocks);

    /**
     *
     *  Retrieves the pointer to the state memory.
     *
     *  @return The pointer to the transmited state memory.
     *
     */

    ROPBlockState *getColorBufferState();

    /**
     *
     *  Get the number of blocks being transmited.
     *
     *  @return Number of block states transmited.
     *
     */

    U32 getNumBlocks();

};

} // namespace arch

#endif
