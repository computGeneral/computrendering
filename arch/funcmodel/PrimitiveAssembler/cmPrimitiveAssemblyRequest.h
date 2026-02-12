/**************************************************************************
 *
 * Primitive Assembly Request definition file.
 *
 */

/**
 *
 *  @file cmPrimitiveAssemblyRequest.h
 *
 *  This file defines the PrimitiveAssemblyRequest class.
 *
 *  The Primitive Assembly Request class is used to
 *  to request a vertex by the Primitive Assembly unit to the
 *  cmoStreamController Commit unit.
 *
 */

#ifndef _PRIMITIVEASSEMBLYREQUEST_
#define _PRIMITIVEASSEMBLYREQUEST_

#include "DynamicObject.h"

namespace arch
{

/**
 *
 *  This class is used to request transformed vertexes to the
 *  cmoStreamController Commit mdu.
 *
 *  Inherits from Dynamic Object that offers dynamic memory
 *  support and tracing capabilities.
 *
 */


class PrimitiveAssemblyRequest : public DynamicObject
{
private:

    U32 request;     //  Number of requested triangles.  

public:

    /**
     *
     *  Creates a new PrimitiveAssemblyRequest object.
     *
     *  @param request Number of requested triangles.
     *
     *  @return A new initialized PrimitiveAssemblyRequest object.
     *
     */

    PrimitiveAssemblyRequest(U32 request);

    /**
     *
     *  Get the number of requested triangles.
     *
     *  @return The number of requested triangles.
     *
     */

    U32 getRequest();

};

} // namespace arch


#endif
