/**************************************************************************
 *
 * Triangle Setup Input implementation file. 
 *
 */

#ifndef _TRIANGLESETUPINPUT_

#define _TRIANGLESETUPINPUT_

#include "support.h"
#include "GPUType.h"
#include "DynamicObject.h"

namespace cg1gpu
{

/**
 * 
 *  @file cmTriangleSetupInput.h 
 *
 *  This file defines the Triangle Setup Input class.
 *
 */
  
/**
 *
 *  This class defines objects that carry triangle data
 *  from the Primitive Assembly to the Triangle Setup
 *  unit.
 *
 *  It is also used by the MicroPolygon Rasterizer to
 *  carry data from Primitive Assembly to Triangle Bound,
 *  and from Triangle Bound to Triangle Setup.
 *   
 *
 *  This class inherits from the DynamicObject class that
 *  offers basic dynamic memory and signal tracing support
 *  functions.
 *
 */
 
class TriangleSetupInput : public DynamicObject
{

private:

    U32 triangleID;          //  Triangle identifier.  
    Vec4FP32 *attributes[3];   //  Triangle vertex attributes.  
    bool rejected;              //  The triangle has been rejected/culled in a previous stage.  
    bool last;                  //  Last triangle signal.  
    U32 setupID;             //  The triangle identifier in Rasterizer behaviorModel.  
    bool preBound;

public:

    /**
     *
     *  Triangle Setup Input constructor.
     *
     *  Creates and initializes a triangle setup input.
     *
     *  @param id The triangle identifier.
     *  @param attrib1 The triangle first vertex attributes.
     *  @param attrib2 The triangle second vertex attributes.
     *  @param attrib3 The triangle third vertex attributes.
     *  @param last Last triangle signal.
     *
     *  @return An initialized triangle setup input.
     *
     */
     
    TriangleSetupInput(U32 id, Vec4FP32 *attrib1, Vec4FP32 *attrib2, 
        Vec4FP32 *attrib3, bool last);
    
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
     *  Gets the triangle vertex attributes.
     *
     *  @param vertex The triangle vertex number (0, 1 or 2) for
     *  which the vertex attributes are requested.
     *
     *  @return A pointer to the triangle vertex attribute array.
     *
     */

    Vec4FP32 *getVertexAttributes(U32 vertex);
    
    /**
     *
     *  Sets the triangle as rejected/culled.
     *
     *
     */
    
    void reject();
   
    /**
     *
     *  Get if the triangle has been rejected/culled in a previous
     *  pipeline stage.
     *
     *  @return If the triangle has been rejected.
     *
     */
     
    bool isRejected();
    
    /**
     * 
     *  Check if the last triangle flag is enabled for this triangle.
     *
     *  @return If the last triangle signal is enabled.
     *
     */
    
    bool isLast();

    /**
     *
     *  Sets the triangle as pre-bound triangle.
     *
     */

    void setPreBound();

    /**
     *
     *  Returns if pre-bound triangle.
     *
     *  @return If pre-bound triangle.
     */
     
    bool isPreBound();

    /**
     *
     *  Sets the triangle ID in Rasterizer behaviorModel.
     *
     *  @param setupID The triangle ID in Rasterizer behaviorModel.
     *
     */

    void setSetupID(U32 setupID);

    /**
     *
     *  Gets the triangle ID in Rasterizer behaviorModel.
     *
     *  @return The triangle ID
     *
     */

    U32 getSetupID();
 
};

} // namespace cg1gpu

#endif
