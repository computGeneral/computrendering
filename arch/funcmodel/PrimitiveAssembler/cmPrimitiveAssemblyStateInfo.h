/**************************************************************************
 *
 * Primitive Assembly State Info definition file.
 *
 */


#ifndef _PRIMITIVEASSEMBLYSTATEINFO_
#define _PRIMITIVEASSEMBLYSTATEINFO_

#include "DynamicObject.h"
#include "cmPrimitiveAssembly.h"

namespace arch
{
/**
 *
 *  @file cmPrimitiveAssemblyStateInfo.h
 *
 *  This file defines the Primitive Assembly State Info class.
 *
 */
 
/**
 *
 *  This class defines a container for the state signals
 *  that the Primitive Assembly sends to the Command Processor.
 *
 *  Inherits from Dynamic Object that offers dynamic memory
 *  support and tracing capabilities.
 *
 */


class PrimitiveAssemblyStateInfo : public DynamicObject
{
private:
    
    AssemblyState state;    //  The primitive assembly state.  

public:

    /**
     *
     *  Creates a new PrimitiveAssemblyStateInfo object.
     *
     *  @param state The primitive assembly state carried by this
     *  primitive assembly state info object.
     *
     *  @return A new initialized PrimitiveAssemblyStateInfo object.
     *
     */
     
    PrimitiveAssemblyStateInfo(AssemblyState state);
    
    /**
     *
     *  Returns the primitive assembly state carried by the primitive
     *  assembly state info object.
     *
     *  @return The primitive assembly state carried in the object.
     *
     */
     
    AssemblyState getState();
};

} // namespace arch

#endif
