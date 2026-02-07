/**************************************************************************
 *
 * Tile Input definition file. 
 *
 */

#ifndef _TILEINPUT_

#define _TILEINPUT_

#include "support.h"
#include "GPUType.h"
#include "DynamicObject.h"
#include "Tile.h"

namespace cg1gpu
{

/**
 * 
 *  @file cmTileInput.h 
 *
 *  This file defines the Tile Input class.
 *
 */
  
/**
 *
 *  This class defines objects that carry tiles for recursive
 *  rasterization between the Tile FIFO and the Tile Evaluators.
 *
 *  This class inherits from the DynamicObject class that
 *  offers basic dynamic memory and signal tracing support
 *  functions.
 *
 */
 
class TileInput : public DynamicObject
{

private:

    U32 triangleID;          //  Triangle identifier.  
    U32 setupID;             //  Setup triangle identifier.  
    Tile *tile;                 //  Pointer to the tile being carried.  
    bool end;                   //  Last tile generated from the parent tile.  
    bool last;                  //  Last triangle signal.  

public:

    /**
     *
     *  Tile Input constructor.
     *
     *  Creates and initializes a tile input.
     *
     *  @param id The triangle identifier being rasterized in the tile.
     *  @param setupID The setup triangle identifier.
     *  @param tile Pointer to the tile being carried for evaluation.
     *  @param end Last generated tile from the input parent tile.
     *  @param last If it is the last triangle/tile for the batch.
     *
     *  @return An initialized tile input.
     *
     */
     
    TileInput(U32 id, U32 setupID, Tile *tile, bool end, bool last);
    
    /**
     *
     *  Gets the triangle identifier.
     *
     *  @return The triangle identifier.
     *
     */

    U32 getTriangleID();
    
    /**
     *
     *  Gets the setup triangle identifier.
     *
     *  @return The setup triangle identifier.
     *
     */

    U32 getSetupTriangleID();
    
    /**
     *
     *  Gets the tile being carried by the Tile Input object.
     *
     *  @return The pointer to the Tile being carried by the
     *  Tile Input object.
     *
     */
     
    Tile *getTile();
    
    /**
     *
     *  Asks if the current tile is the last tile generated from
     *  its parent tile.
     *
     *  @return If the current tile is the last generated from the
     *  parent tile.
     *
     */
     
    bool isEndTile();
    
    /**
     * 
     *  Asks if the last triangle flag is enabled for this triangle.
     *
     *  @return If the last triangle signal is enabled.
     *
     */
    
    bool isLast();
    
};

} // namespace cg1gpu

#endif
