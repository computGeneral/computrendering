/**************************************************************************
 *
 * Triangle Setup Output definition file. 
 *
 */

/**
 *
 *  @file cmTriangleSetupOutput.h
 *
 *  This file defines the Triangle Setup Output class.
 *
 *  This class is used to send setup triangles from the
 *  Triangle Setup mdu to the Fragment Generator mdu.
 *
 */
 
#ifndef _TRIANGLESETUPOUTPUT_

#define _TRIANGLESETUPOUTPUT_

#include "support.h"
#include "GPUType.h"
#include "DynamicObject.h"

namespace cg1gpu
{

/**
 *
 *  This class defines objects that carry the information
 *  about setup triangles from the Triangle Setup mdu
 *  to the Fragment Generator mdu.
 *
 *  This class inherits from the DynamicObject class that
 *  offers basic dynamic memory and signal tracing support
 *  functions.
 *
 */
 
class TriangleSetupOutput : public DynamicObject
{
private:

    U32 triangleID;      //  Triangle identifier (inside the batch).  
    U32 triSetupID;      //  The setup triangle ID (rasterizer behaviorModel).  
    bool culled;            //  The triangle has been culled.  
    bool last;              //  Last triangle mark.  

public:

    /**
     *
     *  Triangle Setup Output constructor.
     *
     *  Creates and initializes a triangle setup output.
     *
     *  @param id The triangle identifier (inside the batch).
     *  @param setupID The setup triangle identifier for the
     *  triangle inside the Rasterizer Behavior Model.
     *  @param last Triangle marked as last triangle.
     *
     *  @return An initialized triangle setup output.
     *
     */
     
    TriangleSetupOutput(U32 id, U32 setupID, bool last);
    
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
     *  Gets if the triangle has been culled in primitive assembly
     *  or triangle setup.
     *
     *  @param If the triangle has been culled.
     *
     */
     
    bool isCulled();
    
    /**
     *
     *  Gets if the triangle is marked as last triangle.
     *
     *  @param If the triangle is marked as the last one in the batch.
     *
     */
     
    bool isLast();

};

} // namespace cg1gpu

#endif
