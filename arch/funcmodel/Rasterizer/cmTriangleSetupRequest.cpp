/**************************************************************************
 *
 * Triangle Setup Request implementation file.
 *
 */

/**
 *
 *  @file TriangleSetupRequest.cpp
 *
 *  This file implements the TriangleSetupRequest class.
 *
 *  The TriangleSetupRequest class carries requests  to Triangle Setup
 *  for new setup triangles.
 *
 */


#include "cmTriangleSetupRequest.h"

using namespace cg1gpu;

//  Creates a new TriangleSetupRequest object.  
TriangleSetupRequest::TriangleSetupRequest(U32 trianglesRequested)
{
    //  Set number of triangles requested.  
    request = trianglesRequested;

    //  Set color for tracing.  
    setColor(0);

    setTag("TrSReq");
}

//  Return the number of triangles requested.  
U32 TriangleSetupRequest::getRequest()
{
    return request;
}

