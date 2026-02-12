/**************************************************************************
 *
 */

#ifndef MEMORYCONTROLLERDEFS_H
    #define MEMORYCONTROLLERDEFS_H

#include "GPUReg.h"

//#define USE_MEMORY_CONTROLLER_V2

namespace arch {


//*  Defines the different states for a memory request entry.  
enum MemReqState
{
    MRS_READY,      //  The memory request can be processed.  
    MRS_WAITING,    //  The memory request is waiting for the memory module response.  
    MRS_MEMORY,     //  The memory request is going to GPU memory.  
    MRS_TRANSMITING //  The memory request is being transmited to a GPU unit.  
};

//**  This defines the GPU unit source of the memory transaction request.  
enum GPUUnit
{
    COMMANDPROCESSOR = 0,   //  Command Processor unit.  
    STREAMERFETCH,          //  cmoStreamController Fetch unit.  
    STREAMERLOADER,         //  cmoStreamController Loader unit.  
    ZSTENCILTEST,           //  Z Stencil Test unit.  
    COLORWRITE,             //  Color Write unit.  
    DISPLAYCONTROLLER,                   //  DisplayController unit.  
    TEXTUREUNIT,            //  Texture Unit.  
    MEMORYMODULE,           //  Memory modules buses (channels), not used in Version 2 
    SYSTEM,                 //  System memory bus.  
    LASTGPUBUS              //  Marks the last GPU bus entry.  
};

/**
 * Defines the memory controller state.
 */
enum MemState
{
    MS_NONE         = 0x00, //  Does not accept any request.  
    MS_READ_ACCEPT  = 0x01, //  Read requests can be issued.  
    MS_WRITE_ACCEPT = 0x02, //  Write data can be sent to the Memory Controller.  
    MS_BOTH         = 0x03  //  Accepts read requests and write data transactions.  
};

/*
 * This defines the different types of memory transactions.
 */
enum MemTransCom
{
    MT_READ_REQ,    //  Read request from GPU memory.  
    MT_READ_DATA,   //  Read data from GPU memory.  
    MT_WRITE_DATA,  //  Write data to GPU memory.  
    MT_PRELOAD_DATA,//  Preload data into GPU memory.  
    MT_STATE        //  Carries the current state of the memory controller.  
};



//**  Maximum size of a memory transaction.  
const U32 MAX_TRANSACTION_SIZE = 64;

/**
 *  Defines the mask to get the offset for a transaction
 *  offset.
 */
const U32 TRANSACTION_OFFSET_MASK = (MAX_TRANSACTION_SIZE - 1);

//*  Write mask size.  
const U32 WRITE_MASK_SIZE = (MAX_TRANSACTION_SIZE >> 2);

//**  Maximum number of memory transaction ids availables.  
static const U32 MAX_MEMORY_TICKETS = 256;


} // namespace arch

#endif // MEMORYCONTROLLERDEFS_H
