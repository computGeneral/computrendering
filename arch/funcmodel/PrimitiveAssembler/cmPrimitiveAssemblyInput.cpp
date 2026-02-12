/**************************************************************************
 *
 * Primitive Assembly Input definition file.
 *
 */

#include "cmPrimitiveAssemblyInput.h"

using namespace arch;

//  Creates a new PrimitiveAssemblyInput.  
PrimitiveAssemblyInput::PrimitiveAssemblyInput(U32 ID, Vec4FP32 *attrib)
{
    //  Set vertex parameters.  
    id = ID;
    attributes = attrib;

    setTag("PAsIn");
}

//  Gets the Primitive Assembly input identifier (vertex index).  
U32 PrimitiveAssemblyInput::getID()
{
    return id;
}

//  Gets the Primitive Assembly input attributes.  
Vec4FP32 *PrimitiveAssemblyInput::getAttributes()
{
    return attributes;
}
