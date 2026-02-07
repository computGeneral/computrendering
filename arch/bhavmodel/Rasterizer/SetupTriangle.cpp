/**************************************************************************
 *
 * Setup Triangle implementation file.
 *
 */

/**
 *
 *  @file SetupTriangle.cpp
 *
 *  Implements the SetupTriangle class.
 *
 *  This class describes and stored the data associated with
 *  triangles in the Rasterization and Triangle Setup stages
 *  of the 3D graphic pipeline.
 *
 */

#include "SetupTriangle.h"

using namespace cg1gpu;

//  Setup Triangle constructor.  
SetupTriangle::SetupTriangle(Vec4FP32 *vattr1, Vec4FP32 *vattr2,
    Vec4FP32 *vattr3)
{
    U32 i;

    //  Set triangle vertex and attributes.  
    vertex1 = vattr1;
    vertex2 = vattr2;
    vertex3 = vattr3;

    //  Reset saved positions valid vector.  
    for(i = 0; i < MAX_SAVED_POSITIONS; i++)
        isSaved[i] = FALSE;

    //  Reset last triangle fragment flag.  
    lastFragmentFlag = FALSE;

    //  Reset setup triangle reference counter.  
    references = 1;

    //  Triangles are not pre-bound by default.  
    preBoundTriangle = false;

    setTag("SetTri");
}

//  Setup triangle destructor.  
SetupTriangle::~SetupTriangle()
{
    //  Delete triangle vertex attributes.  
    delete[] vertex1;
    delete[] vertex2;
    delete[] vertex3;
}

//  Sets the triangle three edge equations.  
void SetupTriangle::setEdgeEquations(F64 *edgeEq1, F64 *edgeEq2,
    F64 *edgeEq3)
{
    //  Set triangle edge equations.  
    edge1[0] = edgeEq1[0];
    edge1[1] = edgeEq1[1];
    edge1[2] = edgeEq1[2];

    edge2[0] = edgeEq2[0];
    edge2[1] = edgeEq2[1];
    edge2[2] = edgeEq2[2];

    edge3[0] = edgeEq3[0];
    edge3[1] = edgeEq3[1];
    edge3[2] = edgeEq3[2];

}

//  Sets the Z interpolation equation for the triangle.  
void SetupTriangle::setZEquation(F64 *zeq)
{
    //  Set the triangle z interpolation equation.  
    zEq[0] = zeq[0];
    zEq[1] = zeq[1];
    zEq[2] = zeq[2];
}

//  Returns the triangle three edge equations.  
void SetupTriangle::getEdgeEquations(F64 *edgeEq1, F64 *edgeEq2,
    F64 *edgeEq3)
{
    //  Get triangle edge equations.  
    edgeEq1[0] = edge1[0];
    edgeEq1[1] = edge1[1];
    edgeEq1[2] = edge1[2];

    edgeEq2[0] = edge2[0];
    edgeEq2[1] = edge2[1];
    edgeEq2[2] = edge2[2];

    edgeEq3[0] = edge3[0];
    edgeEq3[1] = edge3[1];
    edgeEq3[2] = edge3[2];

}

//  Returns the Z interpolation equation for the triangle.  
void SetupTriangle::getZEquation(F64 *zeq)
{
    //  Get the triangle z interpolation equation.  
    zeq[0] = zEq[0];
    zeq[1] = zEq[1];
    zeq[2] = zEq[2];
}

//  Updates the current position for fragment generation.  
void SetupTriangle::updatePosition(F64 *update, S32 xPos, S32 yPos)
{
    /*  Update current triangle rasterization position
        stored in C coefficients.  */
    edge1[2] = update[0];
    edge2[2] = update[1];
    edge3[2] = update[2];
    zEq[2] = update[3];

    //  Update raster position.  
    x = xPos;
    y = yPos;
}

//  Save start position.  
void SetupTriangle::saveRasterStart()
{
    F64 start[4];

    //  Get the initial C coefficients.  
    start[0] = edge1[2];
    start[1] = edge2[2];
    start[2] = edge3[2];
    start[3] = zEq[2];

    //  Save start raster position.  
    save(start, 0 , 0, RASTER_START_SAVE);
}

//  Up save.  
void SetupTriangle::saveUp(F64 *up)
{
    //  Save right position.  
    save(up, x, y + 1, UP_SAVE);
}

//  Down save.  
void SetupTriangle::saveDown(F64 *down)
{
    //  Save down position.  
    save(down, x, y - 1, DOWN_SAVE);
}

//  Right save.  
void SetupTriangle::saveRight(F64 *right)
{
    //  Save right position.  
    save(right, x + 1, y, RIGHT_SAVE);
}

//  Saves a rasterization position.  
void SetupTriangle::save(F64 *save, S32 x, S32 y,
    SavedPosition savePos)
{
    //  Save edge and z interpolation equation values.  
    savedEdge[savePos][0] = save[0];
    savedEdge[savePos][1] = save[1];
    savedEdge[savePos][2] = save[2];
    savedEdge[savePos][3] = save[3];

    //  Save raster position one from the current one.  
    savedRaster[savePos][0] = x;
    savedRaster[savePos][1] = y;

    //  Set saved position valid bit.  
    isSaved[savePos] = TRUE;
}

//  Returns if the right position is saved.  
bool SetupTriangle::isRightSaved()
{
    //  Returns the right saved position valid bit.  
    return isSaved[RIGHT_SAVE];
}

//  Returns if the down position is saved.  
bool SetupTriangle::isDownSaved()
{
    //  Returns the down saved position valid bit.  
    return isSaved[DOWN_SAVE];
}

//  Returns if the up position is saved.  
bool SetupTriangle::isUpSaved()
{
    //  Returns the up saved position valid bit.  
    return isSaved[UP_SAVE];
}

//  Returns if the left tile position is saved.  
bool SetupTriangle::isLeftTileSaved()
{
    //  Returns if the left tile position is saved.  
    return isSaved[TILE_LEFT_SAVE];
}

//  Returns if the right tile position is saved.  
bool SetupTriangle::isRightTileSaved()
{
    //  Returns if the right tile position is saved.  
    return isSaved[TILE_RIGHT_SAVE];
}

//  Returns if the down tile position is saved.  
bool SetupTriangle::isDownTileSaved()
{
    //  Returns if the down tile position is saved.  
    return isSaved[TILE_DOWN_SAVE];
}

//  Returns if the up tile position is saved.  
bool SetupTriangle::isUpTileSaved()
{
    //  Returns if the up tile position is saved.  
    return isSaved[TILE_UP_SAVE];
}


//  Restores right state.  
void SetupTriangle::restoreRight()
{
    //  Restored the right saved position.  
    restore(RIGHT_SAVE);
}

//  Restores down state.  
void SetupTriangle::restoreDown()
{
    //  Restored the down saved position.  
    restore(DOWN_SAVE);
}

//  Restores up state.  
void SetupTriangle::restoreUp()
{
    //  Restored the up saved position.  
    restore(UP_SAVE);
}

//  Restores left tile save position.  
void SetupTriangle::restoreLeftTile()
{
    //  Restored the left tile saved position.  
    restore(TILE_LEFT_SAVE);

    //  Change rasterization direction.  
    direction = CENTER_LEFT_DIR;

    //  Change tile rasterization direction.  
    tileDirection = (tileDirection & ~RIGHT_DIR) | LEFT_DIR;
}

//  Restores right tile save position.  
void SetupTriangle::restoreRightTile()
{
    //  Restored the right tile saved position.  
    restore(TILE_RIGHT_SAVE);

    //  Change rasterization direction.  
    direction = CENTER_RIGHT_DIR;

    //  Change tile rasterization direction.  
    tileDirection = (tileDirection & ~LEFT_DIR) | RIGHT_DIR;
}

//  Restores down tile save position.  
void SetupTriangle::restoreDownTile()
{
    //  Restored the down tile saved position.  
    restore(TILE_DOWN_SAVE);

    //  Change rasterization direction.  
    direction = DOWN_DIR;

    //  Change tile rasterization direction.  
    tileDirection = (tileDirection & ~UP_DIR) | DOWN_DIR | LEFT_DIR | RIGHT_DIR;
}

//  Restores up tile save position.  
void SetupTriangle::restoreUpTile()
{
    //  Restored the up tile saved position.  
    restore(TILE_UP_SAVE);

    //  Change rasterization direction.  
    direction = UP_DIR;

    //  Change tile rasterization direction.  
    tileDirection = (tileDirection & ~DOWN_DIR) | UP_DIR | LEFT_DIR | RIGHT_DIR;

}

//  Restores a saved rasterization position.  
void SetupTriangle::restore(SavedPosition save)
{
    //  Restore C coefficients saved position.  
    edge1[2] = savedEdge[save][0];
    edge2[2] = savedEdge[save][1];
    edge3[2] = savedEdge[save][2];
    zEq[2] = savedEdge[save][3];

    //  Restore raster position saved position.  
    x = savedRaster[save][0];
    y = savedRaster[save][1];

    //  Reset saved position valid bit.  
    isSaved[save] = FALSE;
}


//  Sets the triangle last fragment flag.  
void SetupTriangle::lastFragment()
{
    lastFragmentFlag = TRUE;
}


//  Returns if the triangle last fragment was generated.  
bool SetupTriangle::isLastFragment()
{
    return lastFragmentFlag;
}

//  Sets the pre-bound triangle flag.  
void SetupTriangle::setPreBound()
{
    preBoundTriangle = true;
}

//  Returns if the triangle is a pre-bound triangle.  
bool SetupTriangle::isPreBoundTriangle()
{
    return preBoundTriangle;
}

//  Set the triangle current raster position.  
void SetupTriangle::setRasterPosition(S32 xPos, S32 yPos)
{
    x = xPos;
    y = yPos;
}

//  Returns the triangle current raster position.  
void SetupTriangle::getRasterPosition(S32 &xPos, S32 &yPos)
{
    xPos = x;
    yPos = y;
}

//  Changes the triangle rasterization direction.  
void SetupTriangle::setDirection(RasterDirection dir)
{
    if (direction & BORDER_DIR)
    {
        direction = CENTER_DIR;
    }

    //  Check if it is center direction.  
    if (dir == CENTER_DIR)
    {
        //  Just set the direction.  
        direction = dir;
    }
    else if (dir == LEFT_DIR)
    {
        //  Set left direction, set right direction.  
        direction = direction | LEFT_DIR | RIGHT_DIR;
    }
    else if (dir == RIGHT_DIR)
    {
        //  Unset left direction and set right direction.  
        direction = (direction & ~LEFT_DIR) | RIGHT_DIR;
    }
    else if (dir == UP_DIR)
    {
        //  Set up direction.  
        direction = UP_DIR;
    }
    else if (dir == DOWN_DIR)
    {
        //  Set down direction.  
        direction = DOWN_DIR;
    }
    else
    {
        //  Compatibility with scanline fragment based algorithm.  
        direction = dir;
        //CG_ASSERT("Unsupported direction change.");
    }
}

//  Returns the triangle rasterization direction.  
U08 SetupTriangle::getDirection()
{
    return direction;
}

//  Changes the tile direction.  
void SetupTriangle::setTileDirection(RasterDirection dir)
{
    //  Check if it is center direction.  
    if (dir == CENTER_DIR)
    {
        //  Set also left and right directions .  
        tileDirection = dir | LEFT_DIR | RIGHT_DIR;
    }
    else if (dir == LEFT_DIR)
    {
        //  Set left direction, unset right direction.  
        tileDirection = (tileDirection | LEFT_DIR) & ~RIGHT_DIR;
    }
    else if (dir == RIGHT_DIR)
    {
        //  Unset left direction and set right direction.  
        tileDirection = (tileDirection & ~LEFT_DIR) | RIGHT_DIR;
    }
    else if (dir == UP_DIR)
    {
        //  Unset down direction and set up direction.  
        tileDirection = (tileDirection & ~DOWN_DIR) | UP_DIR | LEFT_DIR | RIGHT_DIR;
    }
    else if (dir == DOWN_DIR)
    {
        //  Unset up direction and set down direction.  
        tileDirection = (tileDirection & ~UP_DIR) | DOWN_DIR | LEFT_DIR | RIGHT_DIR;
    }
    else
    {
        CG_ASSERT("Unsupported tile direction change.");
    }
}

//  Returns if a saved tile position must be calculated for a given direction.  
bool SetupTriangle::getTileDirection(RasterDirection dir)
{
    return ((tileDirection & dir) != 0);
}


//  Returns the pointer to a vertex attribute.  
Vec4FP32 *SetupTriangle::getAttribute(U32 vertex, U32 attribute)
{
    switch(vertex)
    {
        case 0:
            return &vertex1[attribute];
        case 1:
            return &vertex2[attribute];
        case 2:
            return &vertex3[attribute];
    }
    CG_ASSERT("Wrong vertex identifier.");
    return 0;
}

//  Returns the address of the three vertex attribute arrays.  
void SetupTriangle::getVertexAttributes(Vec4FP32 *&vAttr1, Vec4FP32 *&vAttr2,
    Vec4FP32 *&vAttr3)
{
    vAttr1 = vertex1;
    vAttr2 = vertex2;
    vAttr3 = vertex3;
}

//  Set the triangle area.  
void SetupTriangle::setArea(F64 tArea)
{
    area = tArea;
}

//  Set the triangle size in percentage of screen area.  
void SetupTriangle::setScreenPercent(F64 size)
{
    screenArea = size;
}

//  Returns the triangle area.  
F64 SetupTriangle::getArea()
{
    return area;
}

//  Gets the triangle size in percentage of screen area.  
F64 SetupTriangle::getScreenPercent()
{
    return screenArea;
}

//  Sets the vertex non-homogeneous position coordinates (divided by W).  
void SetupTriangle::setNHVtxPosition(U32 vtx, Vec4FP32 nHPos)
{
    CG_ASSERT_COND(!( vtx > 2 ), "Triangle's vertex indentifier is out of [0-2] range");    nHVtxPos[vtx] = nHPos;
}

//  Gets the vertex position coordinates divided by W.  
void SetupTriangle::getNHVtxPosition(U32 vtx, Vec4FP32& nHPosOut)
{
    CG_ASSERT_COND(!( vtx > 2 ), "Triangle's vertex indentifier is out of [0-2] range");    nHPosOut = nHVtxPos[vtx];
}

//  Inverts the triangle edge equations signs.  
void SetupTriangle::invertEdgeEquations()
{
    //  Change the signs of the first edge equation coefficients.  
    edge1[0] = - edge1[0];
    edge1[1] = - edge1[1];
    edge1[2] = - edge1[2];

    //  Change the signs of the second edge equation coefficients.  
    edge2[0] = - edge2[0];
    edge2[1] = - edge2[1];
    edge2[2] = - edge2[2];

    //  Change the signs of the third edge equation coefficients.  
    edge3[0] = - edge3[0];
    edge3[1] = - edge3[1];
    edge3[2] = - edge3[2];
}

//  Adds a new reference to the setup triangle object.  
void SetupTriangle::newReference()
{
    //  Increment reference counter.  
    references++;
}

//  Deletes a reference for the setup triangle object.  
void SetupTriangle::deleteReference()
{
    //  Decrement reference counter.  
    references--;

    //  Check if no more references.  
    if (references == 0)
    {
        //  Self destruct.  
        delete this;
    }
}

//  Sets the triangle first stamp flag.  
void SetupTriangle::setFirstStamp(bool value)
{
    firstStamp = value;
}

//  Returns the first stamp flag value.  
bool SetupTriangle::isFirstStamp()
{
    return firstStamp;
}

//  Set the bounding mdu.  
void SetupTriangle::setBoundingBox(S32 x0, S32 y0, S32 x1, S32 y1)
{
    minX = x0;
    minY = y0;
    maxX = x1;
    maxY = y1;
}

//  Sets the subpixel bounding mdu  
void SetupTriangle::setSubPixelBoundingBox(FixedPoint subXmin, FixedPoint subYmin, FixedPoint subXmax, FixedPoint subYmax)
{
    subXMin = subXmin;
    subYMin = subYmin;
    subXMax = subXmax;
    subYMax = subYmax;
}

//  Get the bounding mdu.  
void SetupTriangle::getBoundingBox(S32 &x0, S32 &y0, S32 &x1, S32 &y1)
{
    x0 = minX;
    y0 = minY;
    x1 = maxX;
    y1 = maxY;
}

//  Gets the subpixel bounding mdu  
void SetupTriangle::getSubPixelBoundingBox(FixedPoint& subXmin, FixedPoint& subYmin, FixedPoint& subXmax, FixedPoint& subYmax)
{
    subXmin = subXMin;
    subYmin = subYMin;
    subXmax = subXMax;
    subYmax = subYMax;
}


