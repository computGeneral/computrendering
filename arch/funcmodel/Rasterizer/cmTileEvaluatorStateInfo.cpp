/**************************************************************************
 *
 * Tile Evaluator State Info implementation file.
 *
 */

/**
 *
 *  @file TileEvaluatorStateInfo.cpp
 *
 *  This file implements the TileEvaluatorStateInfo class.
 *
 *  The TileEvaluatorStateInfo class carries state information
 *  from the Tile Evaluators to the Tile FIFO.
 *
 */


#include "cmTileEvaluatorStateInfo.h"

using namespace cg1gpu;

//  Creates a new TileEvaluatorStateInfo object.  
TileEvaluatorStateInfo::TileEvaluatorStateInfo(TileEvaluatorState newState) : 

    state(newState)
{
    //  Set color for tracing.  
    setColor(state);
}


//  Returns the tile evaluator state carried by the object.  
TileEvaluatorState TileEvaluatorStateInfo::getState()
{
    return state;
}
