/**************************************************************************
 *
 * Clipper Command class implementation file.
 *
 */

/**
 *
 *  @file ClipperCommand.cpp
 *
 *  This file implements the Clipper Command class.
 *
 *  This class objects are used to carry command from the
 *  Command Processor to the Clipper unit.
 *
 */


#include "cmClipperCommand.h"

using namespace arch;

/*  Clipper Command constructor.  For CLPCOM_RESET, CLPCOM_START,
    and CLPCOM_END.  */
ClipperCommand::ClipperCommand(ClipCommand comm)
{
    //  Check the type rasterizer command.  
    GPU_ASSERT(
        if ((comm != CLPCOM_RESET) && (comm != CLPCOM_START) &&
            (comm != CLPCOM_END))
            CG_ASSERT("Illegal primitive assembly command for this constructor.");
    )

    //  Set the command.  
    command = comm;

    //  Set color for tracing.  
    setColor(command);

    setTag("ClpCom");
}


//  Clipper Command constructor.  For CLPCOM_REG_WRITE.  
ClipperCommand::ClipperCommand(GPURegister rReg, U32 rSubReg, GPURegData rData)
{
    //  Set the command.  
    command = CLPCOM_REG_WRITE;

    //  Set the command parameters.  
    reg = rReg;
    subReg = rSubReg;
    data = rData;

    //  Set color for tracing.  
    setColor(command);

    setTag("ClpCom");
}

//  Returns the clipper command type.  
ClipCommand ClipperCommand::getCommand()
{
    return command;
}


//  Returns the register identifier to write/read.  
GPURegister ClipperCommand::getRegister()
{
    return reg;
}

//  Returns the register subregister number to write/read.  
U32 ClipperCommand::getSubRegister()
{
    return subReg;
}

//  Returns the register data to write.  
GPURegData ClipperCommand::getRegisterData()
{
    return data;
}
