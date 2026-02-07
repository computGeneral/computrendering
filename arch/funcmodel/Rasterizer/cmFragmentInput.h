/**************************************************************************
 *
 * Fragment Input implementation file.
 *
 */

#ifndef _FRAGMENTINPUT_

#define _FRAGMENTINPUT_

#include "support.h"
#include "GPUType.h"
#include "DynamicObject.h"
#include "Fragment.h"

namespace cg1gpu
{

/**
 *
 *  @file cmFragmentInput.h
 *
 *  This file defines the Fragment Input class.
 *
 *  This class is used to carry fragment information between
 *  Triangle Traversal and the Interpolator unit up to the
 *  Fragment FIFO and Pixel Shader.
 *
 */

/**
 *
 *  This class defines objects that carry fragment data
 *  from the Fragment Generator/Triangle Traversal to the
 *  Fragment FIFO and Pixel Shader unit.
 *
 *  This class inherits from the DynamicObject class that
 *  offers basic dynamic memory and signal tracing support
 *  functions.
 *
 */

class FragmentInput : public DynamicObject
{

private:

    U32 triangleID;          //  Triangle identifier (inside the current batch).  
    U32 setupTriangle;       //  The setup triangle identifier (in the rasterizer behaviorModel).  
    Fragment *fr;               //  Pointer to the fragment object with the fragment data.  
    Vec4FP32 *attributes;      //  Pointer to the fragment interpolated attributes.  
    bool culled;                //  Flag that keeps if the fragment has been culled in any of the test stages (alpha, Z, stencil, ...).  
    TileIdentifier tileID;      //  The fragment tile identifier.  
    U32 stampUnit;           //  Identifier of the stamp unit to which the fragment is assigned.  
    U32 shaderUnit;          //  Identifier of the shader unit to which the fragment is assigned.  
    U64 startCycle;          //  Stores the cycle when the fragment was created (or any other reference initial cycle).  
    U64 startCycleShader;    //  Cycle in which the fragment was issued into the shader unit.  
    U32 shaderLatency;       //  Stores the cycles spent inside the shader unit by the fragment.  

public:

    /**
     *
     *  Fragment Input constructor.
     *
     *  Creates and initializes a fragment input.
     *
     *  @param id The triangle identifier.
     *  @param setupID The setup triangle identifier.
     *  @param fr Pointer to the fragment object with the data.
     *  @param tileID The fragment tile identifier.
     *  @param stampUnitID Identifier of the stamp unit to which the fragment has been assigned.
     *
     *  @return An initialized fragment input.
     *
     */

    FragmentInput(U32 id, U32 setupID, Fragment *fr, TileIdentifier tid, U32 stampUnitID);

    /**
     *
     *  Gets the fragment triangle identifier (inside the batch).
     *
     *  @return The triangle identifier.
     *
     */

    U32 getTriangleID() const;

    /**
     *
     *  Gets the fragment setup triangle identifier (rasterizer behaviorModel).
     *
     *  @return The setup triangle identifier.
     *
     */

    U32 getSetupTriangle() const;

    /**
     *
     *  Gets the fragment input Fragment object.
     *
     *  @return The fragment input Fragment object.
     *
     */

    Fragment *getFragment() const;

    /**
     *
     *  Gets the fragment interpolated attributes.
     *
     *  @return A pointer to the fragment interpolated attribute array.
     *
     */

    Vec4FP32 *getAttributes() const;

    /**
     *
     *  Sets the fragment interpolated attributes.
     *
     *  @param attr The interpolated fragment attributes.
     *
     */

    void setAttributes(Vec4FP32 *attr);

    /**
     *
     *  Sets the fragment cull flag.
     *
     *  @param cull The new value of the fragment cull flag.
     *
     */

    void setCull(bool cull);

    /**
     *
     *  Returns if the fragment has been culled in any stage
     *  of the fragment pipeline (scissor, alpha, z, stencil, ...).
     *
     *  @return The fragment cull flag.
     *
     */

    bool isCulled() const;

    /**
     *
     *  Returns the identifier of the stamp unit/pipe to which the fragment has
     *  been assigned.
     *
     *  @return The identifier of the stamp unit for the fragment.
     *
     */

    U32 getStampUnit() const;

    /**
     *
     *  Sets the fragment start cycle.
     *
     *  @param startCycle The initial cycle for the fragment.
     *
     */

    void setStartCycle(U64 cycle);

    /**
     *
     *  Gets the fragment start cycle.
     *
     *  @return The fragment start cycle.
     *
     */

    U64 getStartCycle() const;

    /**
     *
     *  Sets the fragment start cycle in the shader.
     *
     *  @param cycle The initial cycle for the fragment in the shader.
     *
     */

    void setStartCycleShader(U64 cycle);

    /**
     *
     *  Gets the fragment start cycle in the shader.
     *
     *  @return The fragment start cycle in the shader.
     *
     */

    U64 getStartCycleShader() const;

    /**
     *
     *  Sets the fragment shader latency (cycles spent in the shader).
     *
     *  @param cycle Number of cycles the fragment spent in the shader.
     *
     */

    void setShaderLatency(U32 cycle);

    /**
     *
     *  Gets the fragment shader latency (cycles spent in the shader).
     *
     *  @return The number of cycles that the fragment spent inside the shader.
     *
     */

    U32 getShaderLatency() const;

    /**
     *
     *  Sets the fragment tile identifier.
     *
     *  @param tileID The tile identifier for the fragment
     *
     */

    void setTileID(TileIdentifier tileID);

    /**
     *
     *  Gets the fragment tile identifier.
     *
     *  @return The fragment tile identifier.
     *
     */

    TileIdentifier getTileID() const;

    /**
     *
     *  Sets the shader unit to which the fragment is going to be assigned.
     *
     *  @param unit Identifier of the shader unit the fragment is assigned to.
     *
     */

     void setShaderUnit(U32 unit);

     /**
      *
      *  Gets the shader unit to which the fragment is assigned.
      *
      *  @return The identifier of the shader unit to which the fragment has been assigned.
      *
      */

    U32 getShaderUnit() const;

};

} // namespace cg1gpu

#endif
