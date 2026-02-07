/**************************************************************************
 *
 * Shader Unit Fetch cmoMduBase.
 *  This file defines the cmoShaderFetch cmoMduBase class (unified shader version).
 *  This mdu implements the simulation of the Fetch stage in a GPU Shader unit.
 *
 */
#ifndef _SHADERFETCH_

#define _SHADERFETCH_

#include "support.h"
#include "GPUType.h"
#include "cmMduBase.h"
#include "GPUReg.h"
#include "cmShaderCommon.h"
#include "bmUnifiedShader.h"
#include "cmShaderState.h"
#include "cmShaderInput.h"
#include "cmShaderCommand.h"
#include "cmShaderDecodeCommand.h"
#include "cmShaderExecInstruction.h"

namespace cg1gpu
{

/***
 *
 *  This type stores information about a shader thread.
 *
 */

struct ThreadTable
{
    bool ready;         //  Thread ready for execution (active threads only).  
    bool blocked;       //  Thread has been blocked.  
    bool free;          //  Thread is free (no data, no execution).  
    bool end;           //  The thread has finished the program (waiting for output read).  
    bool repeat;        //  Next instruction is a repeated instruction.  
    bool zexported;     //  Thread has executed the z export instruction.  
    U32 PC;          //  Thread PC (only for active threads).  
    U32 instructionCount;    //  Number of executed dynamic instructions for this thread.  
    ShaderInput *shInput;       //  Pointer to the thread shader input.  
    U64 nextFetchCycle;      //  Marks the next cycle this thread will be allowed to be fetched.  
};


/**
 *
 *  cmoShaderFetch implements the Fetch stage of a Shader.
 *
 *  Inherits from cmoMduBase class.
 *
 */

class cmoShaderFetch : public cmoMduBase
{

private:

    //  Renames the shader partitions.  
    static const U32 PRIMARY_PARTITION = 0;
    static const U32 SECONDARY_PARTITION = 1;

    //  Patched address for the triangle setup program.  
    static const U32 TRIANGLE_SETUP_PROGRAM_PC = 384;

    //  Shader signals.  
    Signal *commProcSignal[2];  //  Command signal from the Command Processor.  
    Signal *commandSignal;      //  Command signal from cmoStreamController.  
    Signal *readySignal;        //  Ready signal to the cmoStreamController.  
    Signal *instructionSignal;  //  Instruction signal to Decode/Execute  
    Signal *newPCSignal;        //  New PC signal from Decode/Execute.  
    Signal *decodeStateSignal;  //  Decoder state signal from Decode/Execute.  
    Signal *outputSignal;       //  Shader output signal to a consumer.  
    Signal *consumerSignal;     //  Consumer readyness state to receive Shader output.  
    Signal *shaderZExport;      //  Z export signal to the ShaderWorkDistributor (MicroPolygon Rasterizer).  

    //  Shader State.  
    U32 currentThread;       //  Pointer to the current thread from where to fetch an instruction.  
    U32 nextThread;          //  Pointer to the next thread from where to fetch an instruction.  
    U32 nextThreadInGroup;   //  Pointer to the next thread in a group from which to fetch instructions.  
    bool readyThreadGroup;      //  Stores if there is a ready thread group.  
    U32 groupPC;             //  Keeps the PC for the current thread group.  
    bool transInProgress;       //  Shader Output transmission in progress.  
    U32 transCycles;         //  Remaining Shader Output transmission cycles.  
    U32 currentOutput;       //  Number of shader outputs being currently transmited.  
    ShaderState shState;        //  State of the Shader.  
    U32 activeOutputs[3];    //  Number outputs currently active (written and send back) per shader target.  
    U32 activeInputs[3];     //  Number of shader inputs active for the current shader program per shader target .  
    bool fetchedSIMD;           //  Stores if a SIMD instruction was fetched for the current thread.  
    bool fetchedScalar;         //  Stores if a scalar instruction was fetched for the current thread.  

    //  Shader structures.  
    ThreadTable *threadTable;   //  Table with information about the state of the threads in the Shader.  
    U32 *freeBufferList;     //  List of free shader buffer/threads.  
    U32 numFreeThreads;      //  Number of free threads.  
    U32 numReadyThreads;     //  Number of ready threads.  
    U32 firstFilledBuffer;   //  Pointer to the entry in the free buffer list with the first (FIFO order) filled buffer.  
    U32 firstEmptyBuffer;    //  Pointer to the entry in the free buffer list with the first empty buffer.  
    U32 lastEmptyBuffer;     //  Pointer to the entry in the free buffer list with the last empty buffer.  
    U32 numFinishedThreads;  //  Number of threads that have already finished and are waiting for their output to be trasmited.  
    U32 *finThreadList;      //  List of finished threads.  
    U32 firstFinThread;      //  Pointer to the first (FIFO) finished thread in the finished thread list.  
    U32 lastFinThread;       //  Pointer to the last (FIFO) finished thread in the finished thread list.  
    U32 freeResources;       //  Number of free thread resources.  

    //  FIFO execution mode state variables.  
    bool batchEnding[3];            //  Flag that stores if the current batch is just waiting for all the groups to finish.  
    bool closedBatch[3][4];         //  Two flags store if the current and next shader input batch are closed and don't accepts new shader inputs.  
    U32 *batchGroupQueue[3][4];  //  Two queues storing the thread groups that form the current and next shader input batch.  
    U32 fetchBatch[3];           //  Pointers to the shader input batch from which instructions can be fetch.  
    U32 loadBatch[3];            //  Pointers to the shader input batch to which new groups are being loaded.  
    U32 nextFetchInBatch[3];     //  Pointers to the next fetch entry in the fetch batch queues.  
    U32 lastInBatch[3][2];       //  Pointers to the last group in the batch queues.  
    U64 lastLoadCycle[3];        //  Last cycle a group was loaded into a batch queue.  
    U32 batchPC[3];              //  Current PC of a shader input batch.  
    U32 currentBatchPartition;   //  Pointer to the shader input batch partition currently active.  

    //  Thread Window execution mode structures and state variables.  
    U32 *readyThreads;       //  Counter per thread group storing the number of ready threads in the group.  
    U32 *finThreads;         //  Counter per thread group storing the number of finished threads in the group.  
    U32 *readyGroups;        //  Queue of ready thread groups.  
    bool *groupBlocked;         //  Flag storing if a group blocked and out of any of the group queues.  
    U32 numGroups;           //  Number of thread groups in the shader.  
    U32 numReadyGroups;      //  Number of ready groups.  
    U32 nextReadyGroup;      //  Pointer to the next ready group in the queue.  
    U32 nextFreeReady;       //  Pointer to the next free entry in the ready groups queue.  
    U32 nextGroup;           //  Stores the group from which instructions are being fetched.  

    U32 *fetchedGroups;      //  Queue with the thread groups that have been fetched in the last cycles.  
    U32 numFetchedGroups;    //  Number of fetched groups in the queue.  
    U32 nextFetchedGroup;    //  Pointer to the next fetched group.  
    U32 nextFreeFetched;     //  Pointer to the next free entry in the fetched group queue.  

    //  Shader Z Export state.  
    U32 pendingZExports;           //  Number of pending z exports to be sent.  
    U32 currentZExportThread;  //  Stores the index of the current thread being processed for z exports.  

    //  Shader instruction group buffer.  
    ShaderExecInstruction ***fetchGroup;//  Stores the fetched instructions for a thread group.  

    //  Shader registers.  
    U32 initPC[MAX_SHADER_TARGETS];           //  Shader program initial PC per partition.  
    U32 threadResources[3];  //  Per thread resource usage per partition.  
    bool output[3][MAX_VERTEX_ATTRIBUTES];  //  Which shader output registers (written outputs) are going to be sent back per target/partition.  
    bool input[3][MAX_VERTEX_ATTRIBUTES];   //  Which shader input registers are enabled for the current program and target/partition.  

    //  Shader derived pseudoconstants.  
    U32 maxThreadResources;  //  Maximum per thread resources of both partitions.  

    //  Shader Parameters.  
    bmoUnifiedShader &bmUnifiedShader;     //  Reference to a bmoUnifiedShader object.  
    bool unifiedModel;          //  Simulate an unified shader model.  
    U32 threadsCycle;        //  Threads from which to fetch instructions per cycle.  
    U32 instrCycle;          //  Number of instructions to fetch each cycle per thread. 
    U32 threadGroup;         //  Number of threads that form a group.  
    bool lockStep;              //  Enables/disables lock step execution mode where a thread group executes in lock step (same instructions fetched per cycle).  
    bool scalarALU;             //  Enable SIMD + scalar fetch.  
    U32 numThreads;          //  Number of threads supported by the Shader  
    U32 numInputBuffers;     //  Number of Shader Input buffers.  
    U32 numBuffers;          //  Number of shader buffers (input buffers + thread state).  
    U32 numResources;        //  Number of thread resources (registers?).  
    bool threadWindow;          //  Flag that enables to search the thread queue for ready threads as it was a window.  
    U32 fetchDelay;          //  Number of cycles until a thread can be fetched again.  
    bool swapOnBlock;           //  Swap the current active thread on blocks only.  
    U32 textureUnits;        //  Number of texture units attached to the shader.  
    U32 inputCycle;          //  Shader inputs that can be received per cycle.  
    U32 outputCycle;         //  Shader outputs that can be sent per cycle.  
    U32 maxOutLatency;       //  Maximum latency of the output signal from the shader to the consumer.  
    bool multisampling;         //  Flag that stores if MSAA is enabled.  
    U32 msaaSamples;         //  Number of MSAA Z samples generated per fragment when MSAA is enabled.  

    //  Statistics.  
    gpuStatistics::Statistic *fetched;      //  Number of fetched instructions.  
    gpuStatistics::Statistic *reFetched;    //  Number of refetched instructions.  
    gpuStatistics::Statistic *inputs;       //  Number of shader inputs.  
    gpuStatistics::Statistic *outputs;      //  Number of shader outputs.  
    gpuStatistics::Statistic *inputInAttribs;   //  Number of input attributes active per input.  
    gpuStatistics::Statistic *inputOutAttribs;  //  Number of output attributes active per input.  
    gpuStatistics::Statistic *inputRegisters;   //  Number of temporal registers used per input.  
    gpuStatistics::Statistic *blocks;       //  Number of block command received.  
    gpuStatistics::Statistic *unblocks;     //  Number of unblocking command received.  
    gpuStatistics::Statistic *nThReady;     //  Number of ready threads.  
    gpuStatistics::Statistic *nThReadyAvg;  //  Number of ready threads.  
    gpuStatistics::NumericStatistic<U32> *nThReadyMax;  //  Number of ready threads.  
    gpuStatistics::NumericStatistic<U32> *nThReadyMin;  //  Number of ready threads.  
    gpuStatistics::Statistic *nThBlocked;   //  Number of blocked threads.  
    gpuStatistics::Statistic *nThFinished;  //  Number of finished threads.  
    gpuStatistics::Statistic *nThFree;      //  Number of free threads.  
    gpuStatistics::Statistic *nResources;   //  Number of used resources.  
    gpuStatistics::Statistic *emptyCycles;  //  Counts the cycles there is no work to be done.  
    gpuStatistics::Statistic *fetchCycles;  //  Counts the cycles instructions are fetched.  
    gpuStatistics::Statistic *noReadyCycles;//  Counts the cycles there is no ready thread from which to fetch instructions.  

    //  Private functions.  

    /**
     *
     *  Returns the number of a free thread if available.
     *
     *  @param partition The partition (vertex/fragment) for which the new thread
     *  is allocated.
     *
     *  @return The identifier of a free thread or -1 if there is no
     *  free threads available.
     *
     */

    S32 getFreeThread(U32 partition);

    /**
     *
     *  Sets a thread free.
     *
     *  @param numThread Number of the thread to set free.
     *  @param partition The partition (vertex/fragment) for which the thread
     *  is deallocated.
     *
     */

    void addFreeThread(U32 numThread, U32 partition);

    /**
     *
     *  Reloads a finished thread that which output has been already
     *  transmitted with new input data.
     *
     *  @param cycle Current simulation cycle.
     *  @param nThread A thread that has finished its ouput transmission.
     *
     */

    void reloadThread(U64 cycle, U32 nThread);

    /**
     *  Finish a thread.
     *
     *  Marks a thread as finished and adds the thread to the list of
     *  finished threads.
     *
     *  @param nThread A thread that has finished.
     *
     */

    void finishThread(U32 nThread);

    /**
     *
     *  Processes a Shader Command.
     *
     *  @param shComm The ShaderCommand to process.
     *  @param partition Identifies command for vertex or fragment partition in the
     *  unified shader model.
     *
     */

    void processShaderCommand(ShaderCommand *shComm, U32 partition);

    /**
     *
     *  Processes a new Shader Input.
     *
     *  @param cycle Current simulation cycle.
     *  @param input The new shader input.
     *
     */

    void processShaderInput(U64 cycle, ShaderInput *input);

    /**
     *
     *  Processes a control command from Decode stage.
     *
     *  @param decodeCommand A control command received from the decode stage.
     *
     */

    void processDecodeCommand(ShaderDecodeCommand *decodeCommand);

    /**
     *
     *  Fetches a shader instruction for a thread.
     *
     *  @param cycle Current simulation cycle.
     *  @param threadID Thread from which to fetch the instruction.
     *  @param shExecInstr Reference to a pointer where to store the
     *  dynamic shader instruction fetched.
     *
     */

    void fetchInstruction(U64 cycle, U32 threadID, ShaderExecInstruction *& shExecInstr);

    /**
     *
     *  Fetches a shader instruction and sends it to decode stage.
     *
     *  @param cycle Current simulation cycle.
     *  @param threadID Thread from which to fetch the instruction.
     *
     */

    void fetchInstruction(U64 cycle, U32 threadID);

    /**
     *
     *  Fetches all the instructions for a wave and fetch cycle.
     *
     *  @param cycle Current simulation cycle.
     *  @param threadID First thread in the group from which to fetch the instructions.
     *
     */

    void fetchInstructionGroup(U64 cycle, U32 threadID);

    /**
     *
     *  Sends a shader instruction it to the shader decode stage.
     *
     *  @param cycle Current simulation cycle.
     *  @param threadID Thread identifier of the instruction to send.
     *  @param shExecInstr Dynamic shader instruction to send to shader decode.
     *
     */

    void sendInstruction(U64 cycle, U32 threadID, ShaderExecInstruction *shExecInstr);

    /**
     *
     *  Sends the shader output for a thread to the shader consumer unit.
     *
     *  @param cycle Current simulation cycle.
     *  @param outputThread The thread for which to send the output to the consumer unit.
     *
     */

    void sendOutput(U64 cycle, U32 outputThread);


    /**
     *
     *  Computes the maximum number of resources required for a new shader thread taking into account
     *  all the shader targets.
     *
     *  @return The maximum number of resources required fo a new shader thread.
     *
     */
     
    U32 computeMaxThreadResources();
     
public:

    /**
     *
     *  Constructor for cmoShaderFetch.
     *
     *  Creates a new cmoShaderFetch object ready for start the simulation.
     *
     *  @param bmShader Reference to a bmoUnifiedShader object that will be
     *  used to emulate the Shader Unit.
     *  @param nThreads  Number of threads in the Shader Unit.
     *  @param nInputBuffers  Number of Input Buffers in the Shader Unit.
     *  @param resources Number of thread resources (registers?) available.
     *  @param threadsPerCycle Number of threads from which to fetch instructions per cycle.
     *  @param instrPerCycle Number of instructions fetched per thread and cycle.
     *  @param threadsGroup Threads per group.
     *  @param lockStepMode If the lock step execution mode is enabled.
     *  @param scalarALU Enables to fetch one SIMD + scalar instruction per cycle.
     *  @param thWindow Enables to search ready threads between all the available threads.
     *  @param fetchDelay Cycles until a thread can be fetched again.
     *  @param swapOnBlock Swap the active thread only when it becomes blocked.
     *  @param texUnits Number of texture units attached to the shader.
     *  @param inCycle Shader inputs received per cycle.
     *  @param outCycle Shader outputs sent per cycle.
     *  @param outLatency Maximum latency of the output signal to the consumer.
     *  @param microTrisAsFrag Process microtriangle fragment shader inputs and export z values (when MicroPolygon Rasterizer enabled only).
     *  @param name Name of the cmoShaderFetch mdu.
     *  @param shPrefix Primary shader prefix.   For non unified model is the prefix for the
     *  shader fetch mdu.  Used as fragment shader prefix (signals from the Command Processor)
     *  for the unified shader model.
     *  @param sh2Prefix Primary shader prefix.  Not used for the non unified model.  For unified shader
     *  model is the virtual vertex shader prefix (signals from command processor).
     *  @param parent Pointer to the parent mdu for this mdu.
     *
     *  @return A new cmoShaderFetch object initializated.
     *
     */

    cmoShaderFetch(bmoUnifiedShader &bmShader, U32 nThreads, U32 nInputBuffers, U32 resources,
        U32 threadsPerCycle, U32 instrPerCycle, U32 threadsGroup, bool lockStepMode, bool scalarALU,
        bool thWindow, U32 fetchDelay, bool swapOnBlock, U32 texUnits, U32 inCycle, U32 outCycle,
        U32 outLatency, bool microTrisAsFrag, char *name, char *shPrefix = 0, char *sh2Prefix = 0,
        cmoMduBase* parent = 0);

    /**
     *
     *  Clock rutine.
     *
     *  Carries the simulation of the Shader Fetch.  It is called
     *  each clock to perform the simulation.
     *
     *  @param cycle  Cycle to simulate.
     *
     */

    void clock(U64 cycle);

    /**
     *
     *  Returns a single line string with state and debug information about the
     *  Shader Fetch mdu.
     *
     *  @param stateString Reference to a string where to store the state information.
     *
     */

    void getState(string &stateString);

};

} // namespace cg1gpu

#endif
