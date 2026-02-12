/**************************************************************************
 *
 * Command Processor class definition file.
 * This file defines the Command Processor Unit.
 * The Command Processor mdu simulates the main control unit of a GPU.
 *
 */

#ifndef __CM_CMDPROC_H__
#define __CM_CMDPROC_H__


#include "GPUType.h"
#include "support.h"
#include "cmMduBase.h"
#include "GPUReg.h"
#include "cmShaderFetch.h"
#include "cmStreamer.h"
#include "TraceDriverBase.h"
#include "cmRasterizerStateInfo.h"
#include "ArchConfig.h"

namespace arch
{

/**
 *  Command Processor class.
 *
 *  This class implements a simple GPU command processor for the
 *  Shader Simulator.  Gets command (vertex, program, param) from
 *  a file and sends it to one or more shader units.
 */

class cmoCommandProcessor: public cmoMduBase
{

private:
    cgeModelAbstractLevel  CurMdlTyp;

    /**
     *
     *  Structure for storing buffered GPU register state changes.
     *
     */

    struct RegisterUpdate
    {
        GPURegister reg;    //  GPU register to be updated.  
        U32 subReg;      //  GPU register subregister identifier.  
        GPURegData data;    //  Data to write in the register.  
    };

    static const U32 MAX_REGISTER_UPDATES = 512; //  Defines the maximum number of register updates to store.  
    static const U32 GEOM_REG = 0;   //  Defines identifier for geometry phase registers.  
    static const U32 FRAG_REG = 1;   //  Defines identifier for geometry phase registers.  
    static const U32 CONSECUTIVE_DRAW_DELAY = 2; //  Defines the number of cycles between consecutive draw calls.  
    
    //  Command Processor parameters.  
    U32 numVShaders;         //  Number of vertex shaders controlled by the Command Processor.  
    U32 numFShaders;         //  Number of fragment shader units controlled by the Command Processor.  
    U32 TotalTextureUnits;     //  Number of texture units.  
    U32 numPerShaderTextureUnits;     //  Number of texture units.  
    U32 numStampUnits;       //  Number of stamp units (Z Stencil + Color Write) controlled by the Command Processor.  
    U32 memorySize;          //  Size of the GPU memory in bytes.  
    bool pipelinedBatches;      //  Enables/disables pipelined rendering of batches.  
    bool dumpShaderPrograms;    //  Dumps shaders loaded to a file.  

    //  Command Processor signals.  
    Signal** vshFCommSignal;    //  Array of the Shader Command signals to the Vertex Shader (Fetch) Units.  
    Signal** vshDCommSignal;    //  Array of the Shader Command signals to the Vertex Shader (Decode) Units.  
    Signal*  streamCtrlSignal;   //  Control signal to the cmoStreamController.  
    Signal*  streamStateSignal;  //  State signal from the cmoStreamController.  
    Signal*  paCommSignal;       //  Command signal to Primitive Assembly.  
    Signal*  paStateSignal;      //  State signal from Primitive Assembly.  
    Signal*  clipCommSignal;     //  Command signal to the Clipper.  
    Signal*  clipStateSignal;    //  State signal from the Clipper.  
    Signal*  rastCommSignal;     //  Command Processor Signal to the Rasterizer.  
    Signal*  rastStateSignal;    //  Rasterizer state signal to the Command Processor.  
    Signal** fshFCommandSignal; //  Array of command signals to the Fragment Shader Units (Fetch).  
    Signal** fshDCommandSignal; //  Array of command signals to the Fragment Shader Units (Decode).  
    Signal** tuCommandSignal;   //  Array of command signals to the Texture Units.  
    Signal** zStencilCommSignal;    //  Array of command signals to the Z Stencil Test unit.  
    Signal** zStencilStateSignal;   //  Array of state signals from the Color Write unit.  
    Signal** colorWriteCommSignal;  //  Array of command signals to the Color Write unit.  
    Signal** colorWriteStateSignal; //  Array of state signals from the Color Write unit.  
    Signal*  dacCommSignal;      //  Command signal to the DisplayController unit.  
    Signal*  dacStateSignal;     //  State signal from the DisplayController unit.  
    Signal*  readMemorySignal;   //  Read signal from the GPU local memoryController.  
    Signal*  writeMemorySignal;  //  Write Signal to the GPU local memoryController.  
    Signal*  mcCommSignal; //* Command signal to the memoryController 

    //  Command Processor state.  
    cgsGpuRegister state;             //  GPU state and registers.  
    cgoTraceDriverBase *driver;        //  Pointer to the trace driver from where to get CGD Transactions.  
    StreamerState streamState;  //  Current cmoStreamController unit state.  
    cgoMetaStream *lastMetaStreamTrans;   //  Pointer to the MetaStream being processed.  
    U08 *programCode;             //  Pointer to a buffer with the shader program being read.  
    RasterizerState *zStencilState;     //  Array for storing the state of the Z Stencil units.  
    RasterizerState *colorWriteState;   //  Array for storing the state of the Color Write units.  
    cgeGpuStatus stateStack;               //  Store the previous (stacked) GPU state.  For memory read/write pipelining support.  
    bool processNewTransaction;         //  Flag storing if a new MetaStream must be requested and processed.  
    bool geometryStarted;               //  Flag storing if the geometry phase of a batch rendering has started.  
    bool traceEnd;                      //  Determines if the trace has finished (no more MetaStreams available).  
    cgoMetaStream *loadFragProgram;    //  Buffers a load fragment program transaction.  
    bool storedLoadFragProgram;         //  Flag storing if there is a load fragment program transaction buffered.  
    cgoMetaStream *backupMetaStreamTrans;     //  Keeps the last MetaStream while processing the buffered load fragment program transaction.  
    bool backupProcNewTrans;            //  Keeps the last state of the process new transaction flag while processing the buffered load fragment program transaction.  
    U32 batch;                       //  Current batch.  
    bool skipDraw;                      //  Flag that stores if the Command Processor must ignore the next DRAW commands.  
    bool skipFrames;                    //  Flag that stores if the Command Processor must ignore GPU commands because it is skipping frames.  
    bool forceTransaction;              //  Flag that stores if an external MetaStream is forced into the GPU at the end of the current batch.  
    cgoMetaStream *forcedTransaction;  //  Stores the transaction that is forced on the GPU by an external agent.  
    bool colorWriteEnd;                 //  END command already sent to color write (swap).  
    bool zStencilTestEnd;               //  END command already sent to Z Stencil Test units (dump buffer).     
    RastComm dumpBufferCommand;         //  Stores the dump buffer command to issue to the DisplayController.  
    bool swapReceived;                  //  Stores if a swap command has been received in the last cycle.  
    bool batchEnd;                      //  Flag that stores if the batch has finished in the last cycle.  
    bool initEnd;                       //  Flag that stores if the initialization end transaction has been received.  
    bool commandEnd;                    //  Flag that stores if the last GPU command finished in the last cycle.  
    bool forcedCommand;                 //  Flag that stores if a forced command is being executed.  
    U32 flushDelayCycles;            //  Cycles to wait for HZ updates to finish.  
    U64 lastEventCycle[GPU_NUM_EVENTS];  //  Stores the last cycle events were signaled.  
    U32 drawCommandDelay;            //  Cycles to wait between consecutive draw commands.  

    U32 vshProgID;                   //  Vertex program identifier. 
    U32 fshProgID;                   //  Fragment program identifier.  
    U32 shProgID;                    //  Shader program identifier.  
     
    RegisterUpdate updateBuffer[2][MAX_REGISTER_UPDATES];   //  Buffers GPU register writes (for geometry and fragment registers).  
    U32 freeUpdates[2];      //  Number of free entries in the register update buffer.  
    U32 regUpdates[2];       //  Number of register updates stored in the update buffer.  
    U32 nextUpdate[2];       //  Pointer to the next register update in the update buffer.  
    U32 nextFreeUpdate[2];   //  Pointer to the next free entry in the register update buffer.  

    //  Memory access state.  
    MemState memoryState;       //  Stores current memory state.  
    U32 transCycles;         //  Stores the remaining cycles for the end of the current MetaStream.  
    U32 currentTicket;       //  Current ticket/id for the memory transactions.  
    U32 freeTickets;         //  Number of free tickets.  
    U32 requested;           //  Bytes requested to the Memory Controller.  
    U32 received;            //  Bytes received from the Memory Controller.  
    U32 sent;                //  Bytes sent to the Memory Controller.  
    U32 lastSize;            //  Size of the last memory transaction (read) received.  

    //  Command processor statistics.  
    gpuStatistics::Statistic &regWrites;    //  Number of register writes.  
    gpuStatistics::Statistic &bytesWritten; //  Bytes written to memory.  
    gpuStatistics::Statistic &bytesRead;    //  Bytes read from memory.  
    gpuStatistics::Statistic &writeTrans;   //  Write transactions.  
    gpuStatistics::Statistic &batches;      //  Number of batches processed.  
    gpuStatistics::Statistic &frames;       //  Number of frames processed (SWAP commands).  
    gpuStatistics::Statistic &bitBlits;     //  Number of Bit blit operations. 
    gpuStatistics::Statistic &readyCycles;  //  Cycles in ready state.  
    gpuStatistics::Statistic &drawCycles;   //  Cycles in draw state.  
    gpuStatistics::Statistic &endGeomCycles;//  Cycles in end geometry state.  
    gpuStatistics::Statistic &endFragCycles;//  Cycles in end fragment state.  
    gpuStatistics::Statistic &clearCycles;  //  Cycles in clear state.  
    gpuStatistics::Statistic &swapCycles;   //  Cycles in swap state.  
    gpuStatistics::Statistic &flushCycles;   //  Cycles in swap state.  
    gpuStatistics::Statistic &saveRestoreStateCycles;   //  Cycles in save/restore state for z/color state.  
    gpuStatistics::Statistic &bitBlitCycles;   //  Cycles in framebuffer blitting state. 
    gpuStatistics::Statistic &memReadCycles;    //  Cycles in memory read state.  
    gpuStatistics::Statistic &memWriteCycles;   //  Cycles in memory write state.  
    gpuStatistics::Statistic &memPreLoadCycles; //  Cycles in memory preload state.  

    //  Debug/Validation.
    bool enableValidation;                          //  Enables the validation mode.  
    U32 nextReadLog;                             //  Pointer to the next log where to read from.  
    U32 nextWriteLog;                            //  Pointer to the next log where to write from.  
    vector<cgoMetaStream*> agpTransactionLog[4];   //  Logs the MetaStreams received.  
    
    
    //  Private functions.  
    void PortBinding(void);

    /**
     *
     *  Starts the processing of a new MetaStream.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void processNewcgoMetaStream(U64 cycle);

    /**
     *  Continues the processing of the last MetaStream.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void processLastcgoMetaStream(U64 cycle);

    /**
     *
     *  Processes an META_STREAM_REG_WRITE transaction.
     *
     *  @param cycle Current simulation cycle.
     *  @param gpuReg GPU register to write.
     *  @param gpuSubReg GPU subregister to write.
     *  @param gpuData Data to write to the GPU register.
     *
     */

    void processGPURegisterWrite(U64 cycle, GPURegister gpuReg, U32 gpuSubReg, GPURegData gpuData);

    /**
     *
     *  Processes an META_STREAM_REG_READ transaction.
     *
     *  @param gpuReg The GPU register to read from.
     *  @param gpuSubReg The GPU subregister to read from.
     *  @param gpuData A reference to variable where to store
     *  the data read from the GPU register.
     *
     */

    void processGPURegisterRead(GPURegister gpuReg, U32 gpuSubReg, GPURegData &gpuData);

    /**
     *
     *  Processes a META_STREAM_COMMAND transaction.
     *
     *  @param cycle Current simulation cycle.
     *  @param gpuComm The GPU command to execute.
     *
     */

    void processGPUCommand(U64 cycle, GPUCommand gpuComm);

    /**
     *
     *  Processes a META_STREAM_EVENT transaction.
     *
     *  @param cycle Current simulation cycle.
     *  @param gpuEvent The event signaled to the GPU.
     *  @param eventMsg A string with a message associated with the event.
     *
     */
     
    void processGPUEvent(U64 cycle, GPUEvent gpuEvent, std::string eventMsg);
         
    /**
     *
     *  Processes a Memory Transaction from the Memory Controller.
     *
     *  Checks the Memory Controller state and accepts requested
     *  data from the Memory Controller.
     *
     *  @param memTrans The Memory Transaction to process.
     *
     */

    void processMemoryTransaction(MemoryTransaction *memTrans);

public:

    /**
     *
     *  Command Processor constructor.
     *
     *  This function initializes a cmoCommandProcessor object.  Creates and initializes the
     *  Command Processor internal state.  Creates the signals to the shaders.
     *
     *  @param driver Pointer to the Trace Driver from where to get MetaStreams.
     *  @param numVShaders Number of Vertex Shader Units controlled by the command processor.
     *  @param vshPrefixArray An array of strings with the prefix for each of the numVShader Vertex shader signals.
     *  @param numFShaders Number of Fragment Shader Units controlled by the Command Processor.
     *  @param fshPrefixArray Array of strings with the prefix of each Fragment Shader.
     *  @param TotalTextureUnits Number of texture units.
     *  @param tuPrefixArray Array of strings with the prefix of each Texture Unit.
     *  @param nStampUnits Number of stamp units (Z Stencil Test + Color Write) controlled by the Command Processor.
     *  @param suPrefixes Array of strings with the prefixes for each stamp unit.
     *  @param memorySize Size of the GPU memory in bytes.
     *  @param pipelinedBatches Enables/Disables pipelined rendering of batches.
     *  @param dumpShaders Enables/Disables the dumping of the shader programs being loaded to files.
     *  @param name Name of the Command Processor cmoMduBase.
     *  @param parent Pointer to a parent mdu.
     *  @return An initialized Command Processor.
     *
     */

    cmoCommandProcessor(
        cgsArchConfig* ArchConf,
        cgoTraceDriverBase* driver,
        char* name,
        cgeModelAbstractLevel  MdlTyp = CG_FUNC_MODEL,
        cmoMduBase *parent = NULL
        );


    /** Performs the simulation of the Command Processor.
     *  This function implements the simulation of the Command Processor
     *  for the given time cycle.
     *  @param cycle Cycle to simulate.
     */
    void clock(U64 cycle);

    /** Resets the GPU state.
     */
    void reset();
    
    /** Returns if the simulation of the trace has finished.
     *  @return TRUE if all the trace has been simulated, FALSE otherwise.
     */
    bool isEndOfTrace();

    /**
     *
     *  Returns if a swap command has been received.  Only valid for the
     *  cycle the command has been received.
     *
     *  @return TRUE if a swap command has been recieved in the last simulated
     *  cycle.
     *
     */

     bool isSwap();

    /**
     *
     *  Returns if the current batch has finished.  Only valid for the cycle the
     *  batch ends (END_FRAGMENT -> READY transition).
     *
     *  @return TRUE if the current batch has finished.
     *
     */

    bool endOfBatch();

    /**
     *
     *  Sets the skip draw command flag that enables/disables the execution of draw and swap commands
     *  by the Command Processor.
     *
     *  @param skip Boolean value to be assigned to the skip draw command flag.
     *
     */

    void setSkipDraw(bool skip);

    /**
     *
     *  Sets the skip frames command flag that enables/disables the execution of GPU commands while frames
     *  are being skipped.
     *
     *  @param skip Boolean value to be assigned to the skip frames command flag.
     *
     */

    void setSkipFrames(bool skip);

    /**
     *
     *  Returns a single line string with basic state and debug information about
     *  the Command Processor.
     *
     *  @param stateString A reference to a string where to store the state line.
     *
     */

    void getState(string &stateString);

    /** 
     *
     *  Returns a list of the debug commands supported by the Command Processor.
     *
     *  @param commandList Reference to a string where to store the list of debug commands supported by 
     *  the Command Processor.
     *
     */

    void getCommandList(string &commandList);

    /** 
     *
     *  Executes a debug command.
     *
     *  @param commandStream A string stream with the command and parameters to execute.
     *
     */

    void execCommand(stringstream &commandStream);


    /**
     *
     *  Returns if the an end of initialization phase transaction has been received.
     *
     *  @return TRUE if the end of initialization transaction has been received.
     *
     */
     
    bool endOfInitialization();

    /**
     *
     *  Returns if in the last cycle a GPU command finished it's execution.
     *
     *  @return TRUE a command finished in the last cycle.
     *
     */
     
    bool endOfCommand();

    /**
     *
     *  Returns if in the last cycle a forced GPU command finished it's execution.
     *
     *  @return TRUE a forced command finished in the last cycle.
     *
     */
     
    bool endOfForcedCommand();

    /**
     *
     *  Saves a snapshot of the current values of the GPU registers stored in the
     *  Command Processor to a file.
     *
     */
     
    void saveRegisters();
    
    /**
     *
     *  Get the MetaStream log.
     *
     *  @return A reference to the MetaStream log.
     *
     */
     
    vector<cgoMetaStream*> &getcgoMetaStreamLog();

    /** Enable/disable the validation mode in the Command Processor.
     *  @param enable Boolean value that defines if the validation mode in the Command Processor is set to enabled or disabled.
     */
    void setValidationMode(bool enable);
        
};

} // namespace arch

#endif
