/**************************************************************************
 *
 * Tile definition file.
 *
 */

/**
 *
 *  @file Tile.h
 *
 *  Defines the Tile class.
 *
 *  This class describes and stores the data associated with
 *  a 2D tile inside the 3D graphic pipeline.
 *
 *
 */

#ifndef _TILE_

#define _TILE_

#include "GPUType.h"
#include "SetupTriangle.h"
#include "DynamicMemoryOpt.h"

namespace cg1gpu
{

/**
 *
 *  Defines the maximum number triangles supported for evaluation in a tile.
 *
 */

static const U32 MAX_TRIANGLES = 32;

/**
 *
 *  Defines a 2D tile of fragments.
 *
 *  This class stores information about tiles of the viewport
 *  that are evaluated in the rasterization stage of 3D GPU
 *  simulator.
 *
 *  This class inherits from the DynamicMemoryOpt object that provides custom optimized
 *  memory allocation and deallocation support
 *
 */

class Tile : public DynamicMemoryOpt
{

private:

    U32 x;                           //  Tile start x position.  
    U32 y;                           //  Tile start y position.  
    U32 level;                       //  Tile level/size.  
    F64 sEq[MAX_TRIANGLES * 3];      //  Storage for the three edge equation values per triangle.  
    F64 *edgeEq[MAX_TRIANGLES];      //  Three edge equations value at the tile start position for the triangles being evaluated.  
    F64 zEq[MAX_TRIANGLES];          //  z/w equation value at the tile start point for the triangles being evaluated.  
    SetupTriangle *triangle[MAX_TRIANGLES]; //  Pointer to the triangle or triangles to be evaluated in the tile.  
    U32 numTriangles;                //  Number of triangles being evaluated.  
    bool inside[MAX_TRIANGLES];         //  Stores if the tile is inside each of the triangles being rasterized.  
    U32 nextTriangle;                //  Stores the next triangle for which to generate fragments.  

public:

    /**
     *
     *  Tile constructor.
     *
     *  Creates and initializes a new Tile.
     *
     *  @param triangle Pointer to the triangle(s) to be evaluated/rasterized in the tile.
     *  @param x Start horizontal coordinate of the tile inside the viewport.
     *  @param y Start vertical coordinate of the tile inside the viewport.
     *  @param edgeq Array with the value of the triangle edge equations at the
     *  tile start point.
     *  @param zeq Value of the z/w equation at the tile start point.
     *  @param level Level/Size of the tile inside the viewport.
     *
     *  @return A new initialized Tile.
     *
     */

    Tile(SetupTriangle *triangle, U32 x, U32 y, F64 *edgeq, F64 zeq,
        U32 level);

    /**
     *
     *  Tile constructor.
     *
     *  Creates and initializes a new Tile.
     *
     *  @param triangle Pointer to an array of triangles to be evaluated/rasterized in the tile.
     *  @param numTriangles Number of triangles to evaluate in the tile.
     *  @param x Start horizontal coordinate of the tile inside the viewport.
     *  @param y Start vertical coordinate of the tile inside the viewport.
     *  @param edgeq Array of float point arrays with the value of the triangle edge equations at the
     *  tile start point for each triangle being evaluated.
     *  @param zeq Array with the value of the z/w equation at the tile start point for each triangle
     *  being evaluated.
     *  @param level Level/Size of the tile inside the viewport.
     *
     *  @return A new initialized Tile.
     *
     */

    Tile(SetupTriangle **triangle, U32 numTriangles, U32 x, U32 y, F64 **edgeq, F64 *zeq,
        U32 level);

    /**
     *
     *  Tile copy constructor.
     *
     *  @param input Reference to the source Tile to be copied into the new Tile.
     *
     *  @return A new initialized Tile that is a copy of the source Tile.
     *
     */

    Tile(const Tile &input);

    /**
     *
     *  Tile destructor.
     *
     */

    ~Tile();

    /**
     *
     *  Return the tile start horizontal position.
     *
     *  @return The tile start horizontal position.
     *
     */

    U32 getX();

    /**
     *
     *  Returns the tile start vertical position.
     *
     *  @return The tile start vertical position.
     *
     */

    U32 getY();

    /**
     *
     *  Gets the pointer to the Setup Triangle (first) that is being evaluated/rasterized in the current tile.
     *
     *  @return Pointer to the (first) setup triangle being evaluated/rasterized in the tile.
     *
     */

    SetupTriangle *getTriangle();

    /**
     *
     *  Gets the pointer to the specified setup triangle that is being evaluated/rasterized in the current tile.
     *
     *  @param id Identifier (order/position) of the triangle requested.
     *
     *  @return Pointer to the setup triangle being evaluated/rasterized in the tile.
     *
     */

    SetupTriangle *getTriangle(U32 id);

    /**
     *
     *  Gets the value of the triangle edge equations at the tile start point of the first triangle being evaluated.
     *
     *  @return A pointer to the value of the triangle edge equations at the tile start point.
     *
     */

    F64 *getEdgeEquations();

    /**
     *
     *  Gets the value of the triangle edge equations at the tile start point of the specified triangle being evaluated.
     *
     *  @param id Triangle identifier inside the tile which edge equations are requested.
     *
     *  @return A pointer to the value of the triangle edge equations at the tile start point.
     *
     */

    F64 *getEdgeEquations(U32 id);

    /**
     *
     *  Gets the value of the z/w triangle equation at the tile start point for the first triangle in the tile.
     *
     *  @return The value of the z/w equation at the tile start point.
     *
     */

    F64 getZEquation();

    /**
     *
     *  Gets the value of the z/w triangle equation at the tile start point for the specified triangle in the tile.
     *
     *  @param id Identifier of the triangle inside the tile which z equation value is requested.
     *
     *  @return The value of the z/w equation at the tile start point.
     *
     */

    F64 getZEquation(U32 id);


    /**
     *
     *  Get the current tile size/level.
     *
     *  @return The tile size/level.
     *
     */

    U32 getLevel();

    /**
     *
     *  Get the number of triangles being evaluated in the tile.
     *
     *  @eturn The number of triangles being evaluated in the tile.
     *
     */

    U32 getNumTriangles();

    /**
     *
     *  Get the vector with the tile inside triangle flags.
     *
     *  @return Pointer to the vector with the tile inside triangle flag.
     *
     */

    bool *getInside();

    /**
     *
     *  Get identifier of the next triangle in the tile for which to generate
     *  fragments.
     *
     *  @return Identifier of the next triangle in the tile for which to generate
     *  fragments.
     *
     */

    U32 getNextTriangle();

    /**
     *
     *  Sets the identifier of the next triangle in the tile for which to generate
     *  fragments.
     *
     *  @param next Identifier of the next triangle for which fragments will be
     *  generated.
     *
     */

    void setNextTriangle(U32 next);
};

} // namespace cg1gpu

#endif
