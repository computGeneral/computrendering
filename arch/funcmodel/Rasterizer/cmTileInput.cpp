/**************************************************************************
 *
 * Tile Input definition file. 
 *
 */

/**
 *
 *  @file TileInput.cpp
 *
 *  This file implements the Tile Input class.
 *
 */
 
#include "cmTileInput.h"

using namespace arch;

//  Creates a new TileInput.  
TileInput::TileInput(U32 ID, U32 setID, Tile *t, bool endTile,
    bool lastTriangle)
{
    //  Set vertex parameters.  
    triangleID = ID;
    setupID = setID; 
    tile = t;
    end = endTile;
    last = lastTriangle;
}

//  Gets the triangle identifier.  
U32 TileInput::getTriangleID()
{
    return triangleID;
}


//  Gets the setup triangle identifier.  
U32 TileInput::getSetupTriangleID()
{
    return setupID;
}

//  Gets the tile being carried.  
Tile *TileInput::getTile()
{
    return tile;
}

//  Returns if it is the last tile from the input parent tile.  
bool TileInput::isEndTile()
{
    return end;
}

/*  Returns if it is a valid tile.  Marks the end of a parent tile without
    any */

//  Gets if it is the last triangle/tile in the pipeline.  
bool TileInput::isLast()
{
    return last;
}
