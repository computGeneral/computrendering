/**************************************************************************
 *
 * Shader Decode Command.
 *
 */

/**
 *
 *  @file cmShaderDecodeCommand.h
 *
 *  Defines ShaderDecodeCommand class.   This class stores commands
 *  from the decode/execute mdu to the Fetch mdu.
 *
 */

#include "GPUType.h"
#include "DynamicObject.h"

#ifndef _SHADERDECODECOMMAND_

#define _SHADERDECODECOMMAND_

namespace cg1gpu
{

//*  Commands that can be received from Shader.  
enum DecodeCommand
{
    UNBLOCK_THREAD,     //  Unblock the thread because the blocking instruction has finished.  
    BLOCK_THREAD,       //  Block the thread because of the execution of long latency instruction.  
    END_THREAD,         //  The thread has executed the last instruction.  
    REPEAT_LAST,        //  Ask fetch to resend the last instruction of the thread.  
    NEW_PC,             //  Send a new PC for the thread (branch, call, ret).  
    ZEXPORT_THREAD      //  The thread has executed a z export instruction.  
};

/**
 *
 *  Defines a Shader Decode Command from the Decode/Execute mdu to the Fetch mdu.
 *
 *  This class stores information about commands sent to the Fetch mdu
 *  from the decode/execute mdu.
 *  This class inherits from the DynamicObject class which offers dynamic
 *  memory management and tracing support.
 *
 */

class ShaderDecodeCommand : public DynamicObject
{

private:

    DecodeCommand command;      //  Type of the decode command.  
    U32 numThread;           //  Thread Number for the command.  
    U32 PC;                  //  New PC for the thread.  

public:

    /**
     *
     *  ShaderDecodeCommand constructor.
     *
     *  Creates a ShaderDecodeCommand without any parameters.
     *
     *  @param comm  Command type for the ShaderDecodeCommand.
     *  @param threadID  Thread identifier for which the command is issued.
     *  @param PC  New PC for thread branches, calls, rets.
     *  @return A ShaderDecodeCommand object.
     *
     */

    ShaderDecodeCommand(DecodeCommand comm, U32 threadID, U32 PC);

    /**
     *
     *  Returns the type of the command.
     *
     *  @return  Type of the command.
     *
     */

    DecodeCommand getCommandType();

    /**
     *
     *  Returns the new PC for the thread.
     *
     *  @return New PC for the thread.
     *
     */

    U32 getNewPC();

    /**
     *
     *  Returns the thread identifier.
     *
     *  @return The thread identifier for which the command is issued.
     *
     */

    U32 getNumThread();

};

} // namespace cg1gpu

#endif
