/**************************************************************************
 *
 * Shader Input implementation file.
 *
 */

#ifndef _SHADERINPUT_

#define _SHADERINPUT_

#include "support.h"
#include "GPUType.h"
#include "DynamicObject.h"

namespace cg1gpu
{

/**
 *
 *  Defines the different shader inputs.
 *
 */

enum ShaderInputMode
{
    SHIM_VERTEX,                 //  Vertex input.  
    SHIM_FRAGMENT,               //  Fragment input.  
    SHIM_TRIANGLE,               //  Triangle input.  
    SHIM_MICROTRIANGLE_FRAGMENT  //  Microtriangle fragment input.  
};

struct ShaderInputID
{
    union 
    {
        struct
        {
            U32 instance;
            U32 index;
        } vertexID;
        
        struct
        {
            U32 triangle;
            U32 x;
            U32 y;            
        } fragmentID;
        
        U32 inputID;
    } id;
    
    ShaderInputID()
    {
        id.inputID = 0;
    }
    
    ShaderInputID(U32 inputID)
    {
        id.inputID = inputID;
    }
    
    ShaderInputID(U32 instance, U32 index)
    {
        id.vertexID.instance = instance;
        id.vertexID.index = index;
    }
    
    ShaderInputID(U32 triangle, U32 x, U32 y)
    {
        id.fragmentID.triangle = triangle;
        id.fragmentID.x = x;
        id.fragmentID.y = y;
    }
    
    ShaderInputID &operator=(U32 &in)
    {
        id.inputID = in;
        
        return *this;
    }
    
    ShaderInputID(U32 &in)
    {
        id.inputID = in;
    }
    
    bool operator==(U32 &in) const
    {
        return (id.inputID == in);
    }
};

/**
 *
 *  This class defines objects that carry shader input
 *  and output data from and to the Shaders.
 *
 *
 */

class ShaderInput : public DynamicObject
{

private:

    ShaderInputID id;       //  Shader Input identifier (it uses to be its index).  
    U32 entry;           //  Destination buffer entry where is going to be stored the input after shading.  
    U32 unit;            //  Unit destination buffer where is going to be stored the input after shading.  
    Vec4FP32 *attributes;  //  Shader Input attributes.  
    bool kill;              //  Flag storing if the execution of this shader input was aborted (killed).  
    bool isAFragment;       //  Stores if the shader input/output is for a fragment (TRUE) or for a vertex (FALSE).  
    bool last;              //  Marks the shader input as the last in a batch (only for vertex inputs!!).  
    ShaderInputMode mode;   //  Stores the shader input mode.  
    U64 startCycleShader;//  Cycle in which the shader input was issued into the shader unit.  
    U32 shaderLatency;   //  Stores the cycles spent inside the shader unit.  

    //  MicroPolygon Rasterizer  
    U32 zUnresolvedPosition;  // The ZUnresolved entry location.  

public:

    /**
     *
     *  Shader Input constructor.
     *
     *  Creates and initializes a shader input.
     *
     *  @param id The shader input identifier.
     *  @param unit Unit destination buffer where is going to be stored after shading.
     *  @param entry Destination buffer entry where is going to be stored after shading.
     *  @param attrib The shader input attributes.
     *
     *  @return An initialized shader input.
     *
     */

    ShaderInput(U32 id, U32 unit, U32 entry, Vec4FP32 *attrib);

    /**
     *
     *  Shader Input constructor.
     *
     *  Creates and initializes a shader input.
     *
     *  @param id The shader input identifier.
     *  @param unit Unit destination buffer where is going to be stored after shading.
     *  @param entry Destination buffer entry where is going to be stored after shading.
     *  @param attrib The shader input attributes.
     *  @param mode Shader input mode (vertex, fragment, triangle, ...).
     *
     *  @return An initialized shader input.
     *
     */

    ShaderInput(U32 id, U32 unit, U32 entry, Vec4FP32 *attrib, ShaderInputMode mode);

    /**
     *
     *  Shader Input constructor.
     *
     *  Creates and initializes a shader input.
     *
     *  @param id The shader input identifier.
     *  @param unit Unit destination buffer where is going to be stored after shading.
     *  @param entry Destination buffer entry where is going to be stored after shading.
     *  @param attrib The shader input attributes.
     *
     *  @return An initialized shader input.
     *
     */

    ShaderInput(ShaderInputID id, U32 unit, U32 entry, Vec4FP32 *attrib);

    /**
     *
     *  Shader Input constructor.
     *
     *  Creates and initializes a shader input.
     *
     *  @param id The shader input identifier.
     *  @param unit Unit destination buffer where is going to be stored after shading.
     *  @param entry Destination buffer entry where is going to be stored after shading.
     *  @param attrib The shader input attributes.
     *  @param mode Shader input mode (vertex, fragment, triangle, ...).
     *
     *  @return An initialized shader input.
     *
     */

    ShaderInput(ShaderInputID id, U32 unit, U32 entry, Vec4FP32 *attrib, ShaderInputMode mode);

    /**
     *
     *  Gets the shader input identifier.
     *
     *  @return The shader input identifier.
     *
     */

    U32 getID();

    /**
     *
     *  Gets the shader input identifier.
     *
     *  @return The shader input identifier.
     *
     */

    ShaderInputID getShaderInputID();

    /**
     *
     *  Gets the storage entry for the input.
     *
     *  @return The storage entry for the input.
     *
     */

    U32 getEntry();

    /**
     *
     *  Gets the storage unit for the input.
     *
     *  @return The storage unit for the input.
     *
     */

    U32 getUnit();

    /**
     *
     *  Gets the shader input attributes.
     *
     *  @return A pointer to the shader input attribute array.
     *
     */

    Vec4FP32 *getAttributes();

    /**
     *
     *  Marks the shader input as execution aborted/killed.
     *
     */

    void setKilled();

    /**
     *
     *  Gets the shader input kill flag.
     *
     *  @return Returns if the shader input execution was aborted (killed).
     *
     */

    bool getKill();

    /**
     *
     *  Sets shader input as a being fragment.
     *
     */

    void setAsFragment();

    /**
     *
     *  Sets shader input as a being a vertex.
     *
     */

    void setAsVertex();

    /**
     *
     *  Gets if the shader input/output is for a fragment.
     *
     *  @return If the shader input/output is for a fragment.
     *
     */
    bool isFragment();

    /**
     *
     *  Gets if the shader input/output is for a vertex.
     *
     *  @return If the shader input/output is for a vertex.
     *
     */
    bool isVertex();

    /**
     *
     *  Sets the shader (vertex) input as the last one in the batch.
     *
     *  @param last Boolean storing if the shader input is the last in the batch.
     *
     */

    void setLast(bool last);

    /**
     *
     *  Gets the last in batch mark for the shader (vertex) input.
     *
     *  @return A boolean value representing if the shader input as the last in the batch.
     *
     */

    bool isLast();

    /**
     *
     *  Get the shader input mode.
     *
     *  @return The shader input mode (fragment, vertex, ...).
     *
     */

    ShaderInputMode getInputMode();

    /**
     *
     *  Sets the start cycle in the shader.
     *
     *  @param cycle The initial cycle in the shader.
     *
     */

    void setStartCycleShader(U64 cycle);

    /**
     *
     *  Gets the start cycle in the shader.
     *
     *  @return The start cycle in the shader.
     *
     */

    U64 getStartCycleShader() const;

    /**
     *
     *  Sets the shader latency (cycles spent in the shader).
     *
     *  @param cycle Number of cycles the shader input spent in the shader.
     *
     */

    void setShaderLatency(U32 cycle);

    /**
     *
     *  Gets the shader latency (cycles spent in the shader).
     *
     *  @return The number of cycles that the shader input spent inside the shader.
     *
     */

    U32 getShaderLatency() const;
};

} // namespace cg1gpu

#endif
