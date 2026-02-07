/**************************************************************************
 *
 * Primitive Assembly Command class definition file. 
 *
 */

/**
 *
 *  @file cmPrimitiveAssemblyCommand.h
 *
 *  This file defines the Primitive Assembly Command class.
 *
 *  This class carries commands from the Command Processor
 *  to Primitive Assembly.
 *
 */
 
#ifndef _PRIMITIVEASSEMBLYCOMMAND_

#define _PRIMITIVEASSEMBLYCOMMAND_

#include "DynamicObject.h"
#include "support.h"
#include "GPUType.h"
#include "GPUReg.h"

namespace cg1gpu
{

//**  Primitive Assembly Command types.  
enum AssemblyComm
{
    PACOM_RESET,            //  Reset the Rasterizer.  
    PACOM_REG_WRITE,        //  Write a Rasterizer register.  
    PACOM_REG_READ,         //  Read a Rasterizer register.  
    PACOM_DRAW,             //  Start drawing a batch of transformed triangles.  
    PACOM_END,              //  End drawing of a vertex batch.  
};

/**
 *
 *  This class defines Primitive Assembly Command objects.
 *
 *  Primitive Assembly commad objects are created in the Command 
 *  Processor and sent to the Primitive Assembly unit to change
 *  the Primitive Assemby state and issue commands to the Primitive
 *  Assembly.
 *
 *  Inherits from DynamicObject class that implements dynamic
 *  memory management and tracing.
 *
 */
 
class PrimitiveAssemblyCommand : public DynamicObject
{

private:

    AssemblyComm command;   //  The primitive assembly command issued.  
    GPURegister reg;        //  The primitive assembly register to write or read.  
    U32 subReg;          //  Primitive assembly register subregister to write or read.  
    GPURegData data;        //  Data to write or read from the primitive assembly register.  


public:

    /**
     *
     *  Primitive Assembly Command constructor.
     *
     *  This function creates and initializes a new Primitive Assembly Command
     *  object.
     *  This function can be only used to create PACOM_RESET, PACOM_DRAW
     *  and PA_END commands.
     *
     *  @param comm The type of Primitive Assembly Command to create.
     *  @return An initialized Primitive Assembly Command object of the
     *  requested type.
     *
     */
     
    PrimitiveAssemblyCommand(AssemblyComm comm);
    
    /**
     *
     *  Primitive Assembly Command constructor.
     *
     *  This function creates and initializes a new Primitive Assembly
     *  Command object of type PACOM_REG_WRITE.
     *
     *  @param reg Primitive Assembly register to write to.
     *  @param subReg Primitive Assembly register subregister to write to.
     *  @param data Data to write to the Primitive Assembly register.
     *  @return An initialized Primitive Assembly Command object for
     *  a write register request. 
     *
     */
     
    PrimitiveAssemblyCommand(GPURegister reg, U32 subReg, GPURegData data);
    
    /**
     *
     *  Gets the type of this Primitive Assembly Command.
     *
     *  @return The type of command for this Primitive Assembly Command
     *  object.
     *
     */
     
    AssemblyComm getCommand();
    
    
    /**
     *
     *  Gets the Primitive Assembly register identifier to write to
     *  or read from for this Primitive Assembly Command object.
     *
     *  @return The destination/source Primitive Assembly register
     *  identifier.
     *
     */
     
    GPURegister getRegister();

    /** 
     *
     *  Gets the Primitive Assembly register subregister to write or to read
     *  from for this Primitive Assembly Command object.
     *
     *  @return The destination/source Primitive Assembly register subregister
     *  number.
     *
     */
    
    U32 getSubRegister();

    /**
     *
     *  Gets the data to write to the Primitive Assembly register. 
     *
     *  @return Data to write to the Primitive Assembly register.
     *
     */   
    
    GPURegData getRegisterData();
   
};


} // namespace cg1gpu

#endif
