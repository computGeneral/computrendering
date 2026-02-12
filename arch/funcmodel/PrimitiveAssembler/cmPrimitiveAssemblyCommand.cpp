/**************************************************************************
 *
 * Primitive Assembly Command class implementation file.
 *
 */

/**
 *
 *  @file cmPrimitiveAssemblyCommand.h
 *
 *  This file implements the Primitive Assembly Command class.
 *
 *  This class carries commands from the Command Processor to
 *  Primitive Assembly.
 *
 */

#include "cmPrimitiveAssemblyCommand.h"

using namespace arch;

/*  Primitive Assembly Command constructor.  For PACOM_RESET, PACOM_DRAW,
    and PACOM_END.  */
PrimitiveAssemblyCommand::PrimitiveAssemblyCommand(AssemblyComm comm)
{
    //  Check the type rasterizer command.  
    GPU_ASSERT(
        if ((comm != PACOM_RESET) && (comm != PACOM_DRAW) &&
            (comm != PACOM_END))
            CG_ASSERT("Illegal primitive assembly command for this constructor.");
    )

    //  Set the command.  
    command = comm;

    //  Set color for tracing.  
    setColor(command);

    setTag("PAsCom");
}


//  Primitive Assembly Command constructor.  For PACOM_REG_WRITE.  
PrimitiveAssemblyCommand::PrimitiveAssemblyCommand(GPURegister rReg, U32 rSubReg, GPURegData rData)
{
    //  Set the command.  
    command = PACOM_REG_WRITE;

    //  Set the command parameters.  
    reg = rReg;
    subReg = rSubReg;
    data = rData;

    //  Set color for tracing.  
    setColor(command);

    setTag("PAsCom");
}

//  Returns the primitive assembly command type.  
AssemblyComm PrimitiveAssemblyCommand::getCommand()
{
    return command;
}


//  Returns the register identifier to write/read.  
GPURegister PrimitiveAssemblyCommand::getRegister()
{
    return reg;
}

//  Returns the register subregister number to write/read.  
U32 PrimitiveAssemblyCommand::getSubRegister()
{
    return subReg;
}

//  Returns the register data to write.  
GPURegData PrimitiveAssemblyCommand::getRegisterData()
{
    return data;
}
