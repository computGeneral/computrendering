/**************************************************************************
 *
 * Clipper Command class definition file. 
 *
 */

#ifndef _CLIPPERCOMMAND_

#define _CLIPPERCOMMAND_

#include "DynamicObject.h"
#include "support.h"
#include "GPUType.h"
#include "GPUReg.h"

namespace arch
{

//**  Clipper Command types.  
enum ClipCommand
{
    CLPCOM_RESET,           //  Reset the Rasterizer.  
    CLPCOM_REG_WRITE,       //  Write a Rasterizer register.  
    CLPCOM_REG_READ,        //  Read a Rasterizer register.  
    CLPCOM_START,           //  Start drawing a batch of transformed triangles.  
    CLPCOM_END              //  End drawing of a vertex batch.  
};

/**
 *
 *  This class defines Clipper Command objects.
 *
 *  Clipper Commad objects are created in the Command Processor and
 *  sent to the Clipper unit to change the Clipper state and issue
 *  commands to the Clipper.
 *
 *  Inherits from DynamicObject class that implements dynamic
 *  memory management and tracing.
 *
 */
 
class ClipperCommand : public DynamicObject
{

private:

    ClipCommand command;    //  The clipper command issued.  
    GPURegister reg;        //  The clipper register to write or read.  
    U32 subReg;          //  Clipper register subregister to write or read.  
    GPURegData data;        //  Data to write or read from the clipper register.  


public:

    /**
     *
     *  Clipper Command constructor.
     *
     *  This function creates and initializes a new Clipper Command
     *  object.
     *  This function can be only used to create CLPCOM_RESET, CLPCOM_START
     *  and CLPCOM_END commands.
     *
     *  @param comm The type of Clipper Command to create.
     *  @return An initialized Clipper Command object of the
     *  requested type.
     *
     */
     
    ClipperCommand(ClipCommand comm);
    
    /**
     *
     *  Clipper Command constructor.
     *
     *  This function creates and initializes a new Clipper
     *  Command object of type CLPCOM_REG_WRITE.
     *
     *  @param reg Clipper register to write to.
     *  @param subReg Clipper register subregister to write to.
     *  @param data Data to write to the Clipper register.
     *  @return An initialized Clipper Command object for
     *  a write register request. 
     *
     */
     
    ClipperCommand(GPURegister reg, U32 subReg, GPURegData data);
    
    /**
     *
     *  Gets the type of this Clipper Command.
     *
     *  @return The type of command for this Clipper Command
     *  object.
     *
     */
     
    ClipCommand getCommand();
    
    
    /**
     *
     *  Gets the Clipper register identifier to write to
     *  or read from for this Clipper Command object.
     *
     *  @return The destination/source Clipper register
     *  identifier.
     *
     */
     
    GPURegister getRegister();

    /** 
     *
     *  Gets the Clipper register subregister to write or to read
     *  from for this Clipper Command object.
     *
     *  @return The destination/source Clipper register subregister
     *  number.
     *
     */
    
    U32 getSubRegister();

    /**
     *
     *  Gets the data to write to the Clipper register. 
     *
     *  @return Data to write to the Clipper register.
     *
     */   
    
    GPURegData getRegisterData();
   
};

} // namespace arch

#endif
