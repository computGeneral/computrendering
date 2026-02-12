/**************************************************************************
 *
 * StreamerCommand class implementation file.
 *
 */

#include "cmStreamerCommand.h"

using namespace arch;

/*  cmoStreamController Command Constructor for reset, start and end streaming
    commands.  */
StreamerCommand::StreamerCommand(StreamComm comm)
{
    //  Check the type of cmoStreamController Command to create.  
    GPU_ASSERT(
        if ((comm != STCOM_RESET) && (comm != STCOM_START) &&
            (comm != STCOM_END))
            CG_ASSERT("Incorrect cmoStreamController command.");
    )

    command = comm;

    //  Set object color for tracing.  
    setColor(command);

    setTag("stCom");
}


//  cmoStreamController Command constructor for register writes.  
StreamerCommand::StreamerCommand(GPURegister reg, U32 subReg, GPURegData regData)
{
    //  Create a STCOM_REG_WRITE command.  
    command = STCOM_REG_WRITE;

    //  Set command parameters.  
    streamReg = reg;
    streamSubReg = subReg;
    data = regData;

    //  Set object color for tracing.  
    setColor(command);

    setTag("stCom");
}

//  Gets the cmoStreamController command type.  
StreamComm StreamerCommand::getCommand()
{
    return command;
}


//  Gets the cmoStreamController destination register of the command.  
GPURegister StreamerCommand::getStreamerRegister()
{
    return streamReg;
}

//  Gets the cmoStreamController destination register subregister of the command.  
U32 StreamerCommand::getStreamerSubRegister()
{
    return streamSubReg;
}

//  Gets the data to write to the cmoStreamController register.  
GPURegData StreamerCommand::getRegisterData()
{
    return data;
}

