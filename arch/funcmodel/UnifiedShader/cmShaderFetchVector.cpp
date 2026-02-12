/**************************************************************************
 *
 * VectorShader Unit Fetch cmoMduBase.
 *
 */

/**
 *
 *  @file cmoShaderFetchVector.cpp
 *
 *  This file implements the cmoShaderFetchVector cmoMduBase Class.
 *  This mdu implements the simulation of the Fetch stage of a GPU shader unit.
 *
 */

#include "cmShaderFetchVector.h"
#include "cmShaderStateInfo.h"
#include "cmConsumerStateInfo.h"
#include "cmShaderDecodeStateInfo.h"
#include "cmStreamer.h"
#include "cmShaderDecodeExecuteVector.h"
#include "cmInstructionFetchVector.h"
#include "ShaderInstr.h"
#include <cmath>
#include <cstdio>
#include <sstream>

using namespace arch;

using arch::tools::Queue;

//  Constructor for the cmoShaderFetchVector BOX class.

cmoShaderFetchVector::cmoShaderFetchVector(bmoUnifiedShader &bmShader, U32 nThreads, U32 resources,
    U32 vectorLength_, U32 vectorALUWidth_, char *vecALUConf, U32 fDelay, bool switchOnBlock,
    U32 texUnits, U32 inCycle, U32 outCycle, U32 outputLatency, bool microTrisAsFrag,
    char *name, char *shPrefix, char *sh2Prefix, cmoMduBase* parent):

    //  Pre-initializations.
    cmoMduMultiClk(name, parent), bmUnifiedShader(bmShader), numThreads(nThreads), numResources(resources),
    vectorLength(vectorLength_), vectorALUWidth(vectorALUWidth_), vectorALUConfig(vecALUConf),
    fetchDelay(fDelay), switchThreadOnBlock(switchOnBlock), textureUnits(texUnits), inputCycle(inCycle), 
    outputCycle(outCycle), maxOutLatency(outputLatency) 

{
    U32 i;
    U32 j;
    DynamicObject *defValue[1];
    char fullName[64];
    char postfix[32];

    CG_INFO("Using Vector Shader (Fetch)");

    //  Derive parameters from the vector ALU configuration.
    string aluConf(vectorALUConfig);
    
    if (aluConf.compare("simd4") == 0)
    {
        instrCycle = 1;
        scalarALU = false;
        soaALU = false;
    }
    else if (aluConf.compare("simd4+scalar") == 0)
    {
        instrCycle = 2;
        scalarALU = true;
        soaALU = false;
    }
    else if (aluConf.compare("scalar") == 0)
    {
        instrCycle = 1;
        scalarALU = false;
        soaALU = true;
    }
    else
    {
        CG_ASSERT("Unsupported vector ALU configuration.");
    }


    //  Check parameters.
    GPU_ASSERT(
        if (vectorLength == 0)
            CG_ASSERT("Vector length must be at least one.");
        if (numResources < numThreads)
            CG_ASSERT("At least one resource per vector thread required.");
        if (vectorALUWidth == 0)
            CG_ASSERT("Vector width (ALUs in the vector ALU array) must be at least one.");
        if (instrCycle == 0)
            CG_ASSERT("At least one instruction from a shader thread must be fetched per cycle.");
        if (inputCycle == 0)
            CG_ASSERT("At least one shader input must be received per cycle.");
        if (GPU_MOD(vectorLength, inputCycle) != 0)
            CG_ASSERT("The vector length must be a multiple of the inputs per cycle.");
        if (outputCycle == 0)
            CG_ASSERT("At least one shader output must be sent per cycle.");
        if (GPU_MOD(vectorLength, outputCycle) != 0)
            CG_ASSERT("The vector length must be a multiple of the outputs per cycle.");
        if (maxOutLatency == 0)
            CG_ASSERT("Output signal latency must be at least 1.");
        if (shPrefix == NULL)
            CG_ASSERT("Shader prefix string required (primeray).");
        if (sh2Prefix == NULL)
            CG_ASSERT("Shader prefix string required (secondary).");
        if (scalarALU && (instrCycle != 2))
            CG_ASSERT("SIMDx4 + scalar ALU configuration requires a fetch rate of two instructions per cycle.");
    )

    //  Create the full name and postfix for the statistics.
    sprintf(fullName, "%s::%s", shPrefix, name);
    sprintf(postfix, "ShF-%s", shPrefix);

//printf("ShFetch %s -> %p \n", fullName, this);

    //  Create statistics.
    fetched = &getSM().getNumericStatistic("FetchedInstr", U32(0), fullName, postfix);
    reFetched = &getSM().getNumericStatistic("ReFetchedInstr", U32(0), fullName, postfix);
    inputs = &getSM().getNumericStatistic("Inputs", U32(0), fullName, postfix);
    outputs = &getSM().getNumericStatistic("Outputs", U32(0), fullName, postfix);
    inputInAttribs = &getSM().getNumericStatistic("InputActiveInputAttributes", U32(0), fullName, postfix);
    inputOutAttribs = &getSM().getNumericStatistic("InputActiveOutputAttributes", U32(0), fullName, postfix);
    inputRegisters = &getSM().getNumericStatistic("InputRegisters", U32(0), fullName, postfix);
    blocks = &getSM().getNumericStatistic("Blocks", U32(0), fullName, postfix);
    unblocks = &getSM().getNumericStatistic("Unblocks", U32(0), fullName, postfix);
    nThReady = &getSM().getNumericStatistic("ReadyThreads", U32(0), fullName, postfix);
    nThReadyAvg = &getSM().getNumericStatistic("ReadyThreadsAvg", U32(0), fullName, postfix);
    nThReadyMax = &getSM().getNumericStatistic("ReadyThreadsMax", U32(0), fullName, postfix);
    nThReadyMin = &getSM().getNumericStatistic("ReadyThreadsMin", U32(0), fullName, postfix);
    nThBlocked = &getSM().getNumericStatistic("BlockedThreads", U32(0), fullName, postfix);
    nThFinished = &getSM().getNumericStatistic("FinishedThreads", U32(0), fullName, postfix);
    nThFree = &getSM().getNumericStatistic("FreeThreads", U32(0), fullName, postfix);
    nResources = &getSM().getNumericStatistic("UsedResources", U32(0), fullName, postfix);
    emptyCycles = &getSM().getNumericStatistic("EmptyCycles", U32(0), fullName, postfix);
    fetchCycles = &getSM().getNumericStatistic("FetchCycles", U32(0), fullName, postfix);
    noReadyCycles = &getSM().getNumericStatistic("NoReadyCycles", U32(0), fullName, postfix);

    //  Create cmoMduBase Signals.

    //  Signal from the Command Processor.  Commands to the Shader.
    commProcSignal[FRAGMENT_PARTITION] = newInputSignal("CommShaderCommand", 1, 1, shPrefix);
    commProcSignal[VERTEX_PARTITION] = newInputSignal("CommShaderCommand", 1, 1, sh2Prefix);

    //  Signal from the Consumer unit.  Inputs and commands to the Shader.
    commandSignal = newInputSignal("ShaderCommand", inputCycle, 1, shPrefix);

    //  Signal from the Decoder/Execute cmoMduBase.  New PC and END signal.
    newPCSignal = newInputSignal("ShaderControl",  1 + (MAX_EXEC_BW + 1) * instrCycle, 1, shPrefix);

    //  Signal from the Decoder/Execute cmoMduBase.  Decoder state.
    decodeStateSignal = newInputSignal("ShaderDecodeState", 1, 1, shPrefix);

    //  Signal to the cmoStreamController.  Shader state.
    readySignal = newOutputSignal("ShaderState", 1, 1, shPrefix);

    //  Build initial signal state.
    defValue[0] = new ShaderStateInfo(SH_READY);

    //  Set default state of ShaderReady signal.
    readySignal->setData(defValue);

    //  Signal to the Decoder/Execute cmoMduBase.
    instructionSignal = newOutputSignal("cgoShaderInstr", instrCycle, 1, shPrefix);

    //  Signal from the Shader to a consumer.  Shader output.
    outputSignal = newOutputSignal("ShaderOutput", outputCycle, maxOutLatency, shPrefix);

    //  Signal from the consumer to the Shader.  State of the consumer.
    consumerSignal = newInputSignal("ConsumerState", 1, 1, shPrefix);

    //  Create and initialize the vector thread array.
    threadArray = new VectorThreadState[numThreads];

    //  Check allocation.
    CG_ASSERT_COND(!(threadArray == NULL), "Error allocating vector thread array.");
    //  Reset state of the vector threads.  Allocate shader input array.
    for (i = 0; i < numThreads; i++)
    {
        threadArray[i].ready = false;
        threadArray[i].free = true;
        threadArray[i].end = false;
        threadArray[i].zexported = false;
        threadArray[i].PC = 0;
        threadArray[i].instructionCount = 0;
        threadArray[i].shInput = new ShaderInput*[vectorLength];
        threadArray[i].traceElement = new bool[vectorLength];
        
        for(U32 e = 0; e < vectorLength; e++)
            threadArray[i].traceElement[e] = false;

        //  Check allocation.
        CG_ASSERT_COND(!(threadArray[i].shInput == NULL), "Error allocating shader input array per vector thread.");
    }

    //  Reset pointer to the next thread to check if it is ready.
    scanThread = 0;

    string queueName;

    //  Set queue names.
    queueName.clear();
    queueName.append("FreeThreadFIFO");
    queueName.append(postfix);
    freeThreadFIFO.setName(queueName);

    queueName.clear();
    queueName.append("EndThreadFIFO");
    queueName.append(postfix);
    endThreadFIFO.setName(queueName);

    //  Reset next element input/output pointers.
    nextInputElement = vectorLength;
    nextOutputElement = vectorLength;

    //  Reset ready/blocked vector threads counters.
    readyThreads = 0;
    blockedThreads = 0;

    //  Initialize the FIFO for storing pointers to the free entries in the vector thread array.
    freeThreadFIFO.resize(numThreads);

    //  Insert all vector thread array entries in the free FIFO.
    for(i = 0; i < numThreads; i++)
        freeThreadFIFO.add(i);

    //  Initialize the FIFO for storing pointers to the threads that have finished in the vector thread array.
    endThreadFIFO.resize(numThreads);

    //  Set initial shader state to reset.
    shState = SH_EMPTY;

    //  Reset active input attributes per shader target/partition.
    for(i = 0; i < SHADER_PARTITIONS; i++)
    {
        //  Reset the active input attributes as not active.
        for(j = 0; j < SHADER_ATTRIBUTES; j++)
            activeInAttr[i][j] = false;

        //  Reset the number of active inputs attributes for the shader target.
        activeInputs[i] = 0;
    }

    //  Reset active output attributes per shader target/partition.
    for(U32 p = 0; p < SHADER_PARTITIONS; p++)
    {
        //  Reset active output attributes.
        for(U32 o = 0; o < SHADER_ATTRIBUTES; o++)
            activeOutAttr[p][o] = false;       

        //  Reset the number of active output attributes (for vertex target/partition).
        activeOutputs[p] = 0;
    }
    
    //  Fragment and shader partitions have output attributes set by default.
    activeOutAttr[FRAGMENT_PARTITION][1] = true;
    activeOutputs[FRAGMENT_PARTITION] = 1;
    activeOutAttr[TRIANGLE_PARTITION][0] = true;
    activeOutAttr[TRIANGLE_PARTITION][1] = true;
    activeOutAttr[TRIANGLE_PARTITION][2] = true;
    activeOutAttr[TRIANGLE_PARTITION][3] = true;
    activeOutputs[TRIANGLE_PARTITION] = 4;

    //  Reset default start PCs.
    startPC[VERTEX_TARGET] = 0;
    startPC[FRAGMENT_TARGET] = 0;
    startPC[TRIANGLE_TARGET] = TRIANGLE_SETUP_PROGRAM_PC;
    startPC[MICROTRIANGLE_TARGET] = 0;

    //  Reset the number of resources to allocate per thread for each vertex target/partition.
    resourceAlloc[VERTEX_TARGET]   = 1;
    resourceAlloc[FRAGMENT_TARGET] = 1;
    resourceAlloc[TRIANGLE_TARGET] = 2;
    resourceAlloc[MICROTRIANGLE_TARGET] = 2;

    //  For triangle shader target/partition force active input attributes to 3.
    activeInputs[TRIANGLE_PARTITION] = 3;

    //  Reset the number of free resources.
    freeResources = numResources;

    //  Reset default maximum resource allocation per thread.
    maxThreadResources = 3;

    //  Allocate buffer for the shader instructions fetched for a vector thread.
    vectorFetch = new ShaderExecInstruction**[instrCycle];

    //  Check allocation.
    CG_ASSERT_COND(!(vectorFetch == NULL), "Error allocating buffer for instructions fetched for a vector thread in a cycle.");
    //  Reset the counter of cycles until the next fetch.
    cyclesToNextFetch = 0;

    //  Clear output transmission in progress flag.
    transInProgress = false;
    
    //  Clear z exports in progress flag.
    zExportInProgress = false;

    //  Reset MSAA registers
    multisampling = false;
    msaaSamples = 4;

    //  Set the number of cycles the wait between fetching two vector instructions.
    //  The current implementation requires a wait of at least 2 cycles to hide the
    //  latency of fetch->decode->fetch control traffic.
    cyclesBetweenFetches = GPU_MAX(U32(2), U32(ceil(F32(vectorLength) / F32(vectorALUWidth))));

    //  Debug/log.
    traceVertex = false;
    watchIndex = 0;
    traceFragment = false;
    watchFragmentX = 0;
    watchFragmentY = 0;
}

//  Clock instruction for ShaderFetch cmoMduBase.  Drives the time of the simulation.
void cmoShaderFetchVector::clock(U64 cycle)
{
    GPU_DEBUG_BOX( printf("%s >> Clock %lld.\n", getName(), cycle); )

    //  Receive shader inputs from producer unit.
    receiveInputs(cycle);
    
    //  Receive updates from the Command Processor.
    updatesFromCommandProcessor(cycle);
    
    //  Receive state and updates from the decode stage.
    updatesFromDecodeStage(cycle);
    
    //  Process outputs pending or being transmited to the consumer.
    processOutputs(cycle);

    //  Fetch instructions.
    fetchStage(cycle);

    //  Send backpreasure state to consumer.
    sendBackPreassure(cycle);

    //  Update statistics.
    updateFetchStatistics();
}

//  Clock instruction for ShaderFetch cmoMduBase.  Drives the time of the simulation.
void cmoShaderFetchVector::clock(U32 domain, U64 cycle)
{
    string clockName;
    switch(domain)
    {
        case GPU_CLOCK_DOMAIN :
            clockName = "GPU_CLOCK";
            break;
        case SHADER_CLOCK_DOMAIN :
            clockName = "SHADER_CLOCK";
            break;
        default:
            CG_ASSERT("Unsupported clock domain.");
            break;
    }
    
    GPU_DEBUG_BOX
    (
        printf("%s >> Domain %s Clock %lld.\n", getName(), clockName.c_str(), cycle);
    )
    
    //  Update domain state.
    switch(domain)
    {
        case GPU_CLOCK_DOMAIN :
            updateGPUDomain(cycle);
            break;
        case SHADER_CLOCK_DOMAIN :
            updateShaderDomain(cycle);
            break;
        default:
            CG_ASSERT("Unsupported clock domain.");
            break;
    }
}


//  Simulate and update the state of the GPU domain section of the vector shader fetch mdu.
void cmoShaderFetchVector::updateGPUDomain(U64 cycle)
{
    //  Receive shader inputs from producer unit.
    receiveInputs(cycle);
    
    //  Receive updates from the Command Processor.
    updatesFromCommandProcessor(cycle);

    //  Process outputs pending or being transmited to the consumer.
    processOutputs(cycle);

    //  Send backpreasure state to consumer.
    sendBackPreassure(cycle);
}

//  Simulate and update the state of the shader domain section of the vector shader fetch mdu.
void cmoShaderFetchVector::updateShaderDomain(U64 cycle)
{
    //  Receive state and updates from the decode stage.
    updatesFromDecodeStage(cycle);

    //  Fetch instructions.
    fetchStage(cycle);

    //  Update statistics.
    updateFetchStatistics();
}


//  Receive new shader inputs from a producer unit.
void cmoShaderFetchVector::receiveInputs(U64 cycle)
{
    ShaderInput *shInput;

    //  Receive inputs and commands from the consumer unit attached to the shader.
    while (commandSignal->read(cycle, (DynamicObject *&) shInput))
        processShaderInput(cycle, shInput);
}

//  Receive updates from the Command Processor
void cmoShaderFetchVector::updatesFromCommandProcessor(U64 cycle)
{
    ShaderCommand *command;

    //  Receive commands from the Command Processor (vertex target/partition).
    if (commProcSignal[VERTEX_PARTITION]->read(cycle, (DynamicObject *&) command))
        processShaderCommand(command, VERTEX_PARTITION);

    //  Receive commands from the Command Processor (fragment target/partition).
    if (commProcSignal[FRAGMENT_PARTITION]->read(cycle, (DynamicObject *&) command))
        processShaderCommand(command, FRAGMENT_PARTITION);
}

//  Receive state and updates from the decode stage.
void cmoShaderFetchVector::updatesFromDecodeStage(U64 cycle)
{
    ShaderDecodeCommand *decodeCommand;
    ShaderDecodeStateInfo *shDecStateInfo;

    //  Receive feedback from decode stage.
    while (newPCSignal->read(cycle, (DynamicObject *&) decodeCommand))
        processDecodeCommand(decodeCommand);

    //  Receive state from the decode stage.
    if (decodeStateSignal->read(cycle, (DynamicObject *&) shDecStateInfo))
    {
        //  Store shader decode state.
        decoderState = shDecStateInfo->getState();

        //  Delete received decoder state.
        delete shDecStateInfo;
    }
    else
    {
        //  No decoder state?  Where did those electrons went?
        CG_ASSERT("No decoder state signal.");
    }
}

//  Update fetch statistics.
void cmoShaderFetchVector::updateFetchStatistics()
{
    //  Update statistics.
    U32 freeVectorThreads = freeThreadFIFO.items();
    nThReadyAvg->incavg(readyThreads);
    nThReadyMax->maximum(readyThreads);
    nThReadyMin->minimum(readyThreads);
    nThReady->inc(readyThreads);
    nThBlocked->inc(blockedThreads);
    nThFinished->inc(endThreadFIFO.items());
    nThFree->inc(freeVectorThreads);
    nResources->inc(numResources - freeResources);
}

//  Compute and send the back preassure state to the consumer unit.
void cmoShaderFetchVector::sendBackPreassure(U64 cycle)
{
    //  Compute minimum requeriments for back preassure.
    U32 requiredThreads;
    U32 requiredResources;
    U32 freeVectorThreads;

    requiredThreads = 2 * GPU_MAX(U32(ceil(F32(inputCycle) / F32(vectorLength))), U32(1));
    requiredResources = 2 * maxThreadResources;
    freeVectorThreads = freeThreadFIFO.items();

    GPU_DEBUG_BOX(
        printf("%s => requiredThreads = %d | freeVectorThreads = %d | requiredResources = %d | freeResources = %d\n",
            getName(), requiredThreads, freeVectorThreads, requiredResources, freeResources);
    )

    //  Update Shader state signal.
    if ((freeVectorThreads < requiredThreads) || (freeResources < requiredResources))
    {
        GPU_DEBUG_BOX(
            printf("%s >> Sending Busy.\n", getName());
        )

        shState = SH_BUSY;
    }
    else if (freeThreadFIFO.empty())
    {
        GPU_DEBUG_BOX(
            printf("%s >> Sending Empty.\n", getName());
        )

        shState = SH_EMPTY;
    }
    else
    {
        GPU_DEBUG_BOX(
            printf("%s >> Sending Ready.\n", getName());
        )

        shState = SH_READY;
    }

    readySignal->write(cycle, new ShaderStateInfo(shState));
}

//  Fetch stage.  Update fetch stage state and fetch new instructions.
void cmoShaderFetchVector::fetchStage(U64 cycle)
{
    //  Update the counter of cycles to wait until the next vector instruction fetch.
    if (cyclesToNextFetch > 0)
    {
        cyclesToNextFetch--;
    }

    GPU_DEBUG_BOX(
        cout << getName() << " => Cycles to next fetch = " << cyclesToNextFetch << endl;
    )

    //  Update statistics.
    if (freeThreadFIFO.items() == numThreads)
        emptyCycles->inc();
    else if (cyclesToNextFetch > 0)
        fetchCycles->inc();
    else if (readyThreads == 0)
        noReadyCycles->inc();

    //  Check if shader decode stage can receive new instructions.
    if (decoderState == SHDEC_READY)
    {
        GPU_DEBUG_BOX(
            cout << getName() << " => Decoder is ready.  Cycles to next fetch = " << cyclesToNextFetch << endl;
        )

        //  Check if there are vector threads runnable.
        if ((cyclesToNextFetch == 0) && (readyThreads > 0))
        {
            U32 fetchVectorThread;

            //  Check if we have to search for the next vector thread that is runnable.
            if (!threadArray[scanThread].ready || !switchThreadOnBlock)
            {
                //  Update scan thread to point to the next thread.
                scanThread = GPU_MOD(scanThread + 1, numThreads);

                //  Scan for the next ready vector thread in the vector array.
                for(U32 visitedThreads = 0; !threadArray[scanThread].ready && (visitedThreads < numThreads); visitedThreads++)
                    scanThread = GPU_MOD(scanThread + 1, numThreads);
            }

            //  Set fetch vector thread.
            fetchVectorThread = scanThread;

            //  Check if the thread is runnable.
            CG_ASSERT_COND(!(!threadArray[fetchVectorThread].ready), "No runnable (ready) vector thread when there should be one.");
            GPU_DEBUG_BOX(
                printf("%s => Fetching from vector thread %d\n", getName(), fetchVectorThread);
            )

            //  Fetch instructions from the next vector thead.
            fetchVectorInstruction(cycle, fetchVectorThread);

            //  Send instructions to decode.
            sendVectorInstruction(cycle, fetchVectorThread);

            //  Set counter of cycles to wait until the next vector instruction fetch.
            cyclesToNextFetch = cyclesBetweenFetches;
        }
    }
}

//  Frees a vector thread and deallocated the resources used by the thread.
void cmoShaderFetchVector::freeVectorThread(U64 cycle, U32 threadID, U32 elementResources)
{
    U32 partition;

    //  Check that the pointer to the entry in the vector thread array to be liberated is correct.
    GPU_ASSERT
    (
        if (threadID > numThreads)
            CG_ASSERT("Thread identifier out of range.");
    )

    partition = threadArray[threadID].partition;

    //  Check overflows.
    GPU_ASSERT(
        //  Check if the free thread FIFO is already full.
        if (freeThreadFIFO.full())
        {
            CG_ASSERT("Free thread FIFO is already full.");
        }

        //  Check if all the resources are already liberated.
        if (freeResources >= numResources)
        {
            printf("cmoShaderFetchVector (%lld) => ThreadID = %d | Partition = %s | ElemResources = %d | FreeResources = %d | NumResources = %d\n",
                cycle, threadID, (partition == VERTEX_PARTITION) ? "VERTEX" : "FRAGMENT", 
                elementResources, freeResources, numResources);
            CG_ASSERT("All resources are already liberated.");
        }
    )

    //  Check that the thread is in end state.
    GPU_ASSERT(
        if (!threadArray[threadID].end || threadArray[threadID].ready || threadArray[threadID].free)
        {
            CG_ASSERT("Vector thread to free isn't in end state, or is in ready or free state.");
        }
    )

    //  Clear flags of the freed vector thread array entry.  Mark the thread as free.
    threadArray[threadID].free = true;
    threadArray[threadID].end = false;
    threadArray[threadID].zexported = false;

//printf("cmoShaderFetchVector (%lld) => Freeing resources.  ThreadID = %d | Partition = %s | ElemResources = %d | FreeResources = %d | NumResources = %d\n",
//    cycle, threadID, (partition == VERTEX_PARTITION) ? "VERTEX" : "FRAGMENT", 
//    (partition != VERTEX_PARTITION) ? GPU_MAX(activeInputs[partition], resourceAlloc[partition]) : GPU_MAX(activeInputs[partition], GPU_MAX(resourceAlloc[partition], activeOutputs[partition])),
//    freeResources, numResources);
        
    //  Liberate the resources allocated to the vector thread.
    freeResources += elementResources;

    //  Add freed entry to the free thread FIFO.
    freeThreadFIFO.add(threadID);

    GPU_ASSERT(
        //  Check overflow in the number of free resources.
        if (freeResources > numResources)
        {
            printf("cmoShaderFetchVector (%lld) => ThreadID = %d | Partition = %s | ElemResources = %d | FreeResources = %d | NumResources = %d\n",
                cycle, threadID, (partition == VERTEX_PARTITION) ? "VERTEX" : "FRAGMENT", 
                (partition != VERTEX_PARTITION) ? GPU_MAX(activeInputs[partition], resourceAlloc[partition]) : GPU_MAX(activeInputs[partition], GPU_MAX(resourceAlloc[partition], activeOutputs[partition])),
                freeResources, numResources);
            CG_ASSERT("Overflow in free resources.");
        }
    )
}


//  Process a Shader Command.
void cmoShaderFetchVector::processShaderCommand(ShaderCommand *command, U32 partition)
{
    U32 firstAddr, lastAddr;
    Vec4FP32 *values;

    switch(command->getCommandType())
    {
        case LOAD_PROGRAM:

            //  New Shader Program must be loaded.

            GPU_DEBUG_BOX(
                printf("%s >> LOAD_PROGRAM Size %d PC %06x\n",
                    getName(), command->getProgramCodeSize(), command->getLoadPC());
            )

            //  Load program into the Shader behaviorModel.
            //bmUnifiedShader.loadShaderProgram(command->getProgramCode(), command->getLoadPC(), command->getProgramCodeSize());
            bmUnifiedShader.loadShaderProgramLight(command->getProgramCode(), command->getLoadPC(), command->getProgramCodeSize());

            break;

        case PARAM_WRITE:

            //  New Shader Parameters must be loaded.

            //  !!!!!! ONE BY ONE WRITE.  I SHOULD CHANGE SHADEREMULATOR INTERFACE TO SUPPORT THIS WITH A SINGLE CALL.

            //  Add constant partition offset as required.
            firstAddr = command->getParamFirstAddress() + partition * UNIFIED_CONSTANT_NUM_REGS;
            lastAddr = command->getParamFirstAddress() + command->getNumValues() + partition * UNIFIED_CONSTANT_NUM_REGS;

            values = command->getValues();

            //  Write a single value in the constant/parameter memory.  Thread field is unused for constant memory.
            bmUnifiedShader.loadShaderState(0, PARAM, values, firstAddr, command->getNumValues());

            GPU_DEBUG_BOX(
                for (U32 i = firstAddr; i < lastAddr; i ++)
                    printf("%s >> PARAM_WRITE %d = {%f, %f, %f, %f}\n",
                        getName(), i, values[i - firstAddr][0], values[i - firstAddr][1],
                        values[i - firstAddr][2],values[i - firstAddr][3]);
            )

            break;

        case SET_INIT_PC:

            //  New shader program initial PC.

            GPU_DEBUG_BOX(
                printf("%s => SET_INIT_PC[", getName());
                switch(command->getShaderTarget())
                {
                    case VERTEX_TARGET:
                        printf("VERTEX_TARGET");
                        break;
                    case FRAGMENT_TARGET:
                        printf("FRAGMENT_TARGET");
                        break;
                    case TRIANGLE_TARGET:
                        printf("TRIANGLE_TARGET");
                        break;
                    case MICROTRIANGLE_TARGET:
                        printf("MICROTRIANGLE_TARGET");
                        break;
                    default:
                        CG_ASSERT("Undefined shader target.");
                        break;
                }
                printf("] = %06x.\n", command->getInitPC());
            )

            //  Set initial PC.
            startPC[command->getShaderTarget()] = command->getInitPC();

            break;

        case SET_THREAD_RES:

            //  New per shader thread resource usage.

            //  Set per thread resource usage.
            resourceAlloc[command->getShaderTarget()] = command->getThreadResources();

            //  Recalculate max thread resources.
            maxThreadResources = computeMaxThreadResources();

            GPU_DEBUG_BOX(
                char buffer[256];
                switch(command->getShaderTarget())
                {
                    case VERTEX_TARGET:
                        sprintf(buffer, "VERTEX_TARGET");
                        break;
                    case FRAGMENT_TARGET:
                        sprintf(buffer, "FRAGMENT_TARGET");
                        break;
                    case TRIANGLE_TARGET:
                        sprintf(buffer, "TRIANGLE_TARGET");
                        break;
                    case MICROTRIANGLE_TARGET:
                        sprintf(buffer, "MICROTRIANGLE_TARGET");
                        break;
                    default:
                        CG_ASSERT("Undefined shader target.");
                        break;
                }
                printf("%s => resourceAlloc[%s] = %d, maxResources =  %d\n",
                    getName(), buffer, resourceAlloc[command->getShaderTarget()], maxThreadResources);
            )

            break;

        case SET_IN_ATTR:

            //  Set which shader input registers are enabled for the current shader program.

            //  Check if the shader output register identifier.
            CG_ASSERT_COND(!(command->getAttribute() >= MAX_VERTEX_ATTRIBUTES), "Out of range shader input register identifier.");
            GPU_DEBUG_BOX(
                printf("%s => Setting input attribute %d as %s for partition %s\n", getName(), command->getAttribute(),
                    command->isAttributeActive()?"ACTIVE":"INACTIVE",
                    (partition == VERTEX_PARTITION)?"VERTEX":((partition == FRAGMENT_PARTITION)?"FRAGMENT":"TRIANGLE"));
            )

            //  Check if the command is changing the current state.
            if (activeInAttr[partition][command->getAttribute()] != command->isAttributeActive())
            {
                //  Update active inputs counter.
                activeInputs[partition] = (command->isAttributeActive())?activeInputs[partition] + 1: activeInputs[partition] - 1;
            }

            //  Configure shader output.
            activeInAttr[partition][command->getAttribute()] = command->isAttributeActive();

            //  Recalculate max thread resources.
            maxThreadResources = computeMaxThreadResources();

            break;

        case SET_OUT_ATTR:

            //  Configure which shader output registers are written by the current shader program and sent back.

            //  Check if the shader output register identifier.
            CG_ASSERT_COND(!(command->getAttribute() >= MAX_VERTEX_ATTRIBUTES), "Out of range shader output register identifier.");
            GPU_DEBUG_BOX(
                printf("%s => Setting output attribute %d as %s for partition %s\n", getName(), command->getAttribute(),
                    command->isAttributeActive()?"ACTIVE":"INACTIVE",
                    (partition == VERTEX_PARTITION)?"VERTEX":((partition == FRAGMENT_PARTITION)?"FRAGMENT":"TRIANGLE"));
            )

            //  Check if the command is changing the current state.
            if (activeOutAttr[partition][command->getAttribute()] != command->isAttributeActive())
            {
                //  Update active outputs counter.
                activeOutputs[partition] = (command->isAttributeActive())?activeOutputs[partition] + 1: activeOutputs[partition] - 1;
            }

            //  Configure shader output.
            activeOutAttr[partition][command->getAttribute()] = command->isAttributeActive();

            //  Recalculate max thread resources.
            maxThreadResources = computeMaxThreadResources();

            break;

        case SET_MULTISAMPLING:
 
            //  Set multisampling enabled/disabled.
 
            GPU_DEBUG_BOX(
                printf("%s => Setting multisampling register: %s\n", getName(), command->multisamplingEnabled()?"ENABLED":"DISABLED");
            )
 
            //  Set register.
            multisampling = command->multisamplingEnabled();            
 
            break;

        case SET_MSAA_SAMPLES:
         
            //  Set the MSAA number of samples.
             
            GPU_DEBUG_BOX(
                printf("%s => Setting MSAA samples register: %d\n", getName(), command->samplesMSAA());
            )
 
            //  Set register.
            msaaSamples = command->samplesMSAA();
 
            break;
 
        default:

            //  ERROR.  Unknow command code.

            CG_ASSERT("Unsupported command type.");

            break;
    }

    //  Delete shader command.
    delete command;
}

//  Reserve a vector thread, if available, and allocates the resources required by the thread.
void cmoShaderFetchVector::reserveVectorThread(U64 cycle, U32 partition, U32 &newThread)
{
    U32 requiredElementResources;

    //  Check if there are free threads available.  A free thread should be garanteed due to the backpreasure logic with
    //  the shader input feeders.
    CG_ASSERT_COND(!(freeThreadFIFO.empty()), "No free vector thread available.");
    //  The resource number is the maximum of the required temporal, input and output registers for the current shader
    //  program for the shader target/partition.
    requiredElementResources = GPU_MAX(activeInputs[partition], GPU_MAX(resourceAlloc[partition], activeOutputs[partition]));

    //  Check that the required resources for the vector thread are available.  The backpreasure logic should garantee that
    //  there are enough resources available.
    GPU_ASSERT(
        if (freeResources < requiredElementResources)
        {
            printf("freeResources = %d | maxThreadResources = %d | requiredElementResources = %d | partition = %d\n",
                freeResources, maxThreadResources, requiredElementResources, partition);
            CG_ASSERT("There aren't enough resources for the vector thread to allocate.");
        }
    )

    //  Get the pointer to the next empty entry in the vector thread array.
    newThread = freeThreadFIFO.pop();

    //  Check that the entry is marked as free.
    CG_ASSERT_COND(!(!threadArray[newThread].free), "Next free entry in the vector thread array is not marked as free.");
    //  Set reserved vector thread array entry as no longer free.
    threadArray[newThread].free = false;

    //  Allocate the vector thread resources.  Decrement the resource counter.
    freeResources -= requiredElementResources;

//printf("cmoShaderFetchVector (%lld) => Allocating resources.  ThreadID = %d | Partition = %s | ElemResources = %d | FreeResources = %d | NumResources = %d\n",
//    cycle, newThread, (partition == VERTEX_PARTITION) ? "VERTEX" : "FRAGMENT", 
//    requiredElementResources, freeResources, numResources);

}

//  Process a new input vertex.
void cmoShaderFetchVector::processShaderInput(U64 cycle, ShaderInput *input)
{
    U32 partition;

    //  A new shader input has arrived.

    //  Select shader partition given shader input type.
    switch(input->getInputMode())
    {
        case SHIM_VERTEX:

            partition = VERTEX_PARTITION;            
            break;

        case SHIM_TRIANGLE:

            partition = TRIANGLE_PARTITION;
            break;

        case SHIM_FRAGMENT:

            partition = FRAGMENT_PARTITION;
            break;

        case SHIM_MICROTRIANGLE_FRAGMENT:
            
            partition = FRAGMENT_PARTITION;
            break;

        default:
            CG_ASSERT("Unsupported shader input mode.");
            break;
    }

    //  Check the current input thread is already full.
    if (nextInputElement == vectorLength)
    {
        //  Reserve a new thread for the new set of shader inputs.
        reserveVectorThread(cycle, partition, inputThread);

        //  Reset input element index to the start of the vector thread.
        nextInputElement = 0;

        //  Set thread start pc based on the shader target/partition.
        threadArray[inputThread].PC = startPC[partition];

        //  Set partition for the vector thread.
        threadArray[inputThread].partition = partition;
    }

    GPU_ASSERT(
        if (threadArray[inputThread].partition != partition)
        {
            CG_ASSERT("Shader input partition doesn't correspond with the vector thread partition.");
        }
    )

    U32 threadEmuID;

    //  Compute the shader thread ID for the shader behaviorModel based on the vector thread ID and the element to load.
    threadEmuID = inputThread * vectorLength + nextInputElement;

    //  Reset the state of the thread element in the shader behaviorModel.
    bmUnifiedShader.resetShaderState(threadEmuID);

    //  Load the shader input into the input register bank according to the shader input type
    switch(input->getInputMode())
    {
        case SHIM_VERTEX:
            //  Load the input attributes for the shader thread element in the shader behaviorModel.
            bmUnifiedShader.loadShaderState(threadEmuID, IN, input->getAttributes(), 0, MAX_VERTEX_ATTRIBUTES);
            break;

        case SHIM_TRIANGLE:
            //  Load the input attributes for the shader thread element in the shader behaviorModel.
            bmUnifiedShader.loadShaderState(threadEmuID, IN, input->getAttributes(), 0, MAX_TRIANGLE_ATTRIBUTES);
            break;

        case SHIM_FRAGMENT:
            //  Load the input attributes for the shader thread element in the shader behaviorModel.
            bmUnifiedShader.loadShaderState(threadEmuID, IN, input->getAttributes(), 0, MAX_FRAGMENT_ATTRIBUTES);
            break;

        case SHIM_MICROTRIANGLE_FRAGMENT:
            //  Load the input attributes for the shader thread element in the shader behaviorModel.
            bmUnifiedShader.loadShaderState(threadEmuID, IN, input->getAttributes(), 0, MAX_MICROFRAGMENT_ATTRIBUTES);
            break;

        default:
            CG_ASSERT("Unsupported shader input mode.");
            break;
    }

    //  Set PC for the thread element in the shader behaviorModel to the start PC of the vector thread.
    bmUnifiedShader.setThreadPC(threadEmuID, threadArray[inputThread].PC);

    //  Reset vector thread instruction counter.
    threadArray[inputThread].instructionCount = 0;

    //  Store the shader input pointer for the thread element loaded.
    threadArray[inputThread].shInput[nextInputElement] = input;

    //  Reset trace log flag.
    threadArray[inputThread].traceElement[nextInputElement] = false;

    //  Check if vertex trace is enabled.
    if (traceVertex)
    {
        //  Check if the current input for the vertex index to watch.
        if ((input->getInputMode() == SHIM_VERTEX) && (input->getID() == watchIndex))
        {
            printf("%s => Enabling trace for vertex %d instance %d at vector %d element %d\n", getName(),
                input->getShaderInputID().id.vertexID.index, input->getShaderInputID().id.vertexID.instance,
                inputThread, nextInputElement);
            threadArray[inputThread].traceElement[nextInputElement] = true;
        }        
    }
    
    //  Check if fragment trace is enabled.
    if (traceFragment)
    {
        // Check if the current input for the fragment coordinates to watch.
        if ((input->getInputMode() == SHIM_FRAGMENT) &&
            (input->getShaderInputID().id.fragmentID.x == watchFragmentX) &&
            (input->getShaderInputID().id.fragmentID.y == watchFragmentY))
        {
        
            printf("%s => Enabling trace for pixel (%d, %d) triangle ID %d at vector %d element %d\n", getName(),
                input->getShaderInputID().id.fragmentID.x, input->getShaderInputID().id.fragmentID.y,
                input->getShaderInputID().id.fragmentID.triangle, inputThread, nextInputElement);
            threadArray[inputThread].traceElement[nextInputElement] = true;
        }
    }

    GPU_DEBUG_BOX(
        printf("%s >> New Shader Input for vectorThread = %d element = %d for partition %s\n", getName(), inputThread,
            nextInputElement, partition==VERTEX_PARTITION?"VERTEX":(partition==FRAGMENT_PARTITION)?"FRAGMENT":"TRIANGLE");
    )

    //  Update the index to the next element to load in the input vector thread.
    nextInputElement++;

    //  If all the elements in the input vector thread were loaded set the thread as runnable.
    if (nextInputElement == vectorLength)
    {
        //  Set the fully loaded vector to ready for execution.
        threadArray[inputThread].ready = true;

        //  Update the counter of ready vector threads.
        readyThreads++;

        GPU_DEBUG_BOX(
            printf("%s => All elements have been loaded.  Setting vectorThread %d to READY.\n", getName(), inputThread);
        )
    }

    //  Update statistics.  
    inputs->inc();
    inputInAttribs->inc(activeInputs[partition]);
    inputRegisters->inc(resourceAlloc[partition]);
    inputOutAttribs->inc(activeOutputs[partition]);
}

//  Ends a vector threads and sets it's elements ready to be delivered as shader outputs to a consumer.
void cmoShaderFetchVector::endVectorThread(U32 threadID)
{
    //  Check if the vector thread is in a runnable state.
    GPU_ASSERT(
        if (threadArray[threadID].ready || !threadArray[threadID].blocked || threadArray[threadID].end)
        {
            CG_ASSERT("Vector thread to end is ready or isn't blocked or already marked as ended.");
        }
    )

    //  Set vector thread as ended.
    threadArray[threadID].end = true;

    //  Set vector thread as no longer blocked.
    threadArray[threadID].blocked = false;

    //  Update blocked threads counter.
    blockedThreads--;

    //  Check if the end vector thread FIFO is full.
    CG_ASSERT_COND(!(endThreadFIFO.full()), "End vector thread FIFO is already full.");
    //  Add the vector thread to the end thread FIFO.
    endThreadFIFO.add(threadID);
}

//  Processes a control command from the Decode stage.
void cmoShaderFetchVector::processDecodeCommand(ShaderDecodeCommand *decodeCommand)
{
    U32 vectorThreadID;
    U32 pc;

    vectorThreadID = decodeCommand->getNumThread();
    pc = decodeCommand->getNewPC();

    //  Check that the vector thread identifier is correct.
    CG_ASSERT_COND(!(vectorThreadID > numThreads), "Illegal vector thread identifier.");
    //  Process the received control command from the Decode stage.
    switch(decodeCommand->getCommandType())
    {
        case UNBLOCK_THREAD:

            GPU_DEBUG_BOX(
                printf("%s => UNBLOCK_THREAD VectorThreadID = %d PC = %x\n", getName(), vectorThreadID, pc);
            )

            //  Check if the vector thread was blocked.
            CG_ASSERT_COND(!((threadArray[vectorThreadID].ready) || (!threadArray[vectorThreadID].blocked)), "Vector thread was already in ready state or not blocked.");
            //  Unblock the blocked vector thread.
            threadArray[vectorThreadID].blocked = false;

            //  Set the vector thread as runnable.
            threadArray[vectorThreadID].ready = true;

            //  Update runnable vector threads counter.
            readyThreads++;

            //  Update blocked vector threads counter.
            blockedThreads--;

            //  Update statistics.
            unblocks->inc();

            break;

        case BLOCK_THREAD:

            GPU_DEBUG_BOX(
                printf("%s => BLOCK_THREAD VectorThreadID = %d PC = %x\n", getName(), vectorThreadID, pc);
            )

            //  Check if the vector thread was ready.
            CG_ASSERT_COND(!(!threadArray[vectorThreadID].ready), "Vector thread wasn't in runnable (ready) state.");
            //  Block a vector thread.
            threadArray[vectorThreadID].blocked = true;

            //  Set the vector thread as not runnable.
            threadArray[vectorThreadID].ready = false;

            //  Update the PC for the vector thread to the next instruction.
            threadArray[vectorThreadID].PC = pc + 1;

            //  Update ready vector threads counter.
            readyThreads--;

            //  Update blocked vector threads counter.
            blockedThreads++;

            //  Update statistics.
            blocks->inc();

            break;

        case END_THREAD:

            GPU_DEBUG_BOX(
                printf("%s => END_THREAD VectorThreadID = %d PC = %x\n", getName(), vectorThreadID, pc);
            )

            //  End the vector thread and set is ready for delivery to the consumer unit.
            endVectorThread(vectorThreadID);

            break;

        case REPEAT_LAST:

            GPU_DEBUG_BOX(
                printf("%s => REPEAT_LAST VectorThreadID = %d PC = %x\n", getName(), vectorThreadID, pc);
            )

            //  Instruction was stalled in decode stage so set PC for the vector thread to the stalled instruction PC.
            threadArray[vectorThreadID].PC = pc;

            //  Set the PC of all the vector thread elements in the shader behaviorModel.
            for(U32 threadEmuID = vectorThreadID * vectorLength; threadEmuID < ((vectorThreadID + 1) * vectorLength); threadEmuID++)
            {
                //  Set thread element PC in the shader behaviorModel.
                bmUnifiedShader.setThreadPC(threadEmuID, threadArray[vectorThreadID].PC);

                //  Update statistics.
                reFetched->inc();
            }


            break;

        case NEW_PC:

            GPU_DEBUG_BOX(
                printf("%s => NEW_PC VectorThreadID = %d PC = %x\n", getName(), vectorThreadID, pc);
            )

            //  Change vector thread PC.
            threadArray[vectorThreadID].PC = pc;

            //  Set the PC of all the vector thread elements in the shader behaviorModel.
            for(U32 threadEmuID = vectorThreadID * vectorLength; threadEmuID < ((vectorThreadID + 1) * vectorLength); threadEmuID++)
            {
                //  Set thread element PC in the shader behaviorModel.
                bmUnifiedShader.setThreadPC(threadEmuID, threadArray[vectorThreadID].PC);
            }

            break;

        case ZEXPORT_THREAD:

            GPU_DEBUG_BOX(
                printf("%s => ZEXPORT_THREAD VectorThreadID = %d PC = %x\n", getName(), vectorThreadID, pc);
            )

            //  Check if the vector thread was ready.
            CG_ASSERT_COND(!(!threadArray[vectorThreadID].ready), "Vector thread wasn't in runnable (ready) state.");
            //  Set thread's Z exported state.
            threadArray[vectorThreadID].zexported = true;

            break;

        default:
            CG_ASSERT("Unknown decode command type.");
            break;
    }

    //  Delete decode command received from the Decode stage.
    delete decodeCommand;
}

//  Fetches a shader instruction.  
void cmoShaderFetchVector::fetchElementInstruction(U64 cycle, U32 threadID, U32 element, U32 shEmuElementID, U32 instruction,
    ShaderExecInstruction *&shExecInstr)
{
    cgoShaderInstr::cgoShaderInstrEncoding *shInstrDec;
    cgoShaderInstr *shInstr;
    char buffer[256];
    U32 partition;

    partition = threadArray[threadID].partition;

    //  Fetch instruction from bmoUnifiedShader.
    shInstrDec = bmUnifiedShader.fetchShaderInstructionLight(shEmuElementID, threadArray[threadID].PC + instruction, partition);
    shInstr = shInstrDec->getShaderInstruction();

    //  Check that a instruction was actually fetched (instructions outside loaded program memory).
    GPU_ASSERT(
        if (shInstr == NULL)
        {
            sprintf(buffer, "Fetching instruction from unloaded address.  Thread %d PC %x\n",
                threadID, threadArray[threadID].PC + instruction);
            CG_ASSERT(buffer);
        }
    )

    //  Check if SIMD + scalar fetch is enabled.
    if (scalarALU)
    {
        //  Check if a SIMD instruction was already fetched for the thread.
        if (fetchedSIMD && (!shInstr->isScalar()))
        {
            GPU_DEBUG_BOX(
                if (element == 0)
                    printf("%s => Ignoring second SIMD instruction fetch.\n", getName());
            )

            //  Create a fake dynamic instruction.
            shExecInstr = new ShaderExecInstruction(*shInstrDec, threadID, element, threadArray[threadID].PC + instruction, cycle, true);

            //  Copy cookies from shader input and add a new cookie.
            shExecInstr->copyParentCookies(*threadArray[threadID].shInput[element]);
            shExecInstr->addCookie();

            return;
        }

        /*
            NOTE : CHECK REMOVED.  A scalar instruction can take either the SIMD or scalar slot.  As the
                   current implementation only allows a simd4+scalar configuration (two instructions) if
                   both instructions are scalar the fetch is allowed.
        //  Check if a scalar and SIMD instruction were already fetched for the thread.
        if (fetchedScalar && fetchedSIMD && shInstr->isScalar())
        {
            GPU_DEBUG_BOX(
                if (element == 0)
                    printf("cmoShaderFetchVector => Ignoring second scalar instruction fetch.\n");
            )

            //  Create a fake dynamic instruction.
            shExecInstr = new ShaderExecInstruction(*shInstrDec, threadID, element, threadArray[threadID].PC + instruction, cycle, true);

            //  Copy cookies from shader input and add a new cookie.
            shExecInstr->copyParentCookies(*threadArray[threadID].shInput[element]);
            shExecInstr->addCookie();

            return;
        }*/

        //  Update fetched instruction flags for the current thread.  A scalar instruction may fit in the
        //  SIMD slot if the scalar slot is already taken.
        fetchedSIMD = fetchedSIMD || !shInstr->isScalar() || (fetchedScalar && shInstr->isScalar());
        fetchedScalar = fetchedScalar || shInstr->isScalar();
    }
    
    //  Check if Scalar (scalar) ALU architecture is enabled.
    if (soaALU)
    {
        GPU_ASSERT(
            //  Check that the instruction fetched is Scalar compatible.
            if (!shInstr->isSOACompatible())
            {           
                char dis[256];
                shInstr->disassemble(dis, MAX_INFO_SIZE);
                sprintf(buffer, "Instruction is not Scalar compatible.  Th %d (Elem %d) PC %x : %s\n",
                    threadID, element, threadArray[threadID].PC + instruction, dis);
                CG_ASSERT(buffer);
            }
        )
    }
    

    //  Create a new dynamic instruction.  */
    shExecInstr = new ShaderExecInstruction(*shInstrDec, threadID, element, threadArray[threadID].PC + instruction, cycle, false);

    //  Copy cookies from shader input and add a new cookie.
    shExecInstr->copyParentCookies(*threadArray[threadID].shInput[element]);
    shExecInstr->addCookie();

    //  Check if vertex or fragment tracing is enabled.
    if (traceVertex || traceFragment)
    {
        //  Check if the vertex to trace is in the current vector thread.
        if (threadArray[threadID].traceElement[element])
        {
            //  Enable tracing on the fetched instruction.
            shExecInstr->enableTraceExecution();
        }
    }
   
    //  Update statistics.
    fetched->inc();
}


//  Fetches instructions for all the elements in vector thread from the bmoUnifiedShader.
void cmoShaderFetchVector::fetchVectorInstruction(U64 cycle, U32 threadID)
{
    U32 startShEmuElementID;

    //  Allocate instruction buffer for each element (shader input) in the vector thread.
    for(U32 instr = 0; instr < instrCycle; instr++)
    {
        //  Allocate an array of ShaderExecInstructions for all the elements in the vector.
        vectorFetch[instr] = new ShaderExecInstruction*[vectorLength];

        //  Check allocation.
        CG_ASSERT_COND(!(vectorFetch[instr] == NULL), "Error allocating buffer for instructions for each element in the vector thread.");    }

    //  Compute the bmoUnifiedShader identifier for the first element in the vector thread
    startShEmuElementID = threadID * vectorLength;

    //  Fetch instruction in the Shader Behavior Model for all the elements in the vector thread.
    for(U32 element = 0; element < vectorLength; element++)
    {
        //  Reset fetched instructions flags for current element.
        fetchedSIMD = fetchedScalar = false;

        //  Fetch instructions for vector thread element from the bmoUnifiedShader.
        for(U32 instruction = 0; instruction < instrCycle; instruction++)
        {
            //  Fetch and store instruction for the thread.
            fetchElementInstruction(cycle, threadID, element, startShEmuElementID + element, instruction, vectorFetch[instruction][element]);
        }
    }

    U32 fetchedInstructions = 0;

    //  Compute how many instructions were actually fetched.
    if (scalarALU && fetchedScalar && fetchedSIMD)
        fetchedInstructions = 2;
    else if ((scalarALU && fetchedScalar && !fetchedSIMD) || (scalarALU && !fetchedScalar && fetchedSIMD))
        fetchedInstructions = 1;
    else
        fetchedInstructions = instrCycle;

    //  Update vector thread PC.
    threadArray[threadID].PC += fetchedInstructions;
}

//  Sends instructions for a vector thread to the decode stage.
void cmoShaderFetchVector::sendVectorInstruction(U64 cycle, U32 threadID)
{
    VectorInstructionFetch *vectorInstruction;

    //  Send all vector instructions fetched in the cycle to the decode stage.
    for(U32 instruction = 0; instruction < instrCycle; instruction++)
    {
        //  Create Vector Instruction Fetch object from the array of ShaderExecInstruction objects
        //  corresponding with the vector insruction fetch.
        vectorInstruction = new VectorInstructionFetch(vectorFetch[instruction]);

        //  Copy cookies from first element ShaderExecInstruction object.
        vectorInstruction->copyParentCookies(*vectorFetch[instruction][0]);

        //  Send vector instruction to decode stage.
        instructionSignal->write(cycle, vectorInstruction);

        GPU_DEBUG_BOX(
            char dis[4096];
            ShaderExecInstruction *shExecInstr = vectorFetch[instruction][0];
            if (!shExecInstr->getFakeFetch())
            {
                shExecInstr->getShaderInstruction().getShaderInstruction()->disassemble(&dis[0], 4096);
                printf("%s => Vector instruction (%d) fetched for vectorThread = %d | PC = %x : %s\n",
                    getName(), instruction, threadID, shExecInstr->getPC(), dis);
            }
            else
            {
                printf("%s => Vector instruction (%d) is a fake instruction for vectorThread = %d\n",
                    getName(), instruction, threadID);
            }
        )
    }
}

//  Send shader output to consumer.
void cmoShaderFetchVector::sendOutput(U64 cycle, U32 threadID, U32 elementID)
{
    ShaderInput *shOutput;
    U32 shaderElementID;
    U32 i;
    bool killed;

    //  Read Output data from bmoUnifiedShader.
    shOutput = threadArray[threadID].shInput[elementID];

    //  Convert thread and element identifier to identifier for bmoUnifiedShader.
    shaderElementID = threadID * vectorLength + elementID;

    //  Read the result output registers and write them into the shader output array.
    bmUnifiedShader.readShaderState(shaderElementID, OUT, shOutput->getAttributes());

    //  Check if the only sample for the shader thread was aborted/killed.  
    if (bmUnifiedShader.threadKill(shaderElementID, 0)) shOutput->setKilled();

    //  Send shader output data to consumer.
    outputSignal->write(cycle, shOutput, transCycles + OUTPUT_DELAY_LATENCY);

    if (threadArray[threadID].traceElement[elementID])
    {
        if (shOutput->getInputMode() == SHIM_FRAGMENT)
        {
            printf("%s => Sending output for pixel (%d, %d) triangle ID %d at vector %d element %d\n", getName(),
                shOutput->getShaderInputID().id.fragmentID.x, shOutput->getShaderInputID().id.fragmentID.y,
                shOutput->getShaderInputID().id.fragmentID.triangle, threadID, elementID);
         
            Vec4FP32 *attributes = shOutput->getAttributes();
            for(U32 o = 0; o < SHADER_ATTRIBUTES; o++)
                if (activeOutAttr[FRAGMENT_PARTITION][o])
                    printf("   O[%02d] = {%f, %f, %f, %f}\n", o,
                        attributes[o][0], attributes[o][1], attributes[o][2], attributes[o][3]);
                
        }
        else if (shOutput->getInputMode() == SHIM_VERTEX)
        {
            printf("%s => Sending output for vertex with index %d instance %d at vector %d element %d\n", getName(),
                shOutput->getShaderInputID().id.vertexID.index,
                shOutput->getShaderInputID().id.vertexID.instance, threadID, elementID);
                
            Vec4FP32 *attributes = shOutput->getAttributes();
            for(U32 o = 0; o < SHADER_ATTRIBUTES; o++)
                if (activeOutAttr[VERTEX_PARTITION][o])
                    printf("   O[%02d] = {%f, %f, %f, %f}\n", o,
                        attributes[o][0], attributes[o][1], attributes[o][2], attributes[o][3]);
        }
    }
    

    GPU_DEBUG_BOX(
        printf("%s => Output sent\n", getName());
        printf("vo[0] = {%f %f %f %f}\n",
            shOutput->getAttributes()[0][0],
            shOutput->getAttributes()[0][1],
            shOutput->getAttributes()[0][2],
            shOutput->getAttributes()[0][3]);

        printf("vo[1] = {%f %f %f %f}\n",
            shOutput->getAttributes()[1][0],
            shOutput->getAttributes()[1][1],
            shOutput->getAttributes()[1][2],
            shOutput->getAttributes()[1][3]);
    )

    //  Update statistics.
    outputs->inc();
}

void cmoShaderFetchVector::processOutputs(U64 cycle)
{
    ConsumerState consumerState;
    ConsumerStateInfo *consumerStateInfo;
    ShaderInput *shOutput;

    //  Read consumer state.  This is a permament signal.
    if (!consumerSignal->read(cycle, (DynamicObject *&) consumerStateInfo))
    {
        //  No signal?  Electrons on strike!!!  So we go to strike too :).
        CG_ASSERT("No signal received from Shader consumer.");
    }
    else
    {
        //  Store consumer state.
        consumerState = consumerStateInfo->getState();

        //  Delete received consumer state.
        delete consumerStateInfo;
    }

    //  Check if there is a transmission in progress.
    if (transInProgress)
    {
        GPU_DEBUG_BOX(
            printf("%s => Transmission in progress Vector Thread = %d | Output Element = %d | Remaining cycles = %d.\n",
                getName(), outputThread, nextOutputElement, transCycles);
        )

        //  Update number of remaining transmission cycles.
        transCycles--;

        //  Check end-of-transmission.
        if (transCycles == 0)
        {
            //  Mark that the current transmission has finished.  Next transmission will start in the next cycle.
            transInProgress = false;

            //  Update index to the next element in the output vector thread to transmite to the consumer.
            nextOutputElement += outputCycle;

            //  Check if all the elements in the vector thread were sent.
            if (nextOutputElement == vectorLength)
            {
                //  Free the vector thread.
                freeVectorThread(cycle, outputThread, outputResources);

                GPU_DEBUG_BOX(
                    printf("%s => Vector thread output fully transmited.\n", getName());
                )
            }
        }
    }
    else
    {
        //  Check if there are outputs to transmit: current output vector thread has not been fully transmited or
        //  there are more vector threads in the end thread FIFO.
        if ((nextOutputElement < vectorLength) || !endThreadFIFO.empty())
        {
            //  Check if consumer is ready to receive a new output.
            if (consumerState == CONS_READY)
            {
                //  Check if the last output vector thread was fully transmited.
                if (nextOutputElement == vectorLength)
                {
                    //  Get next end thread.
                    outputThread = endThreadFIFO.pop();

                    GPU_DEBUG_BOX(
                        printf("%s => Initiating transference of outputs for vectorThread = %d\n", getName(), outputThread);
                    )

                    //  Reset index to the next output element.
                    nextOutputElement = 0;

                    //  Get partition for the output thread.
                    U32 partition = threadArray[outputThread].partition;

                    //  Calculate the number of resources allocated per thread element.
                    outputResources = GPU_MAX(activeInputs[partition], GPU_MAX(resourceAlloc[partition], activeOutputs[partition]));

                    currentActiveOutputs = activeOutputs[partition];
                }

                //  Calculate the number of cycles for output transmission (only vertices).
                transCycles = (U32) ceil(((double) currentActiveOutputs) * OUTPUT_TRANSMISSION_LATENCY);

                //  Read shader output from Shader Behavior Model.
                shOutput = threadArray[outputThread].shInput[nextOutputElement];

                //  Determine transmision cycles for fragment and triangle outputs.
                if (shOutput->getInputMode() == SHIM_FRAGMENT)
                {
                    //  Only one cycle transmission latency (1 or 2 attributes only).
                    transCycles = 1;
                }
                else if (shOutput->getInputMode() == SHIM_TRIANGLE)
                {
                    //  Two cycle transmission latency (4 attributes).
                    transCycles = 2;
                }
                
                // Send outputs from vector thread to the consumer.
                for(U32 element = 0; element < outputCycle; element++)
                    sendOutput(cycle, outputThread, nextOutputElement + element);

                //  If multicycle transmission, set transInProgress.
                if (transCycles > 1)
                {
                    //  Multicyle transmission.  Decrease cycle count.  Set transmission in progress on.
                    transCycles--;
                    transInProgress = true;
                }
                else
                {
                    //  Mark that the current transmission has finished.  Next transmission will start in the next cycle.                    
                    transInProgress = false;
                    
                    //  Update index to the next element in the output vector thread to transmite to the consumer.
                    nextOutputElement += outputCycle;

                    //  Check if all the elements in the vector thread were sent.
                    if (nextOutputElement == vectorLength)
                    {
                        //  Free the vector thread.
                        freeVectorThread(cycle, outputThread, outputResources);

                        GPU_DEBUG_BOX(
                            printf("%s => Vector thread output fully transmited.\n", getName());
                        )
                    }
                }
            }
        }
    }
}

U32 cmoShaderFetchVector::computeMaxThreadResources()
{
    U32 maxResources = 1;
    
    for(U32 target = 0; target < 3; target++)
        maxResources = GPU_MAX(maxResources, resourceAlloc[target]);
        
    for(U32 target = 0; target < 3; target++)
        maxResources = GPU_MAX(maxResources, activeInputs[target]);

    for(U32 target = 0; target < 3; target++)
        maxResources = GPU_MAX(maxResources, activeOutputs[target]);        

    return maxResources;
}


//  Debug info.  Returns a string with the state of the vector shader fetch stage.
void cmoShaderFetchVector::getState(string &stateString)
{
    stringstream stateStream;

    stateStream.clear();

    stateStream << " state = ";

    switch(shState)
    {
        case SH_EMPTY:
            stateStream << "SH_EMPTY";
            break;
        case SH_READY:
            stateStream << "SH_READY";
            break;
        case SH_BUSY:
            stateStream << "SH_BUSY";
            break;
        default:
            stateStream << "undefined";
            break;
    }

    stateStream << " | Ready Thread Group = " << readyThreads;
    stateStream << " | Scan Thread = " << scanThread;

    stateString.assign(stateStream.str());
}


//  Detects stalls conditions in the Fragment FIFO.
void cmoShaderFetchVector::detectStall(U64 cycle, bool &active, bool &stalled)
{
    //  Stall detection is implemented.
    active = true;

    stalled = false;
}

void cmoShaderFetchVector::stallReport(U64 cycle, string &stallReport)
{
    stringstream reportStream;

    reportStream << getName() << " stall report for cycle " << cycle << endl;
    reportStream << "------------------------------------------------------------" << endl;
    reportStream << " Stall Threashold = " << STALL_CYCLE_THRESHOLD << endl;

    reportStream << " Total Threads = " << numThreads << " | Ready Threads = " << readyThreads << " | Blocked Threads = " << blockedThreads;
    reportStream << " | End Threads = " << endThreadFIFO.items() << " | Free Threads = " << freeThreadFIFO.items() << endl;
    reportStream << " Total Resources = " << numResources << " | freeResources = " << freeResources << endl;
    reportStream << " Transmission In Progress = " << (transInProgress ? "Y" : "N");
    reportStream << " | Trans Cycles Remaining = " << transCycles << endl;

    reportStream << " Thread Array : " << endl;
    for(U32 th = 0; th < numThreads; th++)
    {
        char line[1024];

        sprintf(line, "    Thread ID = %04d | Ready = %s | Blocked = %s | Free = %s | End = %s | PC = %04x | Instr. Exec. Count = %06d | Partition = ",
            th, (threadArray[th].ready ? "Y" : "N"), (threadArray[th].blocked ? "Y" : "N"), (threadArray[th].free ? "Y" : "N"),
            (threadArray[th].end ? "Y" : "N"), threadArray[th].PC, threadArray[th].instructionCount);
        reportStream << line;
        switch(threadArray[th].partition)
        {
            case VERTEX_PARTITION   : reportStream << "VERTEX"; break;
            case FRAGMENT_PARTITION : reportStream << "FRAGMENT"; break;
            case TRIANGLE_PARTITION : reportStream << "TRIANGLE"; break;
        }
        reportStream << endl;
    }

    reportStream << " Shader Programs Loaded : " << endl;

    for(U32 p = 0; p < 2; p++)
    {
        if (p == 0)
            reportStream << "   Vertex Program" << endl;
        if (p == 1)
            reportStream << "   Fragment Program" << endl;

        bool programEnd = false;

        U32 pc = startPC[p] + p * UNIFIED_INSTRUCTION_MEMORY_SIZE;

        while (!programEnd)
        {
            cgoShaderInstr::cgoShaderInstrEncoding *shInstrDec;
            cgoShaderInstr *shInstr;

            char disasmInstr[256];
            char disasmLine[256];

            shInstrDec = bmUnifiedShader.fetchShaderInstructionLight(0, pc, p);
            shInstr = shInstrDec->getShaderInstruction();

            shInstr->disassemble(disasmInstr, MAX_INFO_SIZE);

            delete shInstr;
            delete shInstrDec;

            sprintf(disasmLine, "      %04X : %s", pc, disasmInstr);
            reportStream << disasmLine << endl;

            pc++;
            
            programEnd = shInstr->isEnd() || (pc == 2 * UNIFIED_INSTRUCTION_MEMORY_SIZE);
        }
    }

    stallReport.assign(reportStream.str());
}

//  List the debug commands supported by the Command Processor
void cmoShaderFetchVector::getCommandList(std::string &commandList)
{
    commandList.append("listvertexprogram   - Displays the current vertex shader program.\n");
    commandList.append("listfragmentprogram - Displays the current fragment shader program.\n");
    commandList.append("tracevertex         - Traces the execution of the defined vertex.\n");
    commandList.append("tracefragment       - Traces the execution of the defined fragment.\n");
}

//  Execute a debug command
void cmoShaderFetchVector::execCommand(stringstream &commandStream)
{
    string command;

    commandStream >> ws;
    commandStream >> command;

    if (!command.compare("listvertexprogram"))
    {
        if (!commandStream.eof())
        {
            cout << getName() << "Usage: listvertexprogram" << endl;
        }
        else
        {
            cout << getName() << " >< Listing current vertex shader program." << endl;
            listProgram(VERTEX_PARTITION);
        }
    }
    else if (!command.compare("listfragmentprogram"))
    {
        if (!commandStream.eof())
        {
            cout << getName() << "Usage: listfragmentprogram" << endl;
        }
        else
        {
            cout << getName() << " >> Listing current fragment shader program." << endl;
            listProgram(FRAGMENT_PARTITION);
        }
    }
    else if (!command.compare("tracevertex"))
    {
        commandStream >> ws;
        
        if (commandStream.eof())
        {
            cout << "Usage : tracevertex <on | off> <index>" << endl;
        }
        else
        {
            string enableParam;
            bool enableVTrace;
            
            commandStream >> enableParam;
            
            bool error = false;
            if (!enableParam.compare("on"))
                enableVTrace = true;
            else if (!enableParam.compare("off"))
                enableVTrace = false;
            else
            {
                error = true;
                cout << getName() << ">> Usage : tracevertex <on | off> <index>" << endl;
            }
            
            commandStream >> ws;
            
            if (commandStream.eof())
            {
                cout << getName() << " >> Usage : tracevertex <on | off> <index>" << endl;
            }
            else
            {
                commandStream >> watchIndex;
                traceVertex = enableVTrace;
                
                if (enableVTrace)
                    cout << getName() << " >> Enabling vertex trace for index " << watchIndex << "." << endl;
                else
                    cout << getName() << " >> Disabling vertex trace." << endl;
            }
        }
    }    
    else if (!command.compare("tracefragment"))
    {
        commandStream >> ws;
        
        if (commandStream.eof())
        {
            cout << "Usage : tracefragment <on | off> <x> <y>" << endl;
        }
        else
        {
            string enableParam;
            bool enableFTrace;
            
            commandStream >> enableParam;
            
            bool error = false;
            if (!enableParam.compare("on"))
                enableFTrace = true;
            else if (!enableParam.compare("off"))
                enableFTrace = false;
            else
            {
                error = true;
                cout << getName() << ">> Usage : tracefragment <on | off> <x> <y>" << endl;
            }
            
            commandStream >> ws;
            
            if (commandStream.eof())
            {
                cout << getName() << " >> Usage : tracefragment <on | off> <x> <y>" << endl;
            }
            else
            {
                commandStream >> watchFragmentX;
                commandStream >> ws;
                
                if (commandStream.eof())
                {
                    cout << getName() << " >> Usage : tracefragment <on | off> <x> <y>" << endl;
                }
                else
                {
                    commandStream >> watchFragmentY;
                    
                    traceFragment = enableFTrace;
                    
                    if (enableFTrace)
                        cout << getName() << " >> Enabling fragment trace for coordinate (" << watchFragmentX << ", " << watchFragmentY << ")." << endl;
                    else
                        cout << getName() << " >> Disabling fragment trace." << endl;
                }
            }
        }
    }    
    else
    {
        cout << "Unsupported mdu command >> " << command;

        while (!commandStream.eof())
        {
            commandStream >> command;
            cout << " " << command;
        }

        cout << endl;
    }
}

//  Displays the disassembled current shader program in the defined partition.
void cmoShaderFetchVector::listProgram(U32 partition)
{
    char buffer[1024];
    
    cout << endl;
    sprintf(buffer, " PC = %03x | Resources = %d | Max Resources = %d\n",
        startPC[partition], resourceAlloc[partition], maxThreadResources);
    cout << buffer << endl;

    bool programEnd = false;
    U32 pc = startPC[partition];

    //  Read and disassemble consecutive instructions from memory until the end flag
    //  is discovered or an undefined instruction is found.
    while (!programEnd)
    {
        cgoShaderInstr *shInstr;
        shInstr = bmUnifiedShader.readShaderInstruction(pc + partition * UNIFIED_INSTRUCTION_MEMORY_SIZE);
        if (shInstr != NULL)
        {
            sprintf(buffer, "  %03x : ", pc);
            cout << buffer;
            shInstr->disassemble(buffer, MAX_INFO_SIZE);
            cout << buffer << endl;
            programEnd = shInstr->getEndFlag();
        }
        else
        {
            cout << " Unexpected program end" << endl;
            programEnd = true;
        }
        pc++;
    }
    cout << endl;
    cout << endl;
}
