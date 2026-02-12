/**************************************************************************
 *
 * Tile Evaluator State Info definition file.
 *
 */

/**
 *
 *  @file cmTileEvaluatorStateInfo.h
 *
 *  This file defines the TileEvaluatorStateInfo class.
 *
 *  The Tile Evaluator State Info class is used to carry state information 
 *  from the Tile Evaluators to the Tile FIFO.
 *
 */

#ifndef _TILEEVALUATORSTATEINFO_
#define _TILEEVALUATORSTATEINFO_

#include "DynamicObject.h"
#include "cmTileEvaluator.h"

namespace arch
{

/**
 *
 *  This class defines a container for the state signals
 *  that the Tile Evaluator boxes send to the Tile FIFO mdu.
 *
 *  Inherits from Dynamic Object that offers dynamic memory
 *  support and tracing capabilities.
 *
 */


class TileEvaluatorStateInfo : public DynamicObject
{
private:
    
    TileEvaluatorState state;   //  The Tile Evaluator state.  

public:

    /**
     *
     *  Creates a new TileEvaluatorStateInfo object.
     *
     *  @param state The Tile Evaluator state carried by this
     *  Tile Evaluator state info object.
     *
     *  @return A new initialized TileEvaluatorStateInfo object.
     *
     */
     
    TileEvaluatorStateInfo(TileEvaluatorState state);
    
    /**
     *
     *  Returns the Tile Evaluator state carried by the Tile Evaluator
     *  state info object.
     *
     *  @return The Tile Evaluator state carried in the object.
     *
     */
     
    TileEvaluatorState getState();
};

} // namespace arch

#endif
