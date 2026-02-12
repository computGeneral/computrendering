/**************************************************************************
 *
 * Triangle Setup Input definition file.
 *
 */

/**
 *
 *  @file TriangleSetupInput.cpp
 *
 *  This file implements the Triangle Setup Input class.
 *
 */

#include "cmTriangleSetupInput.h"

using namespace arch;

//  Creates a new TriangleSetupInput.  
TriangleSetupInput::TriangleSetupInput(U32 ID, Vec4FP32 *attrib1,
    Vec4FP32 *attrib2, Vec4FP32 *attrib3, bool lastTriangle)
{
    //  Set vertex parameters.  
    triangleID = ID;
    attributes[0] = attrib1;
    attributes[1] = attrib2;
    attributes[2] = attrib3;
    rejected = FALSE;
    last = lastTriangle;
    preBound = false;

    setTag("TrSIn");
}

//  Gets the triangle identifier.  
U32 TriangleSetupInput::getTriangleID()
{
    return triangleID;
}

//  Gets the triangle vertex attributes.  
Vec4FP32 *TriangleSetupInput::getVertexAttributes(U32 vertex)
{
    //  Check vertex number.  
    CG_ASSERT_COND(!(vertex > 2), "Undefined vertex number.");
    return attributes[vertex];
}

//  Sets the triangle as rejected/culled.  
void TriangleSetupInput::reject()
{
    rejected = TRUE;
}


//  Gets if the triangle was rejected/culled.  
bool TriangleSetupInput::isRejected()
{
    return rejected;
}

//  Gets if it is the last triangle in the pipeline.  
bool TriangleSetupInput::isLast()
{
    return last;
}

//  Sets the triangle as pre-bound triangle.  
void TriangleSetupInput::setPreBound()
{
    preBound = true;
}

//  Returns if pre-bound triangle.  
bool TriangleSetupInput::isPreBound()
{
    return preBound;
}

//  Sets the triangle ID in Rasterizer behaviorModel.  
void TriangleSetupInput::setSetupID(U32 setupId)
{
    setupID = setupId;
}

//  Gets the triangle ID in Rasterizer behaviorModel.  
U32 TriangleSetupInput::getSetupID()
{
    return setupID;
}

