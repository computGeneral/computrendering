/**************************************************************************
 *
 * Vector Shader Unit Decode and Execute cmoMduBase.
 *
 */

/**
 *  @file VectorcmShaderDecodeExecute.h
 *
 *  This file defines a cmoShaderDecExeVector cmoMduBase.
 *
 *  Defines and implements a simulator cmoMduBase for the Decode,
 *  Execute and WriteBack stages of a Shader pipeline.
 *
 */

#ifndef _VECTORSHADERDECODEXECUTE_

#define _VECTORSHADERDECODEXECUTE_

#include "cmMultiClockMdu.h"
#include "cmShaderCommon.h"
#include "cmShaderState.h"
#include "ShaderInstr.h"
#include "cmShaderDecodeCommand.h"
#include "cmShaderDecodeStateInfo.h"
#include "cmShaderExecInstruction.h"
#include "cmShaderCommand.h"
#include "bmUnifiedShader.h"
#include "cmShaderFetchVector.h"
#include "cmtoolsQueue.h"

namespace cg1gpu
{

/**
 *
 *  Defines a structure that contains information about a shader thread
 *  in the decode and execution stages.
 *
 */

struct VectorThreadInfo
{
    bool tempBankDep[MAX_TEMP_BANK_REGS];       //  Stores pending writes in the temporary register bank.  
    bool addrBankDep[MAX_ADDR_BANK_REGS];       //  Stores pending writes in the address register bank.  
    bool outpBankDep[MAX_OUTP_BANK_REGS];       //  Stores pending writes in the output register bank.  
    bool predBankDep[MAX_PRED_BANK_REGS];       //  Stores pending writes in the predicate register bank.  
    U64 tempBankWrite[MAX_TEMP_BANK_REGS];   //  Stores the cycle a temporary register will be written.  
    U64 addrBankWrite[MAX_ADDR_BANK_REGS];   //  Stores the cycle an address register will be written.  
    U64 outpBankWrite[MAX_OUTP_BANK_REGS];   //  Stores the cycle an output register will be written.  
    U64 predBankWrite[MAX_PRED_BANK_REGS];   //  Stores the cycle an predicate register will be written.  
    U32 pendingInstructions;                 //  Stores the number of instructions that have not finished yet execution.  
    U32 pendingTexElements;                  //  Counter that stores the number of texture fetches pending for elements in the vector thread.  
    bool ready;                                 //  Stores if the thread is active, not blocked by a previous instruction.  
    bool waitTexture;                           //  Stores if the thread is waiting for the result of a texture instruction.  
    bool end;                                   //  Stores if the thread has received the END instruction and is waiting for the last instructions to end execution.  
    bool pendingJump;                           //  Stores if the thread has received a JMP instruction and is waiting until it is executed.  
    bool zexport;                               //  Stores if the thread has executed the z exported instruction.  
};

/**
 *
 *  This class implements the cmoMduBase that simulates the Decode, Execute and WriteBack stages of a Shader pipeline.
 *
 */

class cmoShaderDecExeVector: public cmoMduMultiClk
{

private:

    //  Renames the shader partitions.
    static const U32 PRIMARY_PARTITION = 0;
    static const U32 SECONDARY_PARTITION = 1;

    //  Shader Decode Execute parameters.
    bmoUnifiedShader &bmUnifiedShader;         //  Reference to a bmoUnifiedShader object that implements the emulation of the Shader Unit.  
    U32 vectorThreads;           //  Number of vector threads supported by the Shader Unit.  
    U32 vectorLength;            //  Number of elements in a shader vector.  
    U32 vectorALUWidth;          //  Number of ALUs in the vector ALU array.  Each ALU executes the instruction for an element of the input vector.  
    char* vectorALUConfig;          //  Configuration of the ALUs in the vector array.  
    bool waitOnStall;               //  Flag that defines if when a vector shader instruction must be stalled the instruction waits in the decode stage or it's discarded and repeated (refetched).  
    bool explicitBlock;             //  Flag that defines if threads are blocked by an explicit trigger in the instruction stream or automatically on texture load.  
    U32 textureClockRatio;       //  Ratio between texture unit clock and shader clock.  Used for the texture blocked thread wakeup queue.  
    U32 textureUnits;            //  Number of texture units attached to the shader.  
    U32 textureRequestRate;      //  Number of texture requests send to a texture unit per cycle.  
    U32 requestsPerTexUnit;      //  Number of texture request to send to each texture unit before skipping to the next one.  
    U32 instrBufferSize;         //  Number of entries in the instruction buffer, an entry stores threadsCycle * instrCycle instructions.  

    //  Derived from the parameters.
    U32 instrCycle;              //  Instructions fetched/decoded/executed per cycle.  
    bool scalarALU;                 //  Architecture supports an extra scalar OP per cycle.  
    bool soaALU;                    //  Scalar architecture (scalar only vector ALU).  
    U32 execCyclesPerVector;     //  Number of execution iterations/cycles for a shader vector (vectorLength/vectorALUWidth).  

    //  Shader Decode Execute signals.
    Signal *commandSignal[2];       //  Command signal from the Command Processor.  
    Signal *instructionSignal;      //  Input signal from the ShaderFetch mdu.  Carries new instructions.  
    Signal *controlSignal;          //  Output signal to the ShaderFetch mdu.  Carries control flow changes.  
    Signal *executionStartSignal;   //  Output signal to the ShaderDecodeExecute mdu.  Starts an instruction execution.  
    Signal *executionEndSignal;     //  Input Signal from the ShadeDecodeExecute mdu.  End of an instruction execution.  
    Signal *decodeStateSignal;      //  Decoder state signal to the ShaderFetch mdu.  
    Signal **textRequestSignal;     //  Texture access request signal to the Texture Unit.  
    Signal **textResultSignal;      //  Texture result signal from the Texture Unit.  
    Signal **textUnitStateSignal;   //  State signal from the Texture Unit.  

    //  State of Decode Execute stages.
    ShaderState state;              //  Current state of the decode execute shader stages.  
    VectorThreadInfo *threadInfo;   //  Table with information about the shader threads in the decode execute stage.  
    U32 regWrites[MAX_EXEC_LAT]; //  Stores the number of register writes for the next MAX_EXEC_LAT cycles.  
    U32 nextRegWrite;            //  Points to the entry in the register writes table for the current cycle.  
    bool reserveSIMDALU;            //  Reserves (one ALU per supported thread rate) the SIMD ALU for an instruction to be initiated in the current cycle.  
    bool reserveScalarALU;          //  Reserves (one ALU per supported thread rate) the scalar ALU for an instruction to be initiated in the current cycle.  
    U32 *textureTickets;         //  Number of texture tickets available that allow to send a texture request to the Texture Unit.  
    U32 nextRequestTU;           //  Pointer to the next texture unit where to send texture requests.  
    U32 tuRequests;              //  Number of texture requests send to the current texture unit.  
    
    bool vectorFetchAvailable;      //  A vector fetch was received from the fetch stage and is available for decode/execution.  
    U32 execInstructions;        //  Stores the number of instructions that can be executed after performing the decode.  
    bool executingVector;           //  Flag that stores if a vector instruction is on execution.  
    U32 execElement;             //  Index to the next element of the vector to be executed.  
    U32 endExecElement;          //  Index to the next element of the vector to end the execution.  
    U32 currentRepeatRate;       //  Stores the repeat rate (cycles between element execution start) for the current decoded instruction fetch.  
    U32 cyclesBetweenFetches;    //  Counter that stores the number of cycles expected between two vector fetches.  
    U32 cyclesToNextFetch;       //  Counter that stores the number of cycles until the next fetch is expected.  
    U32 cyclesToNextExec;        //  Counter that stores the number of cycles until the next execution of shader elements can be started.  

    tools::Queue<U32> threadWakeUpQ;     //  Stores the identifier of threads pending from being awakened by Texture Unit.  
    
    ShaderExecInstruction ***vectorFetch;   //  Array that stores a vector instruction fetch.  Includes the Shader Behavior Model instructions for all the elements in the vector.  
    
    //  Aux structures.
    U32 *textThreads;            //  Auxiliary array to retrieve the number of the threads that terminate a texture access.  

    //  Helper classes.
    ShaderArchParam *shArchParams; //  Pointer to the Shader Architecture Parameters singleton.  

    //  Statistics.
    gpuStatistics::Statistic *execInstr;    //  Number of executed instructions.  
    gpuStatistics::Statistic *blockedInstr; //  Number of instructions blocked at decode.  
    gpuStatistics::Statistic *removedInstr; //  Number of instructions removed without being executed at decode.  
    gpuStatistics::Statistic *fakedInstr;   //  Number of faked instructions ignored at decode.  
    gpuStatistics::Statistic *blocks;       //  Block commands sent to Shader Fetch.  
    gpuStatistics::Statistic *unblocks;     //  Unblock commands sent to Shader Fetch.  
    gpuStatistics::Statistic *ends;         //  End commands sent to Shader Fetch.  
    gpuStatistics::Statistic *replays;      //  Replay commands sent to Shader Fetch.  
    gpuStatistics::Statistic *textures;     //  Texture accesses sent to Texture Unit.  
    gpuStatistics::Statistic *zexports;     //  Number of executed Z export instructions.  

    /**
     *
     *  Sends a new PC command to Shader Fetch.
     *
     *  @param cycle Current simulation cycle.
     *  @param numThread  Thread for which to change the PC.
     *  @param PC New PC for the thread.
     *  @param shExecInstr Pointer to the dynamic instruction.
     *
     */

    void changePC(U64 cycle, U32 numThread, U32 PC, ShaderExecInstruction *shExecInstr);

    /**
     *
     *  Sends a replay last instruction command to Shader Fetch.
     *
     *  @param cycle  Current simulation cycle.
     *  @param numThread  Thread of the instruction.
     *  @param PC PC of the instruction.
     *  @param shExecInstr Pointer to the dynamic instruction.
     *
     */

    void replayInstruction(U64 cycle, U32 numThread, U32 PC, ShaderExecInstruction *shExecInstr);

    /**
     *
     *  Send a block thread command to Shader Fetch.
     *
     *  @param cycle  Current simulation cycle.
     *  @param numThread  Thread of the instruction.
     *  @param PC PC of the instruction.
     *  @param shExecInstr Pointer to the dynamic instruction.
     *
     */

    void blockThread(U64 cycle, U32 numThread, U32 PC, ShaderExecInstruction *shExecInstr);

    /**
     *
     *  Send an unblock thread command to Shader Fetch.
     *
     *  @param cycle  Current simulation cycle.
     *  @param numThread  Thread of the instruction.
     *  @param PC PC of the  instruction.
     *  @param shExecInstr Pointer to the dynamic instruction.
     *
     */

    void unblockThread(U64 cycle, U32 numThread, U32 PC, ShaderExecInstruction *shExecInstr);

    /**
     *
     *  Send an end thread command to Shader Fetch.
     *
     *  @param cycle  Current simulation cycle.
     *  @param numThread  Thread of the instruction.
     *  @param PC PC of the instruction.
     *  @param shExecInstr Pointer to the dynamic instruction.
     *
     */

    void endThread(U64 cycle, U32 numThread, U32 PC, ShaderExecInstruction *shExecInstr);

    /**
     *
     *  Sends a thread's z export executed command to Shader Fetch.
     *
     *  @param cycle  Current simulation cycle.
     *  @param numThread  Thread of the instruction.
     *  @param PC PC of the instruction.
     *  @param shExecInstr Pointer to the dynamic instruction.
     *
     */

    void zExportThread(U64 cycle, U32 numThread, U32 PC, ShaderExecInstruction *shExecInstr);

    /**
     *
     *  Processes a command from the Command Processor unit.
     *
     *  @param shComm Pointer to a Shader Command receives from the Command Processor.
     *
     */

    void processCommand(ShaderCommand *shCom);

    /**
     *
     *  Resets the vector shader decode and execute stages state.
     *
     *
     */
        
    void reset();

    /**
     *
     *  Receives a vector instruction fetch from the vector shader fetch stage.
     *
     *  @param cycle The current simulation cycle.
     *
     */

    void receiveVectorInstruction(U64 cycle);

    /**
     *
     *  Updates the state at the decode state and decodes shader instructions.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void decodeStage(U64 cycle);
    
    /**
     *
     *  Updates the state at the execute (start of the pipeline) stage and executes instructions.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void executeStage(U64 cycle);
   

    /**
     *
     *  Updates the state at the write back/execution pipeline end stage.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void writeBackStage(U64 cycle);

    /**
     *
     *  Send decode stage state to the fetch stage.
     *
     *  @param cycle Current simulation cycle.
     *
     */
     
    void sendDecodeState(U64 cycle);
    
    /**
     *
     *  Sends requests to the texture units.
     *
     *  @param cycle Current simulation cycle.
     *
     */
    
    void sendRequestsToTextureUnits(U64 cycle);
    
    /**
     *
     *  Receive results from the texture units.
     *
     *  @param cycle Current simulation cycle.
     *
     */
     
    void receiveResultsFromTextureUnits(U64 cycle);

    /**
     *
     *  Receive state from the Texture units.
     *
     *  @param cycle Current simulation cycle.
     *
     */
    
    void receiveStateFromTextureUnits(U64 cycle);    

    /**
     *
     *  Wake up threads pending from texture unit.
     *
     *  @param cycle Current simulation cycle.
     *
     */
     
    void wakeUpTextureThreads(U64 cycle);

    /**
     *
     *  Receive updates from the Command Processor.
     *
     *  @param cycle Current simulation cycle.
     *
     */
    
    void updatesFromCommandProcessor(U64 cycle);

    /**
     *
     *  Starts the execution of a shader instruction.
     *
     *  @param cycle The current simulation cycle.
     *  @param elem The element index in the vector being executed.
     *  @param shExecInstr The shader instruction to execute.
     *
     */

    void startExecution(U64 cycle, U32 instruction, ShaderExecInstruction *shExecInstr);

    /**
     *
     *  Decodes a shader instruction.
     *
     *  Processes instructions that must be repeated.  Sends a repeat command
     *  to the shader fetch unit.
     *
     *  @param cycle The current simulation cycle.
     *  @param instruction The instruction in the vector instruction fetch that is stalled (repeated).
     *
     */

    void repeatInstruction(U64 cycle, U32 instruction);

    /**
     *
     *  Decodes a shader instruction.
     *
     *  Determines the dependences of the incoming shader instruction with
     *  the shader instructions being executed.  Blocks and ignores instructions
     *  with dependences, instructions received for a blocked thread or a thread
     *  that has already received the END instruction.
     *
     *  @param cycle The current simulation cycle.
     *  @param instruction The instruction in the vector instruction fetch that is going to be decoded.
     *  @param exec Reference to a boolean variable where to store if the instruction can be executed.
     *  @param stall Reference to a boolean variable where to store if the instruction has to be stalled due to a dependence.
     *
     */

    void decodeInstruction(U64 cycle, U32 instruction, bool &exec, bool &stall);

    /**
     *
     *  Update decode stage structures (dependence checking, write port limitations).
     *
     *  @param cycle The current simulation cycle.
     *  @param shExecInstr The shader instruction for which to update the decode stage.
     *
     */
     
    void updateDecodeStage(U64 cycle, ShaderExecInstruction *shExecInstr);

    /**
     *
     *  Determines if there is a RAW dependence for a register.
     *
     *  @param numThread Thread identifier of the instruction for which to check RAW dependences.
     *  @param bank Bank of the register to check for RAW.
     *  @param reg Register to check for RAW.
     *
     *  @return TRUE if there is a RAW dependence for the specified register and thread
     *  with a previous instruction that has not finished its execution yet.  FALSE if there is
     *  no dependence.
     *
     */

    bool checkRAWDependence(U32 numThread, Bank bank, U32 reg);

    /**
     *
     *  Clears register write dependences.   Sets to false the dependence flag for
     *  the instruction result register after the instruction has finished execution
     *  or the instructions has been removed.
     *
     *  @param cycle The current simulation cycle.
     *  @param shInstr Instruction for which to clear write dependences.
     *  @param numThread Thread for which to clear the write dependences.
     *
     */

    void clearDependences(U64 cycle, cgoShaderInstr *shInstr, U32 numThread);


    /*
     *  Simulate and update the state of the GPU domain section of the vector shader decode execute mdu.
     *
     *  @param cycle Current simulation cycle.
     *
     */
     
    void updateGPUDomain(U64 cycle);

    /*
     *  Simulate and update the state of the shader domain section of the vector shader decode execute mdu.
     *
     *  @param cycle Current simulation cycle.
     *
     */
     
    void updateShaderDomain(U64 cycle);

    
    /**
     *
     *  Prints the values of the operands for the decoded shaded instruction.
     *  Should be called before the execution of the instruction.
     *
     *  @param shInstrDec Pointer to a cgoShaderInstrEncoding object.
     *
     */
     
    void printShaderInstructionOperands(cgoShaderInstr::cgoShaderInstrEncoding *shInstrDec);
       
    /**
     *
     *  Prints the values of the results for the decoded shaded instruction.
     *  Should be called after the execution of the instruction.
     *
     *  @param shInstrDec Pointer to a cgoShaderInstrEncoding object.
     *
     */
     
    void printShaderInstructionResult(cgoShaderInstr::cgoShaderInstrEncoding *shInstrDec);

public:

    /**
     *
     *  Constructor for the class.
     *
     *  Creates a new initializated ShaderDecodeExecute mdu that implements
     *  the simulation of the Decode/Read/Executed/WriteBack stages of a
     *  Shader Unit.
     *
     *  @param bmUnifiedShader Reference to a bmoUnifiedShader object that emulates the Shader Unit.
     *  @param vectorThreads  Number of vector threads that can be on execution on the Vector Shader.
     *  @param vectorLength Number of elements in a shader vector.
     *  @param vectorALUWidth Number of ALUs in the vector ALU.  Each ALU executes instructions for a vector element.
     *  @param vectorALUConfig Configuration of the vector array ALUs.
     *  @param waitOnStall Flag that is used to determine if a vector shader instruction waits until the stall conditions
     *  are cleared at the decode stage or the instruction is discarded and a request is send to VectorShaderFetch to
     *  refetch (repeat) the instruction.
     *  @param explicitBlock Flag that defines if threads are blocked by an explicit trigger in the instruction stream or
     *  automatically on texture load.
     *  @param textClockRatio Texture Unit clock ratio with shader clock.
     *  @param textUnits Number of texture units attached to the shader.
     *  @param txReqCycle Texture requests sent to a texture unit per cycle.
     *  @param txReqUnit Groups of consecutive texture requests send to a texture unit.
     *  @param name Name of the mdu.
     *  @param shPrefix String used to prefix the fragment partition related mdu signals names to differentiate
     *  between multiple instances of the ShaderDecodeExecute class.
     *  @param sh2Prefix String used to prefix the vertex partition related mdu signals names to differentiate
     *  between multiple instances of the ShaderDecodeExecute class. 
     *  shader model is the vertex partition prefix.
     *  @param parent Pointer to the parent mdu.
     *  @return A new cmoShaderDecExeVector object.
     *
     */

    cmoShaderDecExeVector(bmoUnifiedShader &bmUnifiedShader, U32 vectorThreads, U32 vectorLength, U32 vectorALUWidth, char *vectorALUConfig,
        bool waitOnStall,  bool explicitBlock, U32 textClockRatio, U32 textUnits, U32 txReqCycle, U32 txReqUnit,
        char *name, char *shPrefix, char *sh2Prefix, cmoMduBase *parent);

    /**
     *
     *  Carries the simulation cycle a cycle of the Shader Decode/Execute mdu.
     *
     *  Simulates the behaviour of Shader Decode/Execute for a given cycle.
     *  Receives intructions that have finished their execution latency,
     *  executes them, updates dependencies and sends feedback to Shader Fetch.
     *  Receives instructions from Shader Fetch, checks dependencies, sends
     *  refetch commands, sends instructions to execute.
     *
     *  @param cycle  The cycle that is going to be simulated.
     *
     */

    void clock(U64 cycle);

    /**
     *
     *  Carries the simulation cycle a cycle of the Shader Decode/Execute mdu.
     *  Multi clock domain version.
     *
     *  Simulates the behaviour of Shader Decode/Execute for a given cycle.
     *  Receives intructions that have finished their execution latency,
     *  executes them, updates dependencies and sends feedback to Shader Fetch.
     *  Receives instructions from Shader Fetch, checks dependencies, sends
     *  refetch commands, sends instructions to execute.
     *
     *  @param domain Clock domain to update.
     *  @param cycle The cycle that is going to be simulated.
     *
     */

    void clock(U32 domain, U64 cycle);
       
};

} // namespace cg1gpu

#endif
