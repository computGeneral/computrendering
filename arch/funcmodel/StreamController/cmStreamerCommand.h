/**************************************************************************
 *
 * cmoStreamController Command class definition file. 
 *
 */

#ifndef _STREAMER_COMMAND_

#define _STREAMER_COMMAND_

#include "DynamicObject.h"
#include "support.h"
#include "GPUType.h"
#include "GPUReg.h"

namespace arch
{

//**  cmoStreamController Commands.  
enum StreamComm
{
    STCOM_RESET,            //  Reset cmoStreamController unit command.  
    STCOM_REG_WRITE,        //  cmoStreamController state register write command.  
    STCOM_REG_READ,         //  cmoStreamController state register read command.  
    STCOM_START,            //  cmoStreamController start streaming command.  
    STCOM_END               //  cmoStreamController end streaming command.  
};

/**
 *
 *  This class defines cmoStreamController Command objects.
 *
 *  A cmoStreamController Command carries dynamically created orders and
 *  state changes from the Command Processor to the cmoStreamController
 *  unit.
 *  This class inherits from the Dynamic Object class that offers
 *  dynamic memory management and trace support.
 *
 */
 
class StreamerCommand : public DynamicObject
{

private:

    StreamComm command;     //  The command issued by the cmoStreamController Commad.  
    GPURegister streamReg;  //  cmoStreamController state register to write/read.  
    U32 streamSubReg;    //  cmoStreamController state register subregister to write/read.  
    GPURegData data;        //  Data to write/read to the StreamController state register.  

public:

    /**
     *
     *  StreamerCommand constructor.
     *  
     *  Creates a new cmoStreamController Command object (for STCOM_RESET, STCOM_START and
     *  STCOM_END).
     *
     *  @param comm Type of cmoStreamController Command.
     *
     *  @return An initialized StreamerCommand object.
     *
     */
     
    StreamerCommand(StreamComm comm);

    /**
     *
     *  cmoStreamController Command constructor.
     *
     *  Creates a new cmoStreamController Command Object (for STCOM_REG_WRITE).
     *
     *  @param reg cmoStreamController register to write.
     *  @param subReg cmoStreamController register subregister to write.
     *  @param data Data to write to the cmoStreamController register.
     *
     *  @return An initialized register write cmoStreamController Command object.
     *
     */
     
    StreamerCommand(GPURegister reg, U32 subReg, GPURegData data);
    
    /**
     *
     *  Returns the cmoStreamController Command type.
     *
     *  @return The cmoStreamController Command type.
     *
     */
     
    StreamComm getCommand();
    
    /**
     *
     *  Returns the cmoStreamController register from where to read/write.
     *
     *  @return The cmoStreamController register destination of the cmoStreamController Command.
     *
     */
     
    GPURegister getStreamerRegister();
    
    /**
     *
     *  Returns the cmoStreamController register subregister from where to read/write.
     *
     *  @return The cmoStreamController register subregister destination of the 
     *  cmoStreamController Command.
     *
     */
     
    U32 getStreamerSubRegister();
    
    /**
     *
     *  Returns the data to write to a cmoStreamController register.
     * 
     *  @return Data to write to a cmoStreamController Register.
     *
     */
     
    GPURegData getRegisterData();
    
};

} // namespace arch

#endif
