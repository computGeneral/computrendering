/**************************************************************************
 *
 * Triangle Setup Request definition file.
 *
 */

/**
 *
 *  @file cmTriangleSetupRequest.h
 *
 *  This file defines the TriangleSetupRequest class.
 *
 *  The Triangle Setup Request class is used to
 *  to request a setup triangle to the Triangle Setup unit.
 *
 */

#ifndef _TRIANGLESETUPREQUEST_
#define _TRIANGLESETUPREQUEST_

#include "DynamicObject.h"

namespace cg1gpu
{

/**
 *
 *  This class is used to request setup triangles to the
 *  Triangle Setup mdu and request bound triangles to the
 *  Triangle Bound mdu.
 *
 *  Inherits from Dynamic Object that offers dynamic memory
 *  support and tracing capabilities.
 *
 */


class TriangleSetupRequest : public DynamicObject
{
private:
     
    U32 request;     //  Number of requested triangles.  

public:

    /**
     *
     *  Creates a new TriangleSetupRequest object.
     *
     *
     *  @param trianglesRequested The number of requested triangles.
     *  @return A new initialized TriangleSetupRequest object.
     *
     */
     
    TriangleSetupRequest(U32 trianglesRequested);

    /**
     *
     *  Get the number of requested triangles.
     *
     *  @return The number of requested triangles.
     *
     */
 
    U32 getRequest();
    
};

} // namespace cg1gpu

#endif
