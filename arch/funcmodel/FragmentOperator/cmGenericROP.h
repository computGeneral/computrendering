/**************************************************************************
 *
 * Generic ROP mdu definition file.
 *
 */

/**
 *
 *  @file cmGenericROP.h
 *
 *  This file defines the Generic ROP mdu.
 *
 *  This class implements a Generic ROP pipeline that will be used to implement
 *  the Z Stencil and Color Write stages of the GPU.
 *
 */


#ifndef _GENERICROP_

#define _GENERICROP_

#include "GPUType.h"
#include "support.h"
#include "cmMduBase.h"
#include "cmROPCache.h"
#include "GPUReg.h"
#include "cmRasterizerState.h"
#include "cmRasterizerCommand.h"
#include "cmMemoryControllerDefs.h"
#include "cmFragmentInput.h"
#include "bmFragmentOperator.h"
#include "bmRasterizer.h"
#include "cmtoolsQueue.h"
#include "PixelMapper.h"

namespace arch
{

/**
 *
 *  Defines a Generic ROP Queue entry.
 *
 *  A Generic ROP queue entry stores information related with a fragment stamp
 *  that must be processed.
 *
 *  The generic information includes the memory address from a buffer associated
 *  with the stamp, the a way, line and offset inside the ROP cache and the array
 *  storing the fragments in the stamp.
 *
 */

class ROPQueue
{

public:

    U32 address[MAX_RENDER_TARGETS]; //  Adddres in the buffer where to store the stamp fragments.  
    U32 way[MAX_RENDER_TARGETS];     //  Way of the ROP cache where to store the stamp.  
    U32 line[MAX_RENDER_TARGETS];    //  Line of the ROP cache where to store the stamp.  
    FragmentInput **stamp;              //  Pointer to the fragments in the stamp.  
    bool culled[STAMP_FRAGMENTS];       //  Array of cull flags for each fragment in the stamp.  
    bool lastStamp;                     //  Flag that stores if the current stamp is the last in a batch.  
    U08 *data;                        //  Buffer for storing data to be read or written for the stamp.  
    bool *mask;                         //  Stores the write mask for the stamp. 
    U32 nextSample;                  //  Stores an index to the next group of samples to process for MSAA.  
    U32 nextBuffer;                  //  Stores an index to the next buffer to process.  
};


/**
 *
 *  Defines the state reported by the ROP unit to the producer stage/unit in the GPU pipeline
 *  that is sending stamps to the ROP stage.
 *
 */

enum ROPState
{
    ROP_READY,      //  ROP can receive stamps.  
    ROP_BUSY        //  ROP can not receive stamps.  
};

/**
 *
 *  This class implements the simulation of the pipeline of a generic ROP stage.
 *  The Generic ROP class is to be used to derive the specific ROP stages
 *  Z Stencil Test and Color Write of the GPU pipeline.
 *
 *  This class inherits from the cmoMduBase class that offers basic simulation support.
 *
 */

class GenericROP : public cmoMduBase
{
private:

    //  Generic ROP rate and latency parameters.  
    U32 stampsCycle;         //  Number of stamps that can be received per cycle.  
    U32 ropRate;             //  Rate at which stamps are operated (minimum cycles between two stamps).  
    U32 ropLatency;          //  ROP operation latency in cycles.  

    //  Generic ROP queue parameters.  
    U32 inQueueSize;         //  Size of the input stamp queue.  
    U32 fetchQueueSize;      //  Size of the fetched stamp queue.  
    U32 readQueueSize;       //  Size of the read stamp queue.  
    U32 opQueueSize;         //  Size of the operation stamp queue.  
    U32 writeQueueSize;      //  Size of the written stamp queue.  

    //  Generic ROP stage name.
    char *ropName;              //  Name of the ROP stage.  
    char *ropShortName;         //  Short name/abbreviature for the specific ROP stage.  

    //  Generic ROP state.  
    MemState memoryState;       //  Current memory controller state.  
    bool receivedFragment;      //  If a fragment has been received in the current cycle.  

    //  Generic ROP counters.  
    U32 ropCycles;           //  Cycles until the next stamp can be operated.  
    U32 inputCycles;         //  Cycles until the next input can be received.  

    //  ROP queues.  
    tools::Queue<ROPQueue*> inQueue;    //  Input stamp queue.  Stores stamps received from a previous GPU stage.  
    tools::Queue<ROPQueue*> fetchQueue; //  Fetched stamp queue.  Stores stamps for which data has been fetched from memory.  
    tools::Queue<ROPQueue*> readQueue;  //  Read stamp queue.   Stores stamps for which data has been read from memory.  
    tools::Queue<ROPQueue*> opQueue;    //  Operation stamp queue.  Stores stamps that have finished the ROP operation.  
    tools::Queue<ROPQueue*> writeQueue; //  Write stamp queue.  Stores stamps for which data has been written into memory.  

    //  ROP CAM for RAW dependence detection.
    std::vector<ROPQueue*> rawCAM;      //  Vector that stores pointers to the stamps that are being processed after the read stage and before the write stage for RAW dependence detection .  
    U32 sizeCAM;                     //  Number of entries in the read non-written stamp CAM.  
    U32 stampsCAM;                   //  Number of stamps in the read non written stamp CAM.  
    U32 firstCAM;                    //  Pointer to the first entry in the read non written stamp CAM.  
    U32 freeCAM;                     //  Pointer to the next free entry in the read non-written stamp CAM.  

    //  Configurable buffers.  
    FragmentInput **stamp;      //  Stores last receveived stamp.  

    //  Statistics.  
    gpuStatistics::Statistic *inputs;       //  Input fragments.  
    gpuStatistics::Statistic *operated;     //  Operated fragments.  
    gpuStatistics::Statistic *outside;      //  Outside triangle/viewport fragments.  
    gpuStatistics::Statistic *culled;       //  Culled for non test related reasons.  
    gpuStatistics::Statistic *readTrans;    //  Read transactions to memory.  
    gpuStatistics::Statistic *writeTrans;   //  Write transactions to memory.  
    gpuStatistics::Statistic *readBytes;    //  Bytes read from memory.  
    gpuStatistics::Statistic *writeBytes;   //  Bytes written to memory.  
    gpuStatistics::Statistic *fetchOK;      //  Succesful fetch operations.  
    gpuStatistics::Statistic *fetchFail;    //  Failed fetch operations.  
    gpuStatistics::Statistic *allocateOK;   //  Succesful allocate operations.  
    gpuStatistics::Statistic *allocateFail; //  Failed allocate operations.  
    gpuStatistics::Statistic *readOK;       //  Sucessful read operations.  
    gpuStatistics::Statistic *readFail;     //  Failed read operations.  
    gpuStatistics::Statistic *writeOK;      //  Sucessful write operations.  
    gpuStatistics::Statistic *writeFail;    //  Failed write operations.  
    gpuStatistics::Statistic *rawDep;       //  Blocked read accesses because of read after write dependence between stamps.  
    gpuStatistics::Statistic *ropOpBusy;    //  Counts cycles in which the ROP Operator is busy and can not start processing a new stamp.  
    gpuStatistics::Statistic *consumerBusy; //  Counts cycles in which the consumer stage is busy and can not receive processeds tamps from the Generic ROP.  
    gpuStatistics::Statistic *inputEmpty;   //  Counts unused cycles when the input queue is empty.  
    gpuStatistics::Statistic *fetchEmpty;   //  Counts unused cycles when the fetch queue is empty.  
    gpuStatistics::Statistic *readEmpty;    //  Counts unused cycles when the read queue is empty.  
    gpuStatistics::Statistic *opEmpty;      //  Counts unused cycles when the operated queue is empty.  
    gpuStatistics::Statistic *writeEmpty;   //  Counts unused cycles when the written queue is empty.  
    gpuStatistics::Statistic *fetchFull;    //  Counts stall cycles when fetch stamp queue is full.  
    gpuStatistics::Statistic *readFull;     //  Counts stall cycles when read stamp queue is full.  
    gpuStatistics::Statistic *opFull;       //  Counts stall cycles when operation stamp queue is full.  
    gpuStatistics::Statistic *writeFull;    //  Counts stall cycles when written stamp queue is full.  

    //  Private functions.  

    /**
     *
     *  Processes a memory transaction.
     *
     *  @param cycle The current simulation cycle.
     *  @param memTrans The memory transaction to process
     *
     */

    void processMemoryTransaction(U64 cycle, MemoryTransaction *memTrans);


    /**
     *
     *  Receives stamps from a producer stage through the input fragment signal.
     *  Calls the processStamp function if a stamp is received.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void receiveStamps(U64 cycle);

    /**
     *
     *  Processes the stamp that has been just received.
     *  If the stamp can not be culled it's added to the input stamp queue.
     *
     */

    void processStamp();

    /**
     *
     *  Tries to fetch data for the stamp that is in the head of the input stamp queue.
     *  If succesful the stamp is moved to the fetch queue.
     *
     *  If the bypass flag is enabled the stamp is moved to the written queue queue.
     *
     *  If the read flag is disabled then it tries to allocate space in the write buffer
     *  for the stamp and if the operation is succesful the stamp is moved to the read
     *  stamp queue.
     *
     */

    void fetchStamp();

    /**
     *
     *  Tries to read data for the stamp that is in the head of the fetched stamp queue.
     *  If the operation is succesful the stamp is moved to read stamp queue.
     *
     */

    void readStamp();

    /**
     *
     *  Starts the ROP operation for the stamp in the head of the read stamp queue.
     *  The stamp is sent through the ROP operation latency signal and removed
     *  from the read stamp queue.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void startOperation(U64 cycle);

    /**
     *
     *  Ends the ROP operation for a stam.
     *  The stamp is received from the ROP operation latency signal and added to the operation
     *  stamp queue.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void endOperation(U64 cycle);

    /**
     *
     *  Tries to write data for an operated stamp that is in the head of the operation
     *  stamp queue.
     *  If the operation is succesful the stamp is moved to the written stamp queue.
     *
     */

    void writeStamp();

    /**
     *
     *  Terminate the processing the current written stamp and remove it from the pipeline.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void terminateStamp(U64 cycle);



protected:

    //  Generic ROP signals.  
    Signal *rastCommand;        //  Command signal from the Rasterizer main mdu.  
    Signal *rastState;          //  State signal to the Rasterizer main mdu.  
    Signal *inFragmentSignal;   //  Fragment input signal from GPU stage/unit producing fragments for the ROP stage.  
    Signal *ropStateSignal;     //  State signal to the GPU stage/unit producing fragments for the ROP stage.  
    Signal *outFragmentSignal;  //  Fragment output signal to the GPU stage/unit consuming fragments from the ROP stage.  
    Signal *consumerStateSignal;//  State signal from the GPU stage/unit consuming fragments from the ROP stage.  
    Signal *memRequest;         //  Memory request signal to the Memory Controller.  
    Signal *memData;            //  Memory data signal from the Memory Controller.  
    Signal *operationStart;     //  Start signal for the simulation of the ROP operation latency.  
    Signal *operationEnd;       //  End signal for the simulation of the ROP operation latency.  

    //  Generic ROP registers.  
    U32 hRes;                                //  Display horizontal resolution.  
    U32 vRes;                                //  Display vertical resolution.  
    S32 startX;                              //  Viewport initial x coordinate.  
    S32 startY;                              //  Viewport initial y coordinate.  
    U32 width;                               //  Viewport width.  
    U32 height;                              //  Viewport height.  
    U32 bufferAddress[MAX_RENDER_TARGETS];   //  Start address in memory of the current ROP buffer.  
    U32 stateBufferAddress;                  //  Start address in memory of the current ROP block state buffer.  
    bool multisampling;                         //  Flag that stores if MSAA is enabled.  
    U32 msaaSamples;                         //  Number of MSAA Z samples generated per fragment when MSAA is enabled.  
    U32 bytesPixel[MAX_RENDER_TARGETS];      //  Bytes per fragment/pixel.  
    bool compression;                           //  Flag to enable or disable buffer compression.  

    //  Generic ROP buffer parameters.  
    U32 overW;               //  Over scan tile width in scan tiles.  
    U32 overH;               //  Over scan tile height in scan tiles.  
    U32 scanW;               //  Scan tile width in generation tiles.  
    U32 scanH;               //  Scan tile height in generation tiles.  
    U32 genW;                //  Generation tile width in stamps.  
    U32 genH;                //  Generation tile height in stamps.  
    
    //  Reference to the ROP behaviorModel classes.  
    bmoFragmentOperator &frEmu;  //  Reference to the fragment operation behaviorModel object.  

    //  Pixel mapper.
    bool activeBuffer[MAX_RENDER_TARGETS];          // Defines if an output buffer is active.  
    U32 numActiveBuffers;                        // Number of output buffers active.  
    PixelMapper pixelMapper[MAX_RENDER_TARGETS];    // Maps pixels to memory addresses and processing units.  
    
    //  Generic ROP state.  
    RasterizerState state;                  //  Current mdu state.  
    ROPState consumerState;                 //  Current state of the consumer state that receives fragments processed by the ROP stage.  
    U32 currentTriangle;                 //  Identifier of the current triangle being processed (used to count triangles).  
    bool endFlush;                          //  Flag that signals the end of the ROP cache flush.  
    bool bypassROP[MAX_RENDER_TARGETS];     //  Bypass flag, set to true if the stamp must bypass the ROP stage without processing.  
    bool readDataROP[MAX_RENDER_TARGETS];   //  Read data flag, set to true if the stamp has to read data from the buffer before processing.  
    bool lastFragment;                      //  Last batch fragment flag.  

    //  Generic ROP Last Stamp container.  
    ROPQueue lastBatchStamp;    //  Stores the information about the last batch stamp.  

    //  Generic ROP counters.  
    U32 triangleCounter;     //  Number of processed triangles.  
    U32 fragmentCounter;     //  Number of fragments processed in the current batch.  
    U32 frameCounter;        //  Counts the number of rendered frames.  

    //  ROP queues.  
    tools::Queue<ROPQueue*> freeQueue;  //  Stores free ROPQueue entry objects.  

    //  ROP cache and associated constants.  
    ROPCache *ropCache;         //  Pointer to the ROP cache.  
    U32 stampMask;           //  Logical mask for the offset of a stamp inside a ROP cache line.  

    /**
     *
     *  Performs any remaining tasks after the stamp data has been written.
     *  The function should read, process and remove the stamp at the head of the
     *  written stamp queue.
     *
     *  This function is a pure virtual function and must be implemented by all the
     *  derived classes.
     *
     *  @param cycle Current simulation cycle.
     *  @param stamp Pointer to a stamp that has been written to the ROP data buffer
     *  and needs processing.
     *
     */

    virtual void postWriteProcess(U64 cycle, ROPQueue *stamp) = 0;

    /**
     *
     *  To be called after calling the update/clock function of the ROP Cache.
     *
     *  This function is a pure virtual function and must be implemented by all the
     *  derived classes.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    virtual void postCacheUpdate(U64 cycle) = 0;

    /**
     *
     *  This function is called when the ROP stage is in the the reset state.
     *
     *  This function is a pure virtual function and must be implemented by all the
     *  derived classes.
     *
     */

    virtual void reset() = 0;

    /**
     *
     *  This function is called when the ROP stage is in the flush state.
     *
     *  This function is a pure virtual function and must be implemented by all the
     *  derived classes.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    virtual void flush(U64 cycle) = 0;
    
    /**
     *
     *  This function is called when the ROP stage is in the swap state.
     *
     *  This function is a pure virtual function and must be implemented by all the
     *  derived classes.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    virtual void swap(U64 cycle) = 0;

    /**
     *
     *  This function is called when the ROP stage is in the clear state.
     *
     *  This function is a pure virtual function and must be implemented by all the
     *  derived classes.
     *
     */

    virtual void clear() = 0;

    /**
     *
     *  This function is called when a stamp is received at the end of the ROP operation
     *  latency signal and before it is queued in the operated stamp queue.
     *
     *  This function is a pure virtual function and must be implemented by all the
     *  derived classes.
     *
     *  @param cycle Current Simulation cycle.
     *  @param stamp Pointer to the ROP queue entry for the stamp that has to be operated.
     *
     */

    virtual void operateStamp(U64 cycle, ROPQueue *stamp) = 0;

    /**
     *
     *  This function is called when all the stamps but the last one have been processed.
     *
     *  This function is a pure virtual function and must be implemented by all the
     *  derived classes.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    virtual void endBatch(U64 cycle) = 0;

    /**
     *
     *  Processes a rasterizer command.
     *
     *  Pure virtual function.  Must be implemented by all the derived classes.
     *
     *  @param command The rasterizer command to process.
     *  @param cycle Current simulation cycle.
     *
     */

    virtual void processCommand(RasterizerCommand *command, U64 cycle) = 0;

    /**
     *
     *  Processes a register write.
     *
     *  Pure virtual function.  Must be implemented by all the derived classes.
     *
     *  @param reg The ROP stage register to write.
     *  @param subreg The register subregister to writo to.
     *  @param data The data to write to the register.
     *
     */

    virtual void processRegisterWrite(GPURegister reg, U32 subreg, GPURegData data) = 0;

public:

    /**
     *
     *  Generic ROP mdu constructor.
     *
     *  Creates and initializes a new Generic ROP mdu object.
     *
     *  @param stampsCycle Number of stamps per cycle.
     *  @param operationRate Rate at which stamp are operated (cycles between two stamps that are operated by the ROP).
     *  @param operationLatency ROP operation latency in cycles.
     *
     *  @param overW Over scan tile width in scan tiles (may become a register!).
     *  @param overH Over scan tile height in scan tiles (may become a register!).
     *  @param scanW Scan tile width in fragments.
     *  @param scanH Scan tile height in fragments.
     *  @param genW Generation tile width in fragments.
     *  @param genH Generation tile height in fragments.
     *
     *
     *  @param inQSz Input stamp queue size (in entries/stamps).
     *  @param fetchQSz Fetched stamp queue size (in entries/stamps).
     *  @param readQSz Read stamp queue size (in entries/stamps).
     *  @param opQSz Operated stamp queue size (in entries/stamps).
     *  @param writeQSz Written stamp queue size (in entries/stamps).
     *
     *  @param frEmu Reference to the Fragment Operation Behavior Model object.
     *
     *  @param ropName Name of the ROP stage.
     *  @param ropShortName Shorter/abbreviature of the ROP stage name.
     *  @param name The mdu name.
     *  @param prefix String used to prefix the mdu signals names.
     *  @param parent The mdu parent mdu.
     *
     *  @return A new Generic cmoMduBase object.
     *
     */

    GenericROP(

        //  Rate and latency parameters
        U32 stampsCycle, U32 operationRate, U32 operationLatency,

        //  ROP buffer parameters
        U32 overW, U32 overH, U32 scanW, U32 scanH, U32 genW, U32 genH,

        //  ROP queue parameters
        U32 inQSz, U32 fetchQSz, U32 readQSz, U32 opQSz, U32 writeQSz,

        //  Behavior Model classes associated with the Generic ROP pipeline
        bmoFragmentOperator &frOp,

        //  Other parameters
        char *ropName, char *ropShortName, char *name, char *prefix = 0, cmoMduBase* parent = 0);

    /**
     *
     *  Generic ROP simulation function.
     *
     *  Simulates a cycle of the Generic ROP mdu.
     *
     *  @param cycle The cycle to simulate of the Generic ROP mdu.
     *
     */

    void clock(U64 cycle);

    /**
     *
     *  Returns a single line string with state and debug information about the
     *  Generic ROP mdu.
     *
     *  @param stateString Reference to a string where to store the state information.
     *
     */

    void getState(string &stateString);

    /**
     *
     *  Writes into a string a report about the stall condition of the mdu.
     *
     *  @param cycle Current simulation cycle.
     *  @param stallReport Reference to a string where to store the stall state report for the mdu.
     *
     */
     
    void stallReport(U64 cycle, string &stallReport);

};

} // namespace arch

#endif

