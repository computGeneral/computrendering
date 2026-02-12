
#ifndef _cgoMetaStream_
#define _cgoMetaStream_

#include "GPUType.h"
#include "DynamicObject.h"
#include "GPUReg.h"
#include <string>
#include <ostream>
#include <iostream>
#include "zfstream.h"

namespace cg1gpu
{

//  Number of bytes per MetaStream Packet.  
static const U32 META_STREAM_PACKET_SIZE = 32;

//  Number of bits for a MetaStream packet 
static const U32 META_STREAM_PACKET_SHIFT = 5;

/**  Defines the different kind of transactions that go through the metaStream port.  */
enum MetaStreamType
{
    META_STREAM_WRITE,      //  MetaStream write from system memory (system to local).  
    META_STREAM_READ,       //  MetaStream read to system memory (local to system).  
    META_STREAM_PRELOAD,    //  Preload data into system or gpu memory.  
    META_STREAM_REG_WRITE,  //  MetaStream write access to GPU registers.  
    META_STREAM_REG_READ,   //  MetaStream read access to GPU registers.  
    META_STREAM_COMMAND,    //  MetaStream control command to the GPU Command Processor.  
    META_STREAM_INIT_END,   //  Marks the end of the initialization phase.  
    META_STREAM_EVENT       //  Signal an event to the GPU Command processor.  
};


/**
 *
 *  This classes defines a transaction through the MetaStream port between
 *  the main system (CPU, main memory) and the 3D hardware (GPU, local
 *  memory).
 *
 *  Inherits from the Dynamic Object class that supports fast dynamic
 *  creation and destruction of comunication objects and tracing
 *  capabilities.
 *
 */

class cgoMetaStream : public DynamicObject
{

private:

    MetaStreamType     metaStreamType;          //  Type of this MetaStream.  
    U32         address;         //  MetaStream destination address.  
    U32         size;            //  Size in bytes of the transmited data.  
    U08        *data;            //  Pointer to the transmited data.  
    GPURegister gpuReg;          //  GPU register from which read or write.  
    GPURegData  regData;         //  Data to read/write from the GPU register.  
    U32         subReg;          //  GPU register subregister address.  
    GPUCommand  gpuCommand;      //  GPU control command.  
    U32         numPackets;      //  Number of MetaStream packets for this transaction.  
    bool        locked;          //  Flag marking the transaction must wait until the next batch has finished.  
    U32         md;              //  Memory descriptor associated with the register write (Used to help parsing MetaStream traces).  

    GPUEvent    gpuEvent;        //  GPU event.  
    std::string eventMsg;        //  Event message.  
    std::string debugInfo;

public:

    /**
     *
     *  MetaStream constructor function.
     *  MetaStream Read/Write data from/to GPU local memory.
     *
     *  @param address GPU local memory address from/to which to read/write the data.
     *  @param size Number of bytes to read from the GPU local memory.
     *  @param data A pointer to the buffer where to write/read the read/written data from/to the GPU local memory.
     *  @param md Identifier of the memory descriptor (GPU driver) associated with this MetaStream.
     *  @param isWrite TRUE if it is a write operation (system to local).
     *  @param isLocked TRUE if the write operation must wait until the end of the batch.
     *
     *  @return  An initialized MetaStream object.
     */
    cgoMetaStream(U32 address, U32 size, U08 *data, U32 md, bool isWrite, bool isLocked = true);

    /**
     *  MetaStream constructor function.
     *  MetaStream Preload data into GPU local memory or system memory.
     *
     *  @param address GPU or system memory address into which to preload the data.
     *  @param size Number of bytes to preload.
     *  @param data A pointer to the buffer from where to preload the data.
     *  @param md Identifier of the memory descriptor (GPU driver) associated with this MetaStream.
     *
     *  @return  An initialized MetaStream object.
     */
    cgoMetaStream(U32 address, U32 size, U08 *data, U32 md);

    /**
     *
     *  MetaStream constructor function.
     *  MetaStream Write to a GPU register.
     *
     *  @param gpuReg  The register identifier where to write.
     *  @param subReg The subregister number where to write.
     *  @param regData The data to write in the register.
     *  @param md Identifier of the memory descriptor (GPU driver) associated with this MetaStream.
     *
     *  @return An initialized MetaStream object.
     *
     */
    cgoMetaStream(GPURegister gpuReg, U32 subReg, GPURegData regData, U32 md);

    /**
     *
     *  MetaStream constructor function.
     *  MetaStream Read from a GPU register.
     *
     *  @param gpuReg GPU register identifier from which to read.
     *  @param subReg GPU register subregister number from which to read.
     *
     *  @return An initialized MetaStream object.
     *
     */
    cgoMetaStream(GPURegister gpuReg, U32 subReg);

    /**
     *
     *  MetaStream constructor function.
     *
     *  MetaStream GPU command.
     *  @param gpuCommand Control command issued to the GPU Command processor.
     *
     *  @return An initialized MetaStream object.
     */
    cgoMetaStream(GPUCommand gpuCommand);

    /**
     *
     *  MetaStream construction function.
     *
     *  MetaStream Event to be signaled to the Command Processor.
     *
     *  @param event The event to be signaled to the Command Processor.
     *  @param string A string that will be printed with the event.
     *
     */
     
    cgoMetaStream(GPUEvent event, string msg);
     
    /**
     *  MetaStream constructor function.
     *  MetaStream Initialization End.
     *
     *  @return An initialized MetaStream object.
     */
    cgoMetaStream();   
     
    /**
     *
     *  MetaStream constructor.
     *  Load MetaStream from a MetaStream trace file.
     *
     *  @param ProfilingFile Reference to a file stream from where to load the MetaStream.
     *
     *  @return An initialized MetaStream object.
     *
     */
    cgoMetaStream(gzifstream *ProfilingFile);
    
    /**
     *
     *  MetaStream constructor.
     *  Clones the MetaStream passed as a parameter.
     *
     *  @param sourceMetaStreamTrans Pointer to the cgoMetaStream to clone.
     *
     *  @return An initialized MetaStream object which is a clone of the passed cgoMetaStream.
     *
     */
    cgoMetaStream(cgoMetaStream *sourceMetaStreamTrans);
    
    /**
     *  Sets the GPU Register Data attribute of the transaction.
     *
     *  @param gpuData The GPU Register Data to write in the transaction.
     *
     */
    void setGPURegData(GPURegData gpuData);

    /**
     *  Returns the GPU Register Data attribute from the transaction.
     *
     *  @return The GPU Register Data from the transaction.
     *
     */
    GPURegData getGPURegData();

    /**
     *  Returns the MetaStream type.
     *  @return The MetaStream type.
     *
     */

    MetaStreamType GetMetaStreamType();

    /**
     *
     *  Returns the MetaStream source/destination address.
     *
     *  @return The MetaStream source/destination address.
     *
     */

    U32 getAddress();

    /**
     *
     *  Returns the pointer to the buffer for the data to read/write
     *  in the MetaStream.
     *
     *  @return MetaStream data buffer pointer.
     *
     */

    U08 *getData();

    /**
     *
     *  Returns the amount of bytes that are being transmited
     *  with the MetaStream.
     *
     *  @return Size in bytes of the MetaStream data.
     *
     */

    U32 getSize();

    /**
     *
     *  Returns the GPU register identifier destination of the
     *  MetaStream.
     *
     *  @return The GPU register identifer.
     *
     */

    GPURegister getGPURegister();

    /**
     *
     *  Returns the GPU register subregister number that is the
     *  destination of the MetaStream.
     *
     *  @return The GPU register subregister number.
     *
     */

     U32 getGPUSubRegister();

    /**
     *
     *  Returns the GPU control command issued with the MetaStream.
     *
     *  @return The GPU command issued in the MetaStream.
     *
     */

    GPUCommand getGPUCommand();

    /**
     *
     *  Returns the GPU event that was signaled in the MetaStream.
     *
     *  @return The GPU event signaled in the MetaStream.
     *
     */
     
    GPUEvent getGPUEvent();
    
    /**
     *
     *  Returns the message string for the event signaled in the MetaStream.
     *
     *  @return The message string for the event signaled to the GPU.
     *
     */     
     
    std::string getGPUEventMsg();
        
    /**
     *
     *  Returns the number of MetaStream packets for this MetaStream.
     *
     *  @return The number of MetaStream packets for this MetaStream.
     *
     */

    U32 getNumPackets();

    /**
     *
     *  Returns if the MetaStream is locked and must wait until the
     *  end of the current batch.
     *
     */

    bool getLocked();

    /**
     *
     *  Returns the memory descriptor associated with the MetaStream.
     *
     *  @return The memory descriptor identifier associated with the MetaStream.
     *
     */
     
    U32 getMD();
     
    /**
     *
     *  Saves the MetaStream object into a file.
     *
     *  @param outFile Pointer to a stream file where the MetaStream is going to be saved.
     *
     */
    
    void save(gzofstream *outFile);

    /**
     *
     *  Changes an MetaStream from META_STREAM_WRITE to META_STREAM_PRELOAD.
     *
     */    
     
    void forcePreload();
     
    /**
     * Dumps cgoMetaStream info
     */
    void dump(std::ostream& os = std::cout) const;

    void setDebugInfo(const std::string& debugInfo) { this->debugInfo = debugInfo; }
    std::string getDebugInfo() const { return debugInfo; }


    ~cgoMetaStream();
};

} // namespace cg1gpu

#endif
