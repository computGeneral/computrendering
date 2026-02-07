/**************************************************************************
 *
 * Vector Shader Unit Fetch cmoMduBase.
 *  This file defines the cmoShaderFetchVector cmoMduBase class.
 *  This mdu implements the simulation of the Fetch stage in a GPU Shader unit.
 *
 */

#ifndef _VECTORSHADERFETCH_
#define _VECTORSHADERFETCH_


#include "support.h"
#include "GPUType.h"
#include "cmMultiClockMdu.h"
#include "GPUReg.h"
#include "cmtoolsQueue.h"
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
 *  This type stores the state of a vector shader thread.
 *
 */

struct VectorThreadState
{
    bool ready;                 //  Vector thread ready for execution.  
    bool blocked;               //  Vector thread has been blocked.  
    bool free;                  //  Vector thread is free (no data, no execution).  
    bool end;                   //  The vector thread has finished the program (waiting for output read).  
    bool zexported;             //  Vector thread has executed the z export instruction.  
    U32 PC;                  //  Thread PC (only for active threads).  
    U32 instructionCount;    //  Number of executed dynamic instructions for this vector thread.  
    U32 partition;           //  Shader partition/target to which the vector thread is assigned (vertex, fragment, triangle).  
    ShaderInput **shInput;      //  Pointer to the array of shader inputs assigned to the vector thread.  
    bool *traceElement;         //  Flag that stores if the execution of one element of the vector must be logged.  
};

/**
 *
 *  cmoShaderFetchVector implements the Fetch stage of a Vector Shader.
 *
 *  Inherits from cmoMduBase class.
 *
 */

class cmoShaderFetchVector : public cmoMduMultiClk
{

private:

    //  Defines the number of shader attributes (same for input and output).
    static const U32 SHADER_ATTRIBUTES = 48;

    //  Defines the number of shader partitions (per shader input type).
    static const U32 SHADER_PARTITIONS = 4;

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

    //  Shader State.
    bool transInProgress;       //  Shader Output transmission in progress.  
    U32 transCycles;         //  Remaining Shader Output transmission cycles.  
    
    ShaderState shState;        //  State of the Shader.  

    ShaderDecodeState decoderState;     //  State of the decode stage.  
    
    U32 activeOutputs[SHADER_PARTITIONS];    //  Number shader output attributes active for the current shade program per shader target.  
    U32 activeInputs[SHADER_PARTITIONS];     //  Number of shader input attributes active for the current shader program per shader target .  

    bool fetchedSIMD;           //  Stores if a SIMD instruction was fetched for the current thread.  
    bool fetchedScalar;         //  Stores if a scalar instruction was fetched for the current thread.  

    //  Shader structures.
    VectorThreadState *threadArray;         //  Array storing the state of the vector threads in the Shader.  
    tools::Queue<U32> freeThreadFIFO;    //  Stores pointers to the free entries in the vector thread array.  
    tools::Queue<U32> endThreadFIFO;     //  Stores pointers to threads finished in the vector thread array.  
    U32 inputThread;                     //  Stores the pointer to the current input thread in the vector thread array.  
    U32 nextInputElement;                //  Stores the index of the next element (shader input) in the input thread to load.  
    U32 outputThread;                    //  Stores the pointer to the current output thread in the vector thread array.  
    U32 nextOutputElement;               //  Stores the index of the next element (shader output) in the output thread to send back.  
    U32 outputResources;                 //  Number of resources to be liberated by the current output thread.  
    U32 currentActiveOutputs;            //  Number of active outputs for the current output thread.  
    U32 readyThreads;                    //  Counter that stores the number of runnable threads (ready state).  
    U32 blockedThreads;                  //  Counter that stores the number of blocked threads (blocked state).  
    U32 scanThread;                      //  Stores the pointer to the next vector thread to scan for ready threads.  

    U32 cyclesToNextFetch;               //  Counter that stores the number of cycles until the next fetch.  
    U32 cyclesBetweenFetches;            //  Stores the precomputed number of cycles to wait between two consecutive vector instruction fetches.  

    //  Shader Z Export state.
    bool zExportInProgress;                 //  Stores if z exports of any vector thread are currently being sent.  
    U32 currentZExportedThread;          //  Stores the index of the current vector thread in the thread array for which z exports are currently being sent.  
    U32 nextZExportedElement;            //  Stores the index of the next element in the current vector thread whose z export is currently being sent.  

    //  Resource counter.
    U32 freeResources;                   //  Number of free thread resources.  

    //  Shader instructions for a vector thread.
    ShaderExecInstruction ***vectorFetch;   //  Stores the fetched instructions for a vector thread (per element).  

    //  Shader registers.
    U32 startPC[MAX_SHADER_TARGETS];         //  Shader program start PC for each target/partition.  
    U32 resourceAlloc[MAX_SHADER_TARGETS];   //  Resources to allocate per thread for each target/partition.  
    bool activeOutAttr[SHADER_PARTITIONS][SHADER_ATTRIBUTES];   //  Defines the active output attributes fir each target/partition.  Active outputs are sent to the shader output consumer.  
    bool activeInAttr[SHADER_PARTITIONS][SHADER_ATTRIBUTES];    //  Defines the active input attributes for each target/partition.  Active inputs are received from the shader input producer.  

    //  Shader derived pseudoconstants.
    U32 maxThreadResources;  //  Maximum per thread resources of both partitions.  
 
    //  Shader Parameters.
    bmoUnifiedShader &bmUnifiedShader;     //  Reference to a bmoUnifiedShader object.  
    U32 numThreads;          //  Number of vector threads supported by the Shader  
    U32 numResources;        //  Number of vector thread resources (equivalent to temporal storage registers).  
    U32 vectorLength;        //  Elements (each element corresponds with a shader input) per vector.  
    U32 vectorALUWidth;      //  Number of ALUs in the vector ALU array.  
    char* vectorALUConfig;      //  Configuration of the vector array ALUs.  
    U32 fetchDelay;          //  Number of cycles until an instruction from a vector thread can be fetched again.  
    bool switchThreadOnBlock;   //  Flag that sets when the fetch thread is switched: after every fetch (false), or only after an instruction is blocked (true).  
    U32 textureUnits;        //  Number of texture units attached to the shader.  
    U32 inputCycle;          //  Shader inputs that can be received per cycle.  
    U32 outputCycle;         //  Shader outputs that can be sent per cycle.  
    U32 maxOutLatency;       //  Maximum latency of the output signal from the shader to the consumer.  
    bool multisampling;         //  Flag that stores if MSAA is enabled.  
    U32 msaaSamples;         //  Number of MSAA Z samples generated per fragment when MSAA is enabled.  

    //  Derived from the parameters.
    U32 instrCycle;              //  Instructions fetched/decoded/executed per cycle.  
    bool scalarALU;                 //  Architecture supports an extra scalar OP per cycle.  
    bool soaALU;                    //  Scalar architecture (scalar only vector ALU).  

    //  Statistics.
    gpuStatistics::Statistic *fetched;          //  Number of fetched instructions.  
    gpuStatistics::Statistic *reFetched;        //  Number of refetched instructions.  
    gpuStatistics::Statistic *inputs;           //  Number of shader inputs.  
    gpuStatistics::Statistic *outputs;          //  Number of shader outputs.  
    gpuStatistics::Statistic *inputInAttribs;   //  Number of input attributes active per input.  
    gpuStatistics::Statistic *inputOutAttribs;  //  Number of output attributes active per input.  
    gpuStatistics::Statistic *inputRegisters;   //  Number of temporal registers used per input.  
    gpuStatistics::Statistic *blocks;           //  Number of block command received.  
    gpuStatistics::Statistic *unblocks;         //  Number of unblocking command received.  
    gpuStatistics::Statistic *nThReady;         //  Number of ready threads.  
    gpuStatistics::Statistic *nThReadyAvg;      //  Number of ready threads (average).  
    gpuStatistics::NumericStatistic<U32> *nThReadyMax;  //  Number of ready threads (maximun).  
    gpuStatistics::NumericStatistic<U32> *nThReadyMin;  //  Number of ready threads (minimun).  
    gpuStatistics::Statistic *nThBlocked;       //  Number of blocked threads.  
    gpuStatistics::Statistic *nThFinished;      //  Number of finished threads.  
    gpuStatistics::Statistic *nThFree;          //  Number of free threads.  
    gpuStatistics::Statistic *nResources;       //  Number of used resources.  
    gpuStatistics::Statistic *emptyCycles;      //  Counts the cycles there is no work to be done.  
    gpuStatistics::Statistic *fetchCycles;      //  Counts the cycles instructions are fetched.  
    gpuStatistics::Statistic *noReadyCycles;    //  Counts the cycles there is no ready thread from which to fetch instructions.  

    //  Debug/Log.
    bool traceVertex;       //  Flag that enables/disables a trace log for the defined vertex.  
    U32 watchIndex;      //  Defines the vertex index to trace log.  
    bool traceFragment;     //  Flag that enables/disables a trace log for the defined fragment.  
    U32 watchFragmentX;  //  Defines the fragment x coordinate to trace log.  
    U32 watchFragmentY;  //  Defines the fragment y coordinate to trace log.  

    
    //  Private functions.

    /**
     *
     *  Reserves a vector thread for the new set of shader inputs and allocates all the resources required by the
     *  vector thread.
     *
     *  @param cycle Current simulation cycle.
     *  @param partition The partition (vertex/fragment) for which the vector thread is reserved.
     *  @param newThread Reference to a variable where to return the reserved vector thread pointer.
     *
     */

    void reserveVectorThread(U64 cycle, U32 partition, U32 &newThread);

    /**
     *
     *  Ends a vector thread and sets it as waiting for the shader outputs to be delivered to the consumer unit.
     *
     *  @param nThread The identifier of the vector thread that has ended.
     *
     */

    void endVectorThread(U32 threadID);

    /**
     *
     *  Sets a vector thread as free and deallocate the resources used by the thread.
     *
     *  @param cycle Current simulation cycle.
     *  @param threadID Identifer of the vector thread.
     *  @param resources Number of resources to liberate for the thread.
     *
     */

    void freeVectorThread(U64 cycle, U32 threadID, U32 resources);

    /**
     *
     *  Processes a Shader Command.
     *
     *  @param shComm The ShaderCommand to process.
     *  @param partition Identifies command for vertex or fragment partition in the unified shader model.
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
     *  Fetches a shader instruction for a vector thread element from the Shader Behavior Model.
     *
     *  @param cycle Current simulation cycle.
     *  @param threadID Identifier of the vector thread.
     *  @param element Index to the element in the vector thread.
     *  @param instruction Instruction to fetch (multiple instructions per cycle), added to the current thread PC.
     *  @param shEmuElemID Identifier in the Shader Behavior Model of the vector thread element.
     *  @param shExecInstr Reference to a pointer where to store the dynamic shader instruction fetched.
     *
     */

    void fetchElementInstruction(U64 cycle, U32 threadID, U32 element, U32 shEmuElemID, U32 instruction,
        ShaderExecInstruction *& shExecInstr);

    /**
     *
     *  Fetches the instructions for all the elements of a vector thread from the Shader Behavior Model.
     *
     *  @param cycle Current simulation cycle.
     *  @param threadID Identifier of the vector thread.
     *
     */

    void fetchVectorInstruction(U64 cycle, U32 threadID);

    /**
     *
     *  Sends vector thread instructions to the shader decode stage.
     *
     *  @param cycle Current simulation cycle.
     *  @param threadID Identifier of the vector thread.
     *
     */

    void sendVectorInstruction(U64 cycle, U32 threadID);

    /**
     *
     *  Receive new shader inputs from a producer unit.
     *
     *  @param cycle Current simulation cycle.
     *
     */
     
    void receiveInputs(U64 cycle);


    /**
     *
     *  Recieve updates from the Command Processor.
     *
     *  @param cycle Current simulation cycle.
     *
     */
     
    void updatesFromCommandProcessor(U64 cycle);

    /*
     *
     *  Receive state and updates from the decode stage.
     *
     *  @param cycle Current simulation cycle.
     *
     */
    
    void updatesFromDecodeStage(U64 cycle);

    /*
     *
     *  Update fetch and shader input/output management statistics
     *
     */
     
    void updateFetchStatistics();
     
    /**
     *
     *  Output transmission stage.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void processOutputs(U64 cycle);

    /**
     *
     *  Sends the shader output corresponding to a element of a vector thread thread to the shader consumer unit.
     *
     *  @param cycle Current simulation cycle.
     *  @param outputThread The vector thread for which to send the output to the consumer unit.
     *  @param element Index to the element in the vector thread to send to the consumer unit.
     *
     */

    void sendOutput(U64 cycle, U32 outputThread, U32 element);

    /**
     *
     *  Simulates the fetch stage: updates fetch stage state and fetchs and issues instructions to the
     *  decode stage.
     *
     *  @param cycle Current simulation cycle.
     *
     */
     
    void fetchStage(U64 cycle);
    
    /**
     *
     *  Computes and sends back preassure state to the consumer unit.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void sendBackPreassure(U64 cycle);

    /*
     *  Simulate and update the state of the GPU domain section of the vector shader fetch mdu.
     *
     *  @param cycle Current simulation cycle.
     *
     */
     
    void updateGPUDomain(U64 cycle);

    /*
     *  Simulate and update the state of the shader domain section of the vector shader fetch mdu.
     *
     *  @param cycle Current simulation cycle.
     *
     */
     
    void updateShaderDomain(U64 cycle);     

    /**
     *
     *  Sends shader z exports of vector threads to the Shader Work Distributor unit.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void sendZExports(U64 cycle);

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
     *  Constructor for cmoShaderFetchVector.
     *
     *  Creates a new cmoShaderFetchVector object ready for start the simulation.
     *
     *  @param bmShader Reference to a bmoUnifiedShader object that will be used to emulate the Shader Unit.
     *  @param nThreads Number of vector threads in the Vector Shader Unit.
     *  @param resources Number of thread resources available (equivalent to registers).
     *  @param vectorLength Elements per vector (correspond to shader inputs).
     *  @param vectorALUWidth Number of ALUs in the vector ALU array.
     *  @param vectorALUConfig Configuration of the vector array ALUs.
     *  @param fetchDelay Cycles until a thread can be fetched again.
     *  @param switchThreadOnBlock Flag that sets if the fetch thread is switched after each instruction (false) or only when the thread becomes blocked.
     *  @param texUnits Number of texture units attached to the shader.
     *  @param inCycle Shader inputs received per cycle.
     *  @param outCycle Shader outputs sent per cycle.
     *  @param outLatency Maximum latency of the output signal to the consumer.
     *  @param microTrisAsFrag Process microtriangle fragment shader inputs and export z values (when MicroPolygon Rasterizer enabled only).
     *  @param name Name of the ShaderFetch mdu.
     *  @param shPrefix Primary shader prefix.   Used as fragment shader prefix (signals from the Command Processor).
     *  @param sh2Prefix Secondary shader prefix.  Used as vertex shader prefix (signasl from the Command Processor).
     *  @param parent Pointer to the parent mdu for this mdu.
     *
     *  @return A new cmoShaderFetchVector object initializated.
     *
     */

    cmoShaderFetchVector(bmoUnifiedShader &bmShader, U32 nThreads, U32 resources, U32 vectorLength,
        U32 vectorALUWidth, char *vectorALUConfig, U32 fetchDelay, bool switchThreadOnBlock,
        U32 texUnits, U32 inCycle, U32 outCycle, U32 outLatency, bool microTrisAsFrag,
        char *name, char *shPrefix = 0, char *sh2Prefix = 0, cmoMduBase* parent = 0);

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
     *  Multi clock domain update rutine.
     *
     *  Updates the state of one of the clock domains implemented in the Vector Shader Fetch mdu.
     *
     *  @param domain Clock domain to update.
     *  @param cycle Cycle to simulate.
     *
     */

    void clock(U32 domain, U64 cycle);

    /**
     *
     *  Returns a single line string with state and debug information about the
     *  Shader Fetch mdu.
     *
     *  @param stateString Reference to a string where to store the state information.
     *
     */

    void getState(string &stateString);

    /**
     *
     *  Detects stall conditions in the Vector Shader Fetch mdu.
     *
     *  @param cycle Current simulation cycle.
     *  @param active Reference to a boolean variable where to return if the stall detection logic is enabled/implemented.
     *  @param stalled Reference to a boolean variable where to return if the Vector Shader Fetch has been detected to be stalled.
     *
     */
     
    void detectStall(U64 cycle, bool &active, bool &stalled);

    /**
     *
     *  Writes into a string a report about the stall condition of the mdu.
     *
     *  @param cycle Current simulation cycle.
     *  @param stallReport Reference to a string where to store the stall state report for the mdu.
     *
     */
     
    void stallReport(U64 cycle, string &stallReport);

    /**
     *
     *  Get the list of debug commands supported by the Vector Shader Fetch mdu.
     *
     *  @param commandList Reference to a string variable where to store the list debug commands supported
     *  by Vector Shader Fetch.
     *
     */
     
    void getCommandList(std::string &commandList);

    /**
     *
     *  Executes a debug command on the Vector Shader Fetch mdu.
     *
     *  @param commandStream A reference to a stringstream variable from where to read
     *  the debug command and arguments.
     *
     */    
     
    void execCommand(stringstream &commandStream);

    /**
     *  Displays the disassembled current shader program in the defined partition.
     *
     *  @param partition The partition for which to display the current shader program.
     *
     */
     
    void listProgram(U32 partition);

};

} // namespace cg1gpu

#endif
