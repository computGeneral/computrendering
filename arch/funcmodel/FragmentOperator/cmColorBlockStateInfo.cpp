/**************************************************************************
 *
 * ColorBlockStateInfo implementation file.
 *
 */

/**
 *
 *  @file ColorBlockStateInfo.cpp
 *
 *  This file implements the ColorBlockStateInfo class.
 *
 *  This class is used to transmit the color buffer block
 *  state memory from the ColorWrite/ColorCache mdu to the
 *  DisplayController mdu.
 *
 */

#include "cmColorBlockStateInfo.h"

using namespace arch;

//  Constructor.  
ColorBlockStateInfo::ColorBlockStateInfo(ROPBlockState *state, U32 numBlocks)
{

    //  Set state memory pointer.  
    stateMemory = state;

    //  Set number of blocks.  
    blocks = numBlocks;

    setTag("CBStIn");
}


//  Gets the pointer to the state memory.  
ROPBlockState *ColorBlockStateInfo::getColorBufferState()
{
    return stateMemory;
}


//  Gets the number of blocks transmited.  
U32 ColorBlockStateInfo::getNumBlocks()
{
    return blocks;
}
