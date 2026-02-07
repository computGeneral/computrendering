/**************************************************************************
 *
 * Shader Decode Command.
 *
 */


/**
 *
 *  @file ShaderDecodeCommand.cpp
 *
 *  This file store the implementation for the ShaderDecodeCommand class.
 *  The ShaderDecodeCommand class stores information about commands sent
 *  to the Fetch mdu from the Decode/Execute mdu.
 *
 */

#include "cmShaderDecodeCommand.h"
#include <stdio.h>

using namespace cg1gpu;

//  Creates the DecodeCommand initialized.
ShaderDecodeCommand::ShaderDecodeCommand(DecodeCommand comm, U32 threadID, U32 newPC):
command(comm), numThread(threadID), PC(newPC)
{
    //  Set color for tracing.
    setColor(command);

    //  Set information string.
    switch(command)
    {
        case UNBLOCK_THREAD:
            sprintf((char *) getInfo(), "UNBLOCK_THREAD ThID %03d PC %04x ", numThread, PC);
            break;
        case BLOCK_THREAD:
            sprintf((char *) getInfo(), "BLOCK_THREAD ThID %03d PC %04x ", numThread, PC);
            break;
        case END_THREAD:
            sprintf((char *) getInfo(), "END_THREAD ThID %03d PC %04x ", numThread, PC);
            break;
        case REPEAT_LAST:
            sprintf((char *) getInfo(), "REPEAT_LAST ThID %03d PC %04x ", numThread, PC);
            break;
        case NEW_PC:
            sprintf((char *) getInfo(), "NEW_PC ThID %03d PC %04x ", numThread, PC);
            break;
        default:
            break;
    }
    
    setTag("shDCom");
}

//  Returns the type of the command.
DecodeCommand ShaderDecodeCommand::getCommandType()
{
    return command;
}

//  Returns the size of the shader program code in bytes.
U32 ShaderDecodeCommand::getNewPC()
{
    return PC;
}

//  Returns the thread number for which this command is send.
U32 ShaderDecodeCommand::getNumThread()
{
    return numThread;
}
