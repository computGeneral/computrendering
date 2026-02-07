/**************************************************************************
 *
 * Primitive Assembly Request implementation file.
 *
 */

/**
 *
 *  @file PrimitiveAssemblyRequest.cpp
 *
 *  This file implements the PrimitiveAssemblyRequest class.
 *
 *  The PrimitiveAssemblyRequest class carries requests from
 *  the Primitive Assembly unit to the cmoStreamController Commmit for
 *  transformed vertexes.
 *
 */


#include "cmPrimitiveAssemblyRequest.h"

using namespace cg1gpu;

//  Creates a new PrimitiveAssemblyRequest object.  
PrimitiveAssemblyRequest::PrimitiveAssemblyRequest(U32 trianglesRequested)
{
    //  Set number of triangles requested.  
    request = trianglesRequested;

    //  Set color for tracing.  
    setColor(request);

    setTag("PAsReq");
}

//  Return the number of triangles requested.  
U32 PrimitiveAssemblyRequest::getRequest()
{
    return request;
}

