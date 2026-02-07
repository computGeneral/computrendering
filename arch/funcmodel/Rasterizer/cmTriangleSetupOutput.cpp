/**************************************************************************
 *
 * Triangle Setup Output implementation file.
 *
 */

/**
 *
 *  @file TriangleSetupInput.cpp
 *
 *  This file implements the Triangle Setup Output class.
 *
 *  This class transports setup triangles from Triangle
 *  Setup to Fragment Generation/Triangle Traversal.
 *
 */

#include "cmTriangleSetupOutput.h"
#include <stdio.h>
using namespace cg1gpu;

//  Creates a new TriangleSetupOutput.  
TriangleSetupOutput::TriangleSetupOutput(U32 ID, U32 setupID, bool lastTri)
{
    //  Set triangle parameters.  
    triangleID = ID;
    triSetupID = setupID;
    culled = FALSE;
    last = lastTri;

    sprintf((char *) getInfo(), "triID %d triSetID %d", ID, setupID);

    setTag("TrSOut");
}

//  Gets the triangle identifier (batch).  
U32 TriangleSetupOutput::getTriangleID()
{
    return triangleID;
}

//  Gets the setup triangle identifier (rasterizer behaviorModel).  
U32 TriangleSetupOutput::getSetupTriangleID()
{
    return triSetupID;
}

//  Gets if the triangle has been culled in previous stage.  
bool TriangleSetupOutput::isCulled()
{
    return culled;
}

//  Returns if the triangle is marked as the last in the batch.  
bool TriangleSetupOutput::isLast()
{
    return last;
}



