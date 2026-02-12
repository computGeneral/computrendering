/**************************************************************************
 *
 */

#include "cmMemoryControllerCommand.h"

using namespace arch;

MemoryControllerCommand::MemoryControllerCommand(MCCommand command, GPURegister reg, 
                                                 GPURegData data) :
   command(command), reg(reg), data(data)
{}

   MemoryControllerCommand::MemoryControllerCommand(MCCommand cmd) :command(cmd)
{}


MemoryControllerCommand* MemoryControllerCommand::createRegWrite(
                                                  GPURegister reg, GPURegData data)
{
    return new MemoryControllerCommand(MCCOM_REG_WRITE, reg, data);
}

MemoryControllerCommand* MemoryControllerCommand::createLoadMemory()
{
    return new MemoryControllerCommand(MCCOM_LOAD_MEMORY);
}

MemoryControllerCommand* MemoryControllerCommand::createSaveMemory()
{
    return new MemoryControllerCommand(MCCOM_SAVE_MEMORY);
}


MCCommand MemoryControllerCommand::getCommand() const
{
    return command;
}


GPURegister MemoryControllerCommand::getRegister() const
{
    return reg;
}

GPURegData MemoryControllerCommand::getRegisterData() const
{
    return data;
}
