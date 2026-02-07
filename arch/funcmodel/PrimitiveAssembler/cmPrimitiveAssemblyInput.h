/**************************************************************************
 *
 * Primitive Assembly Input implementation file. 
 *
 */

#ifndef _PRIMITIVEASSEMBLYRINPUT_

#define _PRIMITIVEASSEMBLYINPUT_

#include "support.h"
#include "GPUType.h"
#include "DynamicObject.h"

namespace cg1gpu
{

/**
 *
 *  This class defines objects that carry transformed vertex
 *  information from the cmoStreamController Commit (output cache) to
 *  the Primitive Assembly unit.
 *
 *  This class inherits from DynamicObject class that provides
 *  basic dynamic memory support and signal tracing capabilities.
 *
 */
 
class PrimitiveAssemblyInput : public DynamicObject
{

private:

    U32 id;              //  Primitive Assembly Input identifier (it uses to be its index).  
    Vec4FP32 *attributes;  //  Primitive Assembly Input attributes.  

public:

    /**
     *
     *  Primitive Assembly Input constructor.
     *
     *  Creates and initializes a Primitive Assembly input.
     *
     *  @param id The Primitive Assembly input identifier (vertex index).
     *  @param attrib Primitive Assembly input attributes.
     *
     *  @return An initialized Primitive Assembly input.
     *
     */
     
    PrimitiveAssemblyInput(U32 id, Vec4FP32 *attrib);
    
    /**
     *
     *  Gets the Primitive Assembly input identifier (vertex index).
     *
     *  @return The Primitive Assembly input identifier.
     *
     */

    U32 getID();
    
    /**
     *
     *  Gets the Primitive Assembly input attributes.
     *
     *  @return A pointer to the Primitive Assembly input attribute array.
     *
     */

    Vec4FP32 *getAttributes();
    
};

} // namespace cg1gpu

#endif
