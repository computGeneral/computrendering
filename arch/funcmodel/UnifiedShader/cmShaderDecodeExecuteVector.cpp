/**************************************************************************
 *
 * Vector Shader Unit Decode and Execute cmoMduBase.
 *
 */

/**
 *
 *  @file cmoShaderDecExeVector.cpp
 *
 *  This file implements a cmoShaderDecExeVector cmoMduBase.
 *
 *  Defines and implements a simulator cmoMduBase for the Decode,
 *  Execute and WriteBack stages of a Shader pipeline.
 *
 */

#include "cmShaderDecodeExecuteVector.h"
#include "cmInstructionFetchVector.h"
#include "cmTextureUnitStateInfo.h"
#include "cmTextureRequest.h"
#include "cmTextureResult.h"

#include <string>

using namespace std;

using namespace cg1gpu;

using cg1gpu::tools::Queue;

// cmoShaderDecExeVector constructor
cmoShaderDecExeVector::cmoShaderDecExeVector(bmoUnifiedShader &bmShader, U32 threads, U32 vecLength,
    U32 vecALUWidth,  char *vecALUConf, bool waitOnStall_, bool explicitBlock_,
    U32 textClockRatio, U32 textUnits, U32 requestsCycle, U32 requestsUnit,
    char *name, char *shPrefix, char *sh2Prefix, cmoMduBase *parent):

    //  Initializations.
    cmoMduMultiClk(name, parent), bmUnifiedShader(bmShader), vectorThreads(threads), vectorLength(vecLength), 
    vectorALUWidth(vecALUWidth), vectorALUConfig(vecALUConf),
    waitOnStall(waitOnStall_), explicitBlock(explicitBlock_),
    textureClockRatio(textClockRatio), textureUnits(textUnits),
    textureRequestRate(requestsCycle), requestsPerTexUnit(requestsUnit)

{

    DynamicObject* defValue[1];
    char fullName[64];
    char postfix[32];
    char tuPrefix[64];

    CG_INFO("Using Vector Shader (Decode/Execute)");

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
            CG_ASSERT("The shader vector length must be at least 1.");
        if (vectorALUWidth == 0)
            CG_ASSERT("The number of ALUs in the vector ALU mustbe at least 1.");
        if (GPU_MOD(vectorLength, vectorALUWidth) != 0)
            CG_ASSERT("The vector lenght must be a multiple of the vector ALU width.");
        if (instrCycle == 0)
            CG_ASSERT("At least one instruction from a shader thread must be decoded per cycle.");
        if (scalarALU && (instrCycle != 2))
            CG_ASSERT("Scalar ALU only supported for 2-way configuration.");
        if ((textureUnits > 0) && (textureRequestRate == 0))
            CG_ASSERT("The texture request rate must be at least 1.");
        if ((textureUnits > 0) && (requestsPerTexUnit == 0))
            CG_ASSERT("The request per texture unit must be at least 1.");
    )

    //  Create the full name and postfix for the statistics.
    sprintf(fullName, "%s::%s", shPrefix, name);
    sprintf(postfix, "ShDX-%s", shPrefix);

//printf("ShDecExec %s -> %p \n", fullName, this);

    //  Create statistics.
    execInstr = &getSM().getNumericStatistic("ExecutedInstructions", U32(0), fullName, postfix);
    blockedInstr = &getSM().getNumericStatistic("BlockedInstructions", U32(0), fullName, postfix);
    fakedInstr = &getSM().getNumericStatistic("FakedInstructions", U32(0), fullName, postfix);
    removedInstr = &getSM().getNumericStatistic("RemovedInstructions", U32(0), fullName, postfix);
    blocks = &getSM().getNumericStatistic("BlockCommands", U32(0), fullName, postfix);
    unblocks = &getSM().getNumericStatistic("UnblockCommands", U32(0), fullName, postfix);
    ends = &getSM().getNumericStatistic("EndCommands", U32(0), fullName, postfix);
    replays = &getSM().getNumericStatistic("ReplayCommands", U32(0), fullName, postfix);
    textures = &getSM().getNumericStatistic("TextureRequests", U32(0), fullName, postfix);
    zexports = &getSM().getNumericStatistic("ZExports", U32(0), fullName, postfix);

    //  Initialize signals.

    //  Create command signal from the Command Processor.
    commandSignal[VERTEX_PARTITION] = newInputSignal("ShaderDecodeExecuteCommand", 1, 1, sh2Prefix);
    commandSignal[FRAGMENT_PARTITION] = newInputSignal("ShaderDecodeExecuteCommand", 1, 1, shPrefix);

    //  Create instruction signal from the Shader Fetch.
    instructionSignal = newInputSignal("cgoShaderInstr", instrCycle, 1, shPrefix);

    //  Create control signal to the Shader Fetch.
    //controlSignal = newOutputSignal("ShaderControl", (2 + textureUnits) * GPU_MAX(threadGroup, threadsCycle) * instrCycle, 1, shPrefix);
    controlSignal = newOutputSignal("ShaderControl", 1 + (MAX_EXEC_BW + 1) * instrCycle, 1, shPrefix);

    //  Create state signal to the Shader Fetch.
    decodeStateSignal = newOutputSignal("ShaderDecodeState", 1, 1, shPrefix);

    //  Create signals that simulate the execution latencies.
    executionStartSignal = newOutputSignal("ShaderExecution", vectorALUWidth * instrCycle * MAX_EXEC_BW, MAX_EXEC_LAT, shPrefix);
    executionEndSignal = newInputSignal("ShaderExecution", vectorALUWidth * instrCycle * MAX_EXEC_BW, MAX_EXEC_LAT, shPrefix);

    //  Build initial signal state array.
    defValue[0] = new ShaderDecodeStateInfo(SHDEC_READY);

    //  Set ShaderDecodeState signal initial value.
    decodeStateSignal->setData(defValue);

    //  Allocate the array of signals with the texture units.
    if (textureUnits > 0)
    {
        textRequestSignal = new Signal*[textureUnits];
        textResultSignal = new Signal*[textureUnits];
        textUnitStateSignal = new Signal*[textureUnits];

        //  Check allocation.
        GPU_ASSERT(
            if (textRequestSignal == NULL)
                CG_ASSERT("Error allocating texture request signal array.");
            if (textResultSignal == NULL)
                CG_ASSERT("Error allocating texture result signal array.");
            if (textUnitStateSignal == NULL)
                CG_ASSERT("Error allocating texture unit state signal array.");
        )
    }

    //  Create signals with the Texture Units.
    for(U32 i = 0; i < textureUnits; i++)
    {
        //  Create the texture unit prefix.
        sprintf(tuPrefix, "%sTU%d", shPrefix, i);

        //  Create request signal to the texture unit.
        textRequestSignal[i] = newOutputSignal("TextureRequest", textureRequestRate, 1, tuPrefix);

        //  Create result signal from the texture unit.
        textResultSignal[i] = newInputSignal("TextureData", 1, 1, tuPrefix);

        //  Create state signal from the texture unit.
        textUnitStateSignal[i] = newInputSignal("TextureUnitState", 1, 1, tuPrefix);
    }

    //  Allocate vector thread state table for decode and execution stage.
    threadInfo = new VectorThreadInfo[vectorThreads];

    //  Check allocation.
    CG_ASSERT_COND(!(threadInfo == NULL), "Error allocating thread state table.");
    //  Allocate auxiliary array for texture access finish threads.
    textThreads = new U32[vectorLength];

    //  Check allocation.
    CG_ASSERT_COND(!(textThreads == NULL), "Error allocating auxiliary array for threads ending texture access.");
    //  Allocate array of texture unit tickets.
    if (textureUnits > 0)
    {
        textureTickets = new U32[textureUnits];

        CG_ASSERT_COND(!(textureTickets == NULL), "Error allocating texture units ticket array.");    }

    //  Allocate buffer for the shader instructions fetched for a vector thread.
    vectorFetch = new ShaderExecInstruction**[instrCycle];

    //  Check allocation.
    CG_ASSERT_COND(!(vectorFetch == NULL), "Error allocating buffer for instructions fetched for a vector thread in a cycle.");
    //  Set pointers to the arrays of pointers of ShaderExecInstruction objects (corresponding with vector instructions) to NULL.
    for(U32 instruction = 0; instruction < instrCycle; instruction++)
        vectorFetch[instruction] = NULL;

    //  Compute the number of cycles/iterations required to execute a shader vector with the available number
    //  of ALUs in the vector ALU array.
    execCyclesPerVector = U32(ceil(F32(vectorLength) / F32(vectorALUWidth)));
    
    //  Get the singleton Shader Architecture Parameters.
    shArchParams = ShaderArchParam::getShaderArchParam();

    string queueName;

    //  Set thread wake up queue name.
    queueName.clear();
    queueName.append("ThreadWakeUpQ");
    threadWakeUpQ.setName(queueName);

    //  Create and initialize thread wake up queue.
    threadWakeUpQ.resize(textureUnits * textureClockRatio);

    //  Change state to reset.
    state = SH_RESET;
}


//  Clock function for the class.  Drives the time of the simulation.
void cmoShaderDecExeVector::clock(U64 cycle)
{
    GPU_DEBUG_BOX(
        printf("%s => Clock %lld.\n", getName(), cycle);
    )

    //  Receive updates from the Command Processor.
    updatesFromCommandProcessor(cycle);

    //  Update state from the Texture units.
    receiveStateFromTextureUnits(cycle);

    //  Check state of the shader.
    if (state == SH_RESET)
    {
        //  Reset the decode and execute state.
        reset();

        //  Change state to ready.
        state = SH_READY;
    }
    else
    {
        //  Receive a vector instructions from a  fetch.
        receiveVectorInstruction(cycle);

        //  Wake up threads pending from texture unit.
        wakeUpTextureThreads(cycle);

        //  Update the write back (end of instruction execution pipeline) stage.
        writeBackStage(cycle);

        //  Update the decode stage state.
        decodeStage(cycle);

        //  Update the execute stage state.
        executeStage(cycle);      

        //  Send requests to the Texture Units.
        sendRequestsToTextureUnits(cycle);
        
        //  Receive results from the Texture Units.
        receiveResultsFromTextureUnits(cycle);
    }

    //  Send decode stage state to fetch.
    sendDecodeState(cycle);    
}

//  Update and simulate a clock in one of the clock domains of the Vector Shader Decode Execute mdu.
void cmoShaderDecExeVector::clock(U32 domain, U64 cycle)
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
        printf("%s => Domain %s Clock %lld.\n", getName(), clockName.c_str(), cycle);
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


//  Simulate and update the state of the GPU domain section of the vector shader decode execute mdu.
void cmoShaderDecExeVector::updateGPUDomain(U64 cycle)
{
    //  Receive updates from the Command Processor.
    updatesFromCommandProcessor(cycle);

    //  Update state from the Texture units.
    receiveStateFromTextureUnits(cycle);

    //  Check state of the shader.
    if (state == SH_RESET)
    {
        //  Reset the decode and execute state.
        reset();

        //  Change state to ready.
        state = SH_READY;
    }
    else
    {
        //  Send requests to the Texture Units.
        sendRequestsToTextureUnits(cycle);
        
        //  Receive results from the Texture Units.
        receiveResultsFromTextureUnits(cycle);
    }
}

//  Simulate and update the state of the shader domain section of the vector shader decode execute mdu.
void cmoShaderDecExeVector::updateShaderDomain(U64 cycle)
{
    //  Check state of the shader.
    if (state == SH_RESET)
    {
        //  Reset the decode and execute state.
        reset();

        //  Change state to ready.
        state = SH_READY;
    }
    else
    {
        //  Receive a vector instructions from a  fetch.
        receiveVectorInstruction(cycle);
        
        //  Wake up threads pending from texture unit.
        wakeUpTextureThreads(cycle);

        //  Update the write back (end of instruction execution pipeline) stage.
        writeBackStage(cycle);

        //  Update the decode stage state.
        decodeStage(cycle);

        //  Update the execute stage state.
        executeStage(cycle);      

    }

    //  Send decode stage state to fetch.
    sendDecodeState(cycle);    
}

//  Receive updates from the Command Processor.
void cmoShaderDecExeVector::updatesFromCommandProcessor(U64 cycle)
{
    ShaderCommand *shCommand;

    //  Process commands from the Command Processor (vertex partition).
    if (commandSignal[VERTEX_PARTITION]->read(cycle, (DynamicObject *&) shCommand))
        processCommand(shCommand);

    //  Process commands from the Command Processor (fragment partition).
    if (commandSignal[FRAGMENT_PARTITION]->read(cycle, (DynamicObject *&) shCommand))
        processCommand(shCommand);

}

//  Send decode stage state to the fetch stage.
void cmoShaderDecExeVector::sendDecodeState(U64 cycle)
{
    ShaderDecodeState decodeState;

    //  Send backpreasure state to fetch stage.
    //  The decoder stage is ready if there isn't a pending vector fetch (stalled) on the
    //  stage or if the current vector fetch is executing and will end in 2 cycles or less.
    decodeState = ((!vectorFetchAvailable) && (cyclesToNextFetch <= 2)) ? SHDEC_READY : SHDEC_BUSY;

    GPU_DEBUG_BOX(
        printf("%s (%lld) => vectorFetchAvailable = %s | cyclesToNextFetch = %d.  Sending %s state to fetch",
            getName(), cycle, vectorFetchAvailable ? "TRUE" : "FALSE", cyclesToNextFetch,
            (decodeState == SHDEC_READY) ? "SHDEC_READY" : "SHDEC_BUSY");
    )
    
    decodeStateSignal->write(cycle, new ShaderDecodeStateInfo(decodeState));
}

//  Update the execute stage state and execute instructions.
void cmoShaderDecExeVector::executeStage(U64 cycle)
{
    ShaderExecInstruction *shExecInstr;

    //  Update the number of cycles until the next execution of shader elements can start.
    if (cyclesToNextExec > 0)
        cyclesToNextExec--;
                           
    //  Check if there is a decoded vector instruction in execution and the next execution can start.
    if (executingVector && (cyclesToNextExec == 0))
    {
        GPU_DEBUG_BOX(
            printf("%s => Executing vector instruction elements starting at %d.  Current repeat rate = %d\n",
                getName(), execElement, currentRepeatRate);
        )

        //  Send instructions for all the elements in the current cycle.
        for(U32 elem = 0; elem < vectorALUWidth; elem++, execElement++)
        {
            //  Send the instruction to execute.
            for(U32 instruction = 0; instruction < execInstructions; instruction++)
            {
                //  Get the shader instruction for the element.
                shExecInstr = vectorFetch[instruction][execElement];

                //  Detect fake instructions.
                if (!shExecInstr->getFakeFetch())
                {
                    //  Send instruction through the execution pipeline.
                    startExecution(cycle, execElement, shExecInstr);
                }
                else
                {
                    //  Delete fake instruction object.

                    GPU_DEBUG_BOX(
                        if (execElement == 0)
                            printf("%s => Deleting fake instruction %d\n", getName(), instruction);
                    )

                    //  Delete the decoded instruction.
                    delete &shExecInstr->getShaderInstruction();

                    //  Remove the instruction.
                    delete shExecInstr;
                }
            }
        }
        
        //  Update the number of cycles until the next execution can start.
        cyclesToNextExec = currentRepeatRate;

        //  Check the number of instructions already executed.
        if (execElement == vectorLength)
        {
            //  The whole vector instruction has been executed so there is no instruction to execute.
            executingVector = false;

            //  Delete the arrays of pointers to ShaderExecInstruction corresponding with the vector instructions executed.
            for(U32 instruction = 0; instruction < execInstructions; instruction++)
            {
                //  Delete array of pointers to ShaderExecInstruction.
                delete[] vectorFetch[instruction];

                //  Set to NULL to detect problems.
                vectorFetch[instruction] = NULL;
            }
        }            
    }
}

//  Update decode stage state and decode instructions.
void cmoShaderDecExeVector::decodeStage(U64 cycle)
{
    ShaderExecInstruction *shExecInstr;

    //  Reset current entry in the register write table.
    regWrites[nextRegWrite] = 0;

    //  Update pointer to the next entry in the register write table.
    nextRegWrite = GPU_MOD(nextRegWrite + 1, MAX_EXEC_LAT);

    //  Update register write cycle counters.  
    //for(i = 0; i < vectorThreads; i++)
    //{
    //    //  Update address register bank write cycle counters.  
    //    for(j = 0; j < MAX_ADDR_BANK_REGS; j++)
    //        if (threadInfo[i].addrBankWrite[j] > 0)
    //            threadInfo[i].addrBankWrite[j]--;

        //  Update Temporal register bank write cycle counters.  
    //   for(j = 0; j < MAX_TEMP_BANK_REGS; j++)
    //        if (threadInfo[i].tempBankWrite[j] > 0)
    //            threadInfo[i].tempBankWrite[j]--;

        //  Update output register bank write cycle counters.  
    //    for(j = 0; j < MAX_OUTP_BANK_REGS; j++)
    //        if (threadInfo[i].outpBankWrite[j] > 0)
    //            threadInfo[i].outpBankWrite[j]--;
    //}

    //  Check if there is an instruction available for decode.
    if (vectorFetchAvailable)
    {
        bool exec = false;
        bool stall = false;
        bool drop = false;

        //  None of the instructions to decode is allowed to execute just yet.
        execInstructions = 0;

        //  Reset reserved ALUs for the current executing thread.
        reserveSIMDALU = false;
        reserveScalarALU = false;

        //  Reset the current repeat rate for the current vector instruction fetch.
        currentRepeatRate = 1;
        
        GPU_DEBUG_BOX(
            printf("%s => Vector fetch received and available for decoding.\n", getName());
        )

        //  Decode all vertex instructions.
        for(U32 instruction = 0; (instruction < instrCycle) && !stall && !drop; instruction++)
        {
            //  Get the shader instruction from the first element in the vector.
            shExecInstr = vectorFetch[instruction][0];

            //  Check for instructions used to pad the vector fetch.
            if (!shExecInstr->getFakeFetch())
            {
                //  Decode the vector instruction.
                decodeInstruction(cycle, instruction, exec, stall);

                //  Check if the instructions have to be dropped.
                drop = !exec && !stall;

                GPU_DEBUG_BOX(
                    printf("%s => Instruction %d decoded: exec = %s | stall = %s | drop = %s\n",
                        getName(), instruction, exec ? "true" : "false", stall ? "true" : "false", drop ? "true" : "false");
                )

                //  Check if the instruction can be executed.
                if (exec)
                {
                    //  Get the type of instruction to execute in this slot.
                    bool isScalar = shExecInstr->getShaderInstruction().getShaderInstruction()->isScalar();
                    //bool isSIMD4 = !isScalar;
                    
                    //  Update ALU reserve flags to control SIMD+scalar execution.
                    //  The first scalar instruction takes the scalar slot.  But a second scalar instruction can also 
                    //  be executed in the SIMD4 slot.
                    reserveScalarALU = reserveScalarALU || isScalar;
                    reserveSIMDALU = reserveSIMDALU || !isScalar || (reserveScalarALU && isScalar);
                }
            }
            else
            {
                GPU_DEBUG_BOX(
                    printf("%s => Instruction %d is a fake instruction.  Skipping decode.\n", getName(), instruction);
                )

                //  Fake instructions are 'executed'.
                exec = true;
            }

            if (exec)
            {
                //  Allow this instruction to be executed.
                execInstructions++;
            }
        }

        //  Check if only half of the instructions were dropped.
        //GPU_ASSERT(
        //    if (drop && (execInstructions > 0))
        //    {
        //        CG_ASSERT("All the vector fetch must be dropped.");
        //    }
        //)

        GPU_DEBUG_BOX(
            printf("%s => Instructions decoded that are executable -> %d\n", getName(), execInstructions);
        )

        //  If an instruction was stalled send a stall/repeat signal to the fetch stage.
        if (stall)
        {
            GPU_DEBUG_BOX(
                printf("%s => Instruction %d is stalled.\n", getName(), execInstructions);
            )

            //  If wait on stall is enabled and there are no instructions to execute in the vector instruction
            //  fetch decode delays the next fetch until the instruction clears the stall condition and can
            //  proceed.  Otherwise the stalling instruction is requested again to fetch.
            if (waitOnStall && (execInstructions == 0))
            {
                GPU_DEBUG_BOX(
                    printf("%s => Decode stage stalled.\n", getName());
                )
            }
            else
            {
                //  Send stall/repeat command to fetch stage.
                repeatInstruction(cycle, execInstructions);

                //  Delete the stalled instruction and all the instructions after the stalled instruction.
                for(U32 element = 0; element < vectorLength; element++)
                {
                    for(U32 instruction = execInstructions; instruction < instrCycle; instruction++)
                    {
                        GPU_DEBUG_BOX(
                            if (element == 0)
                                printf("%s => Deleting instruction %d due to stall\n", getName(), instruction);
                        )

                        //  Get shader instruction carrier object.
                        shExecInstr = vectorFetch[instruction][element];

                        //  Delete the decoded instruction.
                        delete &shExecInstr->getShaderInstruction();

                        //  Remove the instruction.
                        delete shExecInstr;
                    }
                }

                //  Delete the arrays of pointers to ShaderExecInstruction corresponding with the vector instructions stalled.
                for(U32 instruction = execInstructions; instruction < instrCycle; instruction++)
                {
                    //  Delete array of pointers to ShaderExecInstruction.
                    delete[] vectorFetch[instruction];

                    //  Set to NULL to detect problems.
                    vectorFetch[instruction] = NULL;
                }
            }
        }

        //  Delete all the instructions that have been dropped.
        if (drop)
        {
            //  Delete the ShaderExecInstruction objects for all the elements and vector instructions.
            for(U32 element = 0; element < vectorLength; element++)
            {
                for(U32 instruction = execInstructions; instruction < instrCycle; instruction++)
                {
                    GPU_DEBUG_BOX(
                        if (element == 0)
                            printf("%s => Deleting instruction dropped instruction %d (thread in end/pending jump/block state).\n", getName(), instruction);
                    )

                    //  Get shader instruction carrier object.
                    shExecInstr = vectorFetch[instruction][element];

                    //  Delete the decoded instruction.
                    delete &shExecInstr->getShaderInstruction();

                    //  Remove the instruction.
                    delete shExecInstr;
                }
            }

            //  Delete the arrays of pointers to ShaderExecInstruction objects for the dropped instructions.
            for(U32 instruction = execInstructions; instruction < instrCycle; instruction++)
            {
                //  Delete array of pointers to ShaderExecInstruction.
                delete[] vectorFetch[instruction];

                //  Set to NULL to detect problems.
                vectorFetch[instruction] = NULL;
            }
        }

        //  Set if the vector instruction can start execution.
        executingVector = (execInstructions > 0);

        //  Reset the index of the element to execute to the first element in the vector.
        execElement = 0;

        //  If the vector instruction fetch is not stalled and waiting for the conditions to be cleared
        //  set the decode stage as clear from work.
        vectorFetchAvailable = stall && waitOnStall && !executingVector;

        //  Set the number of cycles the wait between fetching two vector instructions.
        //  The current implementation requires a wait of at least 2 cycles to hide the
        //  latency of fetch->decode->fetch control traffic.
        cyclesBetweenFetches = GPU_MAX(U32(2), currentRepeatRate * U32(ceil(F32(vectorLength) / F32(vectorALUWidth))));

        //  Set cycles to the next expected vector fetch.
        cyclesToNextFetch = cyclesBetweenFetches;
    }
}

//  Sends requests to the texture units.
void cmoShaderDecExeVector::sendRequestsToTextureUnits(U64 cycle)
{
    TextureAccess *textAccess;
    TextureRequest *textRequest;

    //  If Texture Unit attached process pending texture accesses.
    if (textureUnits > 0)
    {
        //  Send texture requests to the current texture unit.
        for(U32 i = 0; i < textureRequestRate; i++)
        {
            //  Process texture accesses waiting in the shader behaviorModel.
            textAccess = bmUnifiedShader.nextTextureAccess();

            //  Check if there was a texture access available.
            if (textAccess != NULL)
            {
                //  NOTE:  IT SHOULD COPY COOKIES FROM THE SHADER INSTRUCTIONS GENERATING THE ACCESS??

                //  Set the cycle the texture access was issued to the Texture Unit.
                textAccess->cycle = cycle;

                textRequest = new TextureRequest(textAccess);

                //  Copy cookies from original instruction and add a new cookie.
                //textRequest->copyParentCookies(?shExecInstr);
                //textRequest->addCookie();

                GPU_DEBUG_BOX(
                    printf("%s (%lld) => Sending texture request to texture unit %d\n", getName(), cycle, nextRequestTU);
                )

                //  Send the request for the texture access to the texture unit.
                textRequestSignal[nextRequestTU]->write(cycle, textRequest);

                //  Update the number of texture requests send to the current texture unit.
                tuRequests++;

                //  Update statistics.
                textures->inc();
            }
            else
            {
                //  Check if enough requests sent for this texture unit.
                if (tuRequests >= requestsPerTexUnit)
                {
                    //  Skip to the next texture unit.
                    tuRequests = 0;
                    nextRequestTU = GPU_MOD(nextRequestTU + 1, textureUnits);
                }
            }
        }
    }
}

//  Receive results from the texture units.
void cmoShaderDecExeVector::receiveResultsFromTextureUnits(U64 cycle)
{
    TextureResult *textResult;

    //  Receive texture access results from the texture units.
    for(U32 i = 0; i < textureUnits; i++)
    {
        //  Receive texture access results from a texture unit.
        if (textResultSignal[i]->read(cycle, (DynamicObject *&) textResult))
        {
            //  Complete the texture access in the shader behaviorModel.
            bmUnifiedShader.writeTextureAccess(textResult->getTextAccessID(), textResult->getTextSamples(), textThreads);

            //  Get the shader behaviorModel element identifier.
            U32 shEmuElemID = textThreads[0];

            //  Retrieve the vector thread identifier from the shader behaviorModel element identifier.
            U32 threadID = shEmuElemID / vectorLength;

            //  Update the number of pending texture fetches for elements in the vector.
            threadInfo[threadID].pendingTexElements -= STAMP_FRAGMENTS;

            if (threadInfo[threadID].pendingTexElements == 0)
            {
                //  Add thread to the queue of threads pending from being awaken by
                //  Texture Unit.
                threadWakeUpQ.add(threadID);
            }

            //  Delete texture result object.
            delete textResult;
        }
    }
}

//  Awakes threads pending from texture unit.
void cmoShaderDecExeVector::wakeUpTextureThreads(U64 cycle)
{
    //  Check if there are threads to awake in the queue.
    for(U32 th = 0; th < threadWakeUpQ.items(); th++)
    {
        U32 threadID = threadWakeUpQ.pop();

        CG_ASSERT_COND(!(!threadInfo[threadID].waitTexture), "Waking up a thread that is not waiting for textures.");        
        GPU_DEBUG_BOX(
            printf("%s => Wake up thread waiting for texture.  ThreadID = %d\n", getName(), threadID);
        )
                
        //  Unblock thread if thread was waiting a texture, the thread is blocked, but the thread is not waiting a pending
        //  jump to finalize and the thread is not waiting in the CG1_ISA_OPCODE_END thread state.
        if (threadInfo[threadID].waitTexture && !threadInfo[threadID].ready &&
            !threadInfo[threadID].pendingJump && !threadInfo[threadID].end)
            unblockThread(cycle, threadID, 0, NULL);

        //  Mark thread as no longer waiting for texture result.
        threadInfo[threadID].waitTexture = false;

        //  Check if thread has received the CG1_ISA_OPCODE_END instruction and finished all previous instructions.
        if (threadInfo[threadID].end && (threadInfo[threadID].pendingInstructions == 0))
        {
            //  Remove thread end condition..
            threadInfo[threadID].end = false;

            //  Remove thread block condition.
            threadInfo[threadID].ready = true;

            //  Send CG1_ISA_OPCODE_END signal to Fetch.
            endThread(cycle, threadID, 0, NULL);
        }
    }
}

//  Receive state from the Texture units.
void cmoShaderDecExeVector::receiveStateFromTextureUnits(U64 cycle)
{
    TextureUnitStateInfo *textUnitStateInfo;

    //  Receive state signal from the Texture Unit.
    for(U32 i = 0; i < textureUnits; i++)
    {
        //  Check if the state signal is received.
        if (textUnitStateSignal[i]->read(cycle, (DynamicObject *&) textUnitStateInfo))
        {
//printf("ShDE(%p) %lld => Texture tickets received from Texture Unit %d\n", this,
//cycle, i);
            //  Update number of texture tickets from Texture Unit.
            textureTickets[i] += 4;

            //  Delete carrier object.
            delete textUnitStateInfo;
        }
    }    
}

//  Ask the fetch stage to change the program counter.
void cmoShaderDecExeVector::changePC(U64 cycle, U32 threadID, U32 PC, ShaderExecInstruction *shExecInstr)
{
    ShaderDecodeCommand *decodeCommand;

    //  Ask the Shader Fetch to refetch the instruction later.
    decodeCommand = new ShaderDecodeCommand(NEW_PC, threadID, PC);

    //  Copy cookies from original instruction and add a new cookie.
    decodeCommand->copyParentCookies(*shExecInstr);
    decodeCommand->addCookie();

    //  Send control command to fetch.
    controlSignal->write(cycle, decodeCommand) ;

    GPU_DEBUG_BOX(
        printf("%s => Sending NEW_PC vectorThreadID = %d PC = %06x.\n", getName(), threadID, PC);
    )

    //  Update statistics.
    replays->inc();
}

//  Ask the fetch stage to replay the current instruction because of a dependence.
void cmoShaderDecExeVector::replayInstruction(U64 cycle, U32 threadID, U32 PC, ShaderExecInstruction *shExecInstr)
{
    ShaderDecodeCommand *decodeCommand;

    //  Ask the Shader Fetch to refetch the instruction later.
    decodeCommand = new ShaderDecodeCommand(REPEAT_LAST, threadID, PC);

    //  Copy cookies from original instruction and add a new cookie.
    decodeCommand->copyParentCookies(*shExecInstr);
    decodeCommand->addCookie();

    //  Send control command to fetch.
    controlSignal->write(cycle, decodeCommand) ;

    GPU_DEBUG_BOX(
        printf("%s => Sending REPEAT_LAST vectorThreadID = %d PC = %06x.\n", getName(), threadID, PC);
    )

    //  Update statistics.
    replays->inc();
}

//  Send to the fetch stage that a vector thread has finished.
void cmoShaderDecExeVector::endThread(U64 cycle, U32 threadID, U32 PC, ShaderExecInstruction *shExecInstr)
{
    ShaderDecodeCommand *decodeCommand;

    //  Tell the Shader Fetch that the thread has finished.
    decodeCommand = new ShaderDecodeCommand(END_THREAD, threadID, PC);

    //  Check if a dynamic instruction object was supplied.
    if (shExecInstr != NULL)
    {
        //  Copy cookies from original instruction and add a new cookie.
        decodeCommand->copyParentCookies(*shExecInstr);
        decodeCommand->addCookie();
    }

    //  Send control command to fetch.  
    controlSignal->write(cycle, decodeCommand) ;

    GPU_DEBUG_BOX(
        printf("%s => Sending END_THREAD vectorThreadID = %d PC = %06x.\n", getName(), threadID, PC);
    )

    //  Update statistics.
    ends->inc();
}

//  Inform the fetch stage that a vector thread has executed a z export instruction.
void cmoShaderDecExeVector::zExportThread(U64 cycle, U32 threadID, U32 PC, ShaderExecInstruction *shExecInstr)
{
    ShaderDecodeCommand *decodeCommand;

    //  Construct the z export executed command.
    decodeCommand = new ShaderDecodeCommand(ZEXPORT_THREAD, threadID, PC);

    //  Check if a dynamic instruction object was supplied.
    if (shExecInstr != NULL)
    {
        //  Copy cookies from original instruction and add a new cookie.
        decodeCommand->copyParentCookies(*shExecInstr);
        decodeCommand->addCookie();
    }

    //  Send control command to fetch.  
    controlSignal->write(cycle, decodeCommand) ;

    GPU_DEBUG_BOX(
        printf("%s => Sending ZEXPORT_THREAD vectorThreadID = %d PC = %06x.\n", getName(), threadID, PC);
    )

    //  Update statistics.
    zexports->inc();
}

//  Make fetch block a thread.
void cmoShaderDecExeVector::blockThread(U64 cycle, U32 threadID, U32 PC, ShaderExecInstruction *shExecInstr)
{
    ShaderDecodeCommand *decodeCommand;

    //  Check if thread was in ready state.
    if (threadInfo[threadID].ready)
    {
        //  Set thread as blocked.
        threadInfo[threadID].ready = false;

        //  Ask the Shader Fetch to block the thread.
        decodeCommand = new ShaderDecodeCommand(BLOCK_THREAD, threadID, PC);

        //  Copy cookies from original instruction and add a new cookie.
        decodeCommand->copyParentCookies(*shExecInstr);
        decodeCommand->addCookie();

        //  Send control command to fetch.
        controlSignal->write(cycle, decodeCommand) ;

        GPU_DEBUG_BOX(
            printf("%s => Sending BLOCK_THREAD vectorThreadID = %d PC = %06x.\n", getName(), threadID, PC);
        )

        //  Update statistics.
        blocks->inc();
    }
}

//  Unblock a thread in the fetch stage.
void cmoShaderDecExeVector::unblockThread(U64 cycle, U32 threadID, U32 PC, ShaderExecInstruction *shExecInstr)
{
    ShaderDecodeCommand *decodeCommand;

    //  Check if the thread is actually blocked.
    if (!threadInfo[threadID].ready)
    {
        //  Set thread as blocked.
        threadInfo[threadID].ready = true;

        //  Ask the Shader Fetch to unblock the thread.
        decodeCommand = new ShaderDecodeCommand(UNBLOCK_THREAD, threadID, PC);

        //  Check if a dynamic instruction object was supplied.
        if (shExecInstr != NULL)
        {
            //  Copy cookies from original instruction and add a new cookie.
            decodeCommand->copyParentCookies(*shExecInstr);
            decodeCommand->addCookie();
        }

        //  Send control command to fetch.
        controlSignal->write(cycle, decodeCommand) ;

        GPU_DEBUG_BOX(
            printf("%s => Sending UNBLOCK_THREAD vectorThreadID = %d PC = %06x.\n", getName(), threadID, PC);
        )

        //  Update statistics.
        unblocks->inc();
    }
    else
    {
        CG_ASSERT("Unblocking ready thread.");
    }
}


//  Processes a command from the Command Processor unit.
void cmoShaderDecExeVector::processCommand(ShaderCommand *shCom)
{
    switch(shCom->getCommandType())
    {
        case RESET:

            GPU_DEBUG_BOX(
                printf("%s => RESET command received.\n", getName());
            )

            //  Change shader decode execute to reset state.
            state = SH_RESET;

            break;

        default:
            CG_ASSERT("Unsupported shader command from the Command Processor.");
            break;
    }
}

//  Processes the end of the execution of shader instructions (write back/execution pipeline end stage).
void cmoShaderDecExeVector::writeBackStage(U64 cycle)
{
    ShaderExecInstruction *shExecInstr;
    cgoShaderInstr *shInstr;
    cgoShaderInstr::cgoShaderInstrEncoding *shInstrDec;
    U32 shEmuElemID;
    U32 threadID;
    U32 pc = 0;
    char dis[256];

    while(executionEndSignal->read(cycle, (DynamicObject *&) shExecInstr))
    {
        //  Get the shader instruction.
        shInstrDec = &shExecInstr->getShaderInstruction();
        shInstr = shInstrDec->getShaderInstruction();

        //  Get the shader behaviorModel element identifier.
        shEmuElemID = shInstrDec->getNumThread();

        //  Retrieve the thread identifier from the shader behaviorModel element identifier.
        threadID = shEmuElemID / vectorLength;

        //  Check for the end of the first element in a vector.  Updates to the decode/execute stage state are only done for this first element.
        if (GPU_MOD(shEmuElemID, vectorLength) == 0)
        {
            //  Get shader thread PC after execution of the instruction.
            pc = bmUnifiedShader.GetThreadPC(shEmuElemID);

            GPU_DEBUG_BOX
            (
                shInstr->disassemble(&dis[0], MAX_INFO_SIZE);
                printf("%s => End Execution vectorThreadID = %d PC = %06x : %s\n", getName(), threadID, pc, dis);
            )

            //  Emulate the instruction at the end of their execution latency.  NOTE: NOW IT IS DONE AT THE START OF THE EXEC LATENCY!!!
            //bmUnifiedShader.execShaderInstruction(shInstr);

            GPU_ASSERT(
                if (threadInfo[threadID].pendingInstructions == 0)
                    printf("cmoShaderDecExeVector", "writeBackStage", "Pending instructions was already 0.");
            )

            //  Update number of pending instructions for the thread.
            threadInfo[threadID].pendingInstructions--;


            //  Check for CG1_ISA_OPCODE_KILL instruction.

            //
            //  EARLY TERMINATION OF SHADER PROGRAM THAT HAS BEEN CG1_ISA_OPCODE_KILLED IS NOT IMPLEMENTED.
            //

            //  Check for Z exported condition.
            if (threadInfo[threadID].zexport)
            {
                //  Send Z EXPORT command to Fetch.
                zExportThread(cycle, threadID, pc, shExecInstr);

                //  Remove Z exported condition.
                threadInfo[threadID].zexport = false;
            }

            //  Clear write dependences.
            clearDependences(cycle, shInstr, threadID);
        }

        //  Determine if all the shader instruction have finished.  Wait until the end of the vector to send the CG1_ISA_OPCODE_END_TRHEAD signal to fetch.
        if (threadInfo[threadID].end && !threadInfo[threadID].waitTexture && (threadInfo[threadID].pendingInstructions == 0) &&
            (GPU_MOD(shEmuElemID, vectorLength) == (vectorLength - 1)))
        {
            //  Remove thread end condition.
            threadInfo[threadID].end = false;

            //  Remove thread block condition.
            threadInfo[threadID].ready = true;

            //  Send CG1_ISA_OPCODE_END signal to Fetch.
            endThread(cycle, threadID, pc, shExecInstr);
        }
            

        //  Check for the last element in the vector.  Jump instructions are executed at the end of the vector.
        if (shInstr->isAJump() && (GPU_MOD(shEmuElemID, vectorLength) == (vectorLength - 1)))
        {
            bool jump;
            U32 destinationPC;
            
            //  Execute the jump instruction.
            jump = bmUnifiedShader.checkJump(shInstrDec, vectorLength, destinationPC);
            
            GPU_DEBUG_BOX(
                printf("%s => CG1_ISA_OPCODE_JMP instruction executed.  Jump Taken? = %s Target PC = %06x\n", getName(), jump? "Yes": "No", destinationPC);
            )
            
            //  Update the PC in the fetch stage.
            changePC(cycle, threadID, destinationPC, shExecInstr);
            
            //  Unblock the thread.
            unblockThread(cycle, threadID, pc, shExecInstr);                

            //  Unset the pending jump flag for the thread.
            threadInfo[threadID].pendingJump = false;
        }
        
        //  Update statistics.
        execInstr->inc();

        //  Check if the shader instruction isn't a texture load.
        if (!shInstr->isALoad())
        {
            //  Delete the decoded shader instruction.
            delete shInstrDec;
        }

        //  Delete the Shader Execution Instruction.
        delete shExecInstr;
    }
}

//  Starts the execution of a shader instruction.
void cmoShaderDecExeVector::startExecution(U64 cycle, U32 element, ShaderExecInstruction *shExecInstr)
{
    cgoShaderInstr *shInstr;
    cgoShaderInstr::cgoShaderInstrEncoding *shInstrDec;
    U32 instrLat;
    U32 shEmuElemID;
    U32 threadID;
    U32 pc;
    char dis[256];

    //  Get shader instruction.
    shInstrDec = &shExecInstr->getShaderInstruction();
    shInstr = shInstrDec->getShaderInstruction();

    //  The latency is variable (per opcode).  The latency table stores the execution latency
    //  for each instruction opcode.  One cycle is added to simulate the write back stage.

    //  Get instruction latency.
    instrLat = shArchParams->getExecutionLatency(shInstr->getOpcode()) + 1;

    CG_ASSERT_COND(!(instrLat > MAX_EXEC_LAT), "Instruction execution latency larger than maximum execution latency.");
    //  Check for the first element in the vector.
    if ((element % vectorALUWidth) == 0)
    {
        GPU_DEBUG_BOX(
            
            //  Get the shader behaviorModel element identifier.
            shEmuElemID = shInstrDec->getNumThread();

            //  Retrieve the vector thread identifier from the shader behaviorModel element identifier.
            threadID = shEmuElemID / vectorLength;

            //  Get shader thread PC.
            pc = shInstrDec->getPC();
            
            shInstr->disassemble(&dis[0], MAX_INFO_SIZE);

//if (cycle > 71000)
            printf("%s => Launching instruction vectorThreadID = %d PC = %06x (Lat = %d) : %s\n",
                getName(), threadID, pc, instrLat - 1, dis);
        )
    }

    //  Start instruction execution.
    executionStartSignal->write(cycle, shExecInstr, instrLat);

    //  Check if the execution of the instruction must be traced.
    if (shExecInstr->getTraceExecution())
    {
        //  Get the shader behaviorModel element identifier.
        shEmuElemID = shInstrDec->getNumThread();

        //  Retrieve the vector thread identifier from the shader behaviorModel element identifier.
        threadID = shEmuElemID / vectorLength;

        //  Get shader thread PC.
        pc = shInstrDec->getPC();
        
        shInstr->disassemble(&dis[0], MAX_INFO_SIZE);

        printf("%s (%lld) => Launching instruction vectorThreadID = %d PC = %06x (Lat = %d) : %s\n",
            getName(), cycle, threadID, pc, instrLat - 1, dis);
            
        //  Print the operands for the instruction.
        printShaderInstructionOperands(shInstrDec);                
    }
    
    //  Emulate the instruction at the start of their execution latency.
    bmUnifiedShader.execShaderInstruction(shInstrDec);

    //  Check if the execution of the instruction must be traced.
    if (shExecInstr->getTraceExecution())
    {
        //  Print the results from the instruction.
        printShaderInstructionResult(shInstrDec);
        
        //  Print the kill mask after the instruction.
        printf("             CG1_ISA_OPCODE_KILL MASK -> %s\n", bmUnifiedShader.threadKill(shInstrDec->getNumThread()) ? "true" : "false");
    }
}

//  Update decode stage structures (dependence checking, write port limitations).
void cmoShaderDecExeVector::updateDecodeStage(U64 cycle, ShaderExecInstruction *shExecInstr)
{
    cgoShaderInstr *shInstr;
    cgoShaderInstr::cgoShaderInstrEncoding *shInstrDec;
    U32 instrLat;
    U32 repeatRate;
    U32 resReg;
    U32 shEmuElemID;
    U32 threadID;
    U32 pc;
    char dis[256];

    //  Get shader instruction.
    shInstrDec = &shExecInstr->getShaderInstruction();
    shInstr = shInstrDec->getShaderInstruction();

    //  The latency is variable (per opcode).  The latency table stores the execution latency
    //  for each instruction opcode.  One cycle is added to simulate the write back stage.

    //  Get instruction latency.
    instrLat = shArchParams->getExecutionLatency(shInstr->getOpcode()) + 1;

    //  Get the instruction repeat rate.
    repeatRate = shArchParams->getRepeatRate(shInstr->getOpcode());
    
    //  Check that the instruction latency and repeat rates are defined for the instruction.
    GPU_ASSERT(
    
        char buffer[256];        
        if (instrLat == 1)
        {
            sprintf(buffer, "Latency for instruction opcode %x not defined.", shInstr->getOpcode());
            CG_ASSERT(buffer);
        }
        if (repeatRate == 0)
        {
            sprintf(buffer, "Repeat rate for instruction opcode %x not defined.", shInstr->getOpcode());
            CG_ASSERT(buffer);
        }
    )
    
    //  Get the shader behaviorModel element identifier.
    shEmuElemID = shInstrDec->getNumThread();

    //  Retrieve the vector thread identifier from the shader behaviorModel element identifier.
    threadID = shEmuElemID / vectorLength;

    //  Get shader thread PC.
    pc = shInstrDec->getPC();

    //  Check if thread is blocked.  This happens for instructions starting after a texture instruction in the
    //  same cycle.
    //if (!threadInfo[numThread].ready)
    //{
        //  Ignore any instruction from a blocked thread.

    //    GPU_DEBUG_BOX(
    //        printf("cmoShaderDecExeVector => Ignoring instruction from blocked thread.\n");
    //    )
    //    return;

    //  Get result register number.
    resReg = shInstr->getResult();

    //  For CG1_ISA_OPCODE_END instruction block the vector thread so no more instructions are accepted until the program is finished.
    if (shInstr->isEnd())
    {
        //  Set CG1_ISA_OPCODE_END received flag for the thread.
        threadInfo[threadID].end = true;

        //  Send a block signal to fetch.
        blockThread(cycle, threadID, pc, shExecInstr);
    }

    //  Check if the instruction is a Z export operation.
    if (shInstr->isZExport())
    {
        //  Set z export flag for the thread.
        threadInfo[threadID].zexport = true;
    }

    //  Check if the instruction is a texture operation.
    if (shInstr->isALoad())
    {
        //  Set thread as waiting for texture result.
        threadInfo[threadID].waitTexture = true;

        //  Consume texture tickets for all the elements in the vector.
        textureTickets[nextRequestTU] -= vectorLength;
        
        //  Check if automatic blocking on texture load is enabled.
        if (!explicitBlock)
        {
            //  Reset the counter of pending texture fetches for elements in the vector.
            threadInfo[threadID].pendingTexElements = vectorLength;

            //  Send a block signal to fetch.
            blockThread(cycle, threadID, pc, shExecInstr);
        }
        else
        {
            //  Increment the counter of pending texture fetches.
            threadInfo[threadID].pendingTexElements += vectorLength;
        }
    }
    
    //  Check if the instruction is a jump.
    if (shInstr->isAJump())
    {
        //  Set the thread pending jump flag.
        threadInfo[threadID].pendingJump = true;

        //  Send a block signal to fetch.
        blockThread(cycle, threadID, pc, shExecInstr);
    }

    //  Check for explicit blocks if enabled and the instruction is a wait point.
    if (explicitBlock && shInstr->isWaitPoint() && (threadInfo[threadID].pendingTexElements > 0))
    {
        //  Block the thread and report to fetch stage.
        blockThread(cycle, threadID, pc, shExecInstr);
    }

    //  Check if it is an instruction with a result.
    if (shInstr->hasResult())
    {
        //  Set pending register write tables.
        switch(shInstr->getBankRes())
        {
                case cg1gpu::ADDR:

                    //  Set pending status for address register.
                    threadInfo[threadID].addrBankDep[resReg] = true;

                    break;

                case cg1gpu::TEMP:

                    //  Set pending status for temporary register.
                    threadInfo[threadID].tempBankDep[resReg] = true;

                    break;

                case cg1gpu::PRED:

                    //  Set pending status for predicate register.
                    threadInfo[threadID].predBankDep[resReg] = true;
                    
                    break;

                case cg1gpu::OUT:

                    //  Set pending status for output register.
                    threadInfo[threadID].outpBankDep[resReg] = true;

                    break;

                default:

                    CG_ASSERT("Unsupported bank for register writes.");
                    break;
        }
    }

    GPU_DEBUG_BOX(
        shInstr->disassemble(&dis[0], MAX_INFO_SIZE);

//if (cycle > 71000)
        printf("%s => Execute instruction vectorThreadID = %d PC = %06x (Lat = %d) : %s\n",
            getName(), threadID, pc, instrLat - 1, dis);
    )

    //  Update on execution instruction counter for the thread.
    threadInfo[threadID].pendingInstructions++;

    //  Check if it is an instruction with a result.
    if (shInstr->hasResult())
    {
        //  Set register write cycle tables.
        switch(shInstr->getBankRes())
        {
            case cg1gpu::OUT:

                //  If current write latency for the register is less than the instruction latency update the table.
                if (threadInfo[threadID].outpBankWrite[resReg] < (cycle + instrLat))
                {
                    threadInfo[threadID].outpBankWrite[resReg] = cycle + instrLat;
                }
                else
                    CG_ASSERT("WAW dependence for output bank.");

                break;

            case cg1gpu::ADDR:

                //  If current write latency for the register is less than the instruction latency update the table.
                if (threadInfo[threadID].addrBankWrite[resReg] < (cycle + instrLat))
                {
                    threadInfo[threadID].addrBankWrite[resReg] = cycle + instrLat;
                }
                else
                    CG_ASSERT("WAW dependence for address bank.");

                break;

            case cg1gpu::TEMP:

                //  If current write latency for the register is less than the instruction latency update the table.
                if (threadInfo[threadID].tempBankWrite[resReg] < (cycle + instrLat))
                {
                    threadInfo[threadID].tempBankWrite[resReg] = cycle + instrLat;
                }
                else
                    CG_ASSERT("WAW dependence for temporary bank.");

                break;

            case cg1gpu::PRED:

                //  If current write latency for the register is less than the instruction latency update the table.
                if (threadInfo[threadID].predBankWrite[resReg] < (cycle + instrLat))
                {
                    threadInfo[threadID].predBankWrite[resReg] = cycle + instrLat;
                }
                else
                    CG_ASSERT("WAW dependence for predicate bank.");

                break;

            default:
                CG_ASSERT("Unsupporte write bank.");
                break;
        }
    }

    //
    //  NOTE:  AS THE WRITE BW IS LIMITED BY THE EXECUTION SIGNAL BW INSTRUCTIONS THAT DON'T
    //  WRITE A RESULT REGISTER ARE STILL AFFECTED BY THE REGISTER WRITE PORT LIMITATIONS IN
    //  THE CURRENT IMPLEMENTATION.
    //

    //  Update register writes table.
    regWrites[GPU_MOD(nextRegWrite + instrLat, MAX_EXEC_LAT)]++;

    //  Update the repeat rate for the current vector fetch.
    currentRepeatRate = GPU_MAX(currentRepeatRate, repeatRate);
}

//  Blocks a shader instruction and sends replay command to shader fetch.
void cmoShaderDecExeVector::repeatInstruction(U64 cycle, U32 instruction)
{
    ShaderExecInstruction *shExecInstr;
    cgoShaderInstr *shInstr;
    cgoShaderInstr::cgoShaderInstrEncoding *shInstrDec;
    U32 shEmuElemID;
    U32 threadID;
    U32 pc;
    char dis[256];

    //  Use the vector instruction first element shader behaviorModel instruction.
    shExecInstr = vectorFetch[instruction][0];

    //  Get the shader behaviorModel instruction.
    shInstrDec = &shExecInstr->getShaderInstruction();
    shInstr = shInstrDec->getShaderInstruction();

    //  Get the shader behaviorModel element identifier.
    shEmuElemID = shInstrDec->getNumThread();

    //  Retrieve the vector shader identifier from the shader behaviorModel identifier.
    threadID = shEmuElemID / vectorLength;

    //  Check if the thread identifier is correct.
    GPU_ASSERT(
        if (threadID >= vectorThreads)
        {
            CG_ASSERT("Vector thread identifier is out of range.");
        }
    )

    //  Get instruction PC.
    pc = shInstrDec->getPC();

    GPU_DEBUG_BOX(
        shInstr->disassemble(&dis[0], MAX_INFO_SIZE);

        printf("%s => Instruction stalled (repeat) for vectorThreadID = %d  PC = %06x : %s \n",
            getName(), threadID, pc, dis);
    )

    //  Ask fetch to replay the instruction.
    replayInstruction(cycle, threadID, pc, shExecInstr);
}

//  Decodes a shader instruction.
void cmoShaderDecExeVector::decodeInstruction(U64 cycle, U32 instruction, bool &execute, bool &stall)
{
    cgoShaderInstr *shInstr;
    cgoShaderInstr::cgoShaderInstrEncoding *shInstrDec;
    U32 shEmuElementID;
    U32 threadID;
    U32 pc;
    U32 instrLat;
    U32 resReg;
    char dis[256];

    //  Use the data from the first element shader behaviorModel instruction for the decoding.
    //  There is a single vector instruction but the shader behaviorModel instruction is replicated per element due to
    //  compatibility with the original Shader Behavior Model.
    shInstrDec = &vectorFetch[instruction][0]->getShaderInstruction();
    shInstr = shInstrDec->getShaderInstruction();

    //  Get the shader behaviorModel element identifier from the shader behaviorModel instruction.
    shEmuElementID = shInstrDec->getNumThread();

    //  Retrieve the vector thread identifier form the element identifier.
    threadID = shEmuElementID / vectorLength;

    //  Check if the vector thread identifier is correct.
    GPU_ASSERT(
        if (threadID >= vectorThreads)
        {
            CG_ASSERT("Vector thread identifier is out of range.");
        }
    )

    //  Get instruction PC.
    pc = shInstrDec->getPC();

    GPU_DEBUG_BOX(
        shInstr->disassemble(&dis[0], MAX_INFO_SIZE);
//if (cycle > 71000)
        printf("%s (%lld) => Decode Instr. Vector Thread = %d  PC = %06x : %s \n",
            getName(), cycle, threadID, pc, dis);
    )

    //  Initially mark the instruction as not executable and not stalled (dropable).
    execute = false;
    stall = false;

    //  Check branch dependencies.  Branch/call/ret must finish before a new instruction can be executed.

    //  **** NOT IMPLEMENTED ****

    //  Check if an CG1_ISA_OPCODE_END instruction was decoded.  Ignore any further instructions different from CG1_ISA_OPCODE_END.
    if (threadInfo[threadID].end)
    {
        //  Ignore any instruction after a thread receives CG1_ISA_OPCODE_END.

        GPU_DEBUG_BOX(
            printf("%s => Ignoring instruction after CG1_ISA_OPCODE_END.\n", getName());
        )

        return;
    }

    //  Check if a CG1_ISA_OPCODE_JMP instruction is pending.  Ignore further instructions until the CG1_ISA_OPCODE_JMP is executed.
    if (threadInfo[threadID].pendingJump)
    {
        //  Ignore any instruction until the pending jump is executed.

        GPU_DEBUG_BOX(
            printf("%s => Ignoring instruction after CG1_ISA_OPCODE_JMP.\n", getName());
        )

        return;
    }
    
    //  Check if thread is blocked.
    if (!threadInfo[threadID].ready)
    {
        //  Ignore any instruction from a blocked thread.

        GPU_DEBUG_BOX(
            printf("%s => Ignoring instruction from blocked thread.\n", getName());
        )

        return;
    }

    //  Check if there are enough texture tickets for the whole texture instruction.
    if (shInstr->isALoad() && (textureTickets[nextRequestTU] < vectorLength))
    {
        GPU_DEBUG_BOX(
            printf("%s => No texture ticket available for the texture instruction.\n", getName());
        )

        //  The instruction must be stalled.
        stall = true;

        return;
    }


    //  Check if SIMD + scalar mode is enabled.
    if (scalarALU)
    {
        /*
            NOTE: REMOVED CHECK.  The simd4+scalar configuration allows for two scalar instructions to be issued.
            One takes the scalar slot and the other the simd4 slot.
           
        //  Only allow one scalar op per cycle and thread rate.
        if (shInstr->isScalar() && reserveScalarALU)
        {
            GPU_DEBUG_BOX(
                printf("cmoShaderDecExeVector => No more ALU ops allowed in current cycle.\n");
            )

            //  The instruction must be stalled.
            stall = true;

            return;
        }*/

        //  Only allow one SIMD op per cycle and thread rate.
        if ((!shInstr->isScalar()) && reserveSIMDALU)
        {
            GPU_DEBUG_BOX(
                printf("%s => No more SIMD ops allowed in current cycle.\n", getName());
            )

            //  The instruction must be stalled.
            stall = true;

            return;
        }
    }

    //  Check register dependence table.

    //  Check RAW dependences for first source operand.
    if (shInstr->getNumOperands() > 0)
    {
        //  Check RAW dependence.
        if (checkRAWDependence(threadID, shInstr->getBankOp1(), shInstr->getOp1()))
        {
            GPU_DEBUG_BOX(
                printf("%s => RAW dependence found for first operand.\n", getName());
            )

            //  The instruction must be stalled.
            stall = true;

            return;
        }
    }


    //  Check RAW dependences for second source operand.
    if (shInstr->getNumOperands() > 1)
    {
        //  Check RAW dependence.  */
        if (checkRAWDependence(threadID, shInstr->getBankOp2(), shInstr->getOp2()))
        {
            GPU_DEBUG_BOX(
                printf("%s => RAW dependence found for second operand.\n", getName());
            )

            //  The instruction must be stalled.
            stall = true;

            return;
        }
    }

    //  Check RAW dependences for third source operand.
    if (shInstr->getNumOperands() > 2)
    {
        //  Check RAW dependence.
        if (checkRAWDependence(threadID, shInstr->getBankOp3(), shInstr->getOp3()))
        {
            GPU_DEBUG_BOX(
                printf("%s => RAW dependence found for third operand.\n", getName());
            )

            //  The instruction must be stalled.
            stall = true;

            return;
        }
    }

    //  Check RAW dependences with the address register used in relative mode access to the constant bank.
    if (shInstr->getRelativeModeFlag())
    {
        //  Check RAW dependence.
        if (checkRAWDependence(threadID, ADDR, shInstr->getRelMAddrReg()))
        {
            GPU_DEBUG_BOX(
                printf("%s => RAW dependence for address register %d in relative access mode to the constant bank.\n",
                    getName(), shInstr->getRelMAddrReg());
            )

            //  The instruction must be stalled.
            stall = true;

            return;
        }
    }

    //  Get instruction latency.
    instrLat = shArchParams->getExecutionLatency(shInstr->getOpcode()) + 1;

    //  Check if it is an instruction with a result.
    if (shInstr->hasResult())
    {
        //  Get Result register.
        resReg = shInstr->getResult();

        //  Check WAW dependences:
        //    If the current write latency for the result register of the instruction is
        //    larger than the instruction execution latency the execution of the instruction
        //    must wait.
        switch(shInstr->getBankRes())
        {
            case cg1gpu::OUT:

                //  Check WAW dependence for output bank register.
                if (threadInfo[threadID].outpBankWrite[resReg] >= (cycle + instrLat))
                {
                    GPU_DEBUG_BOX(
                        printf("%s => WAW dependence for OUTPUT register %d.\n", getName(), resReg);
                    )

                    //  The instruction must be stalled.
                    stall = true;

                    return;
                }

                break;

            case cg1gpu::ADDR:

                //  Check WAW dependence for address bank register.
                if (threadInfo[threadID].addrBankWrite[resReg] >= (cycle + instrLat))
                {
                    GPU_DEBUG_BOX(
                        printf("%s => WAW dependence for ADDRESS register %d.\n", getName(), resReg);
                    )

                    //  The instruction must be stalled.
                    stall = true;

                    return;
                }

                break;

            case cg1gpu::TEMP:

                //  Check WAW dependence for temporal bank register.
                if (threadInfo[threadID].tempBankWrite[resReg] >= (cycle + instrLat))
                {
                    GPU_DEBUG_BOX(
                        printf("%s => WAW dependence for TEMPORAL register %d.\n", getName(), resReg);
                    )

                    //  The instruction must be stalled.
                    stall = true;

                    return;
                }

                break;

            case cg1gpu::PRED:

                //  Check WAW dependence for predicate bank register.
                if (threadInfo[threadID].predBankWrite[resReg] >= (cycle + instrLat))
                {
                    GPU_DEBUG_BOX(
                        printf("%s => WAW dependence for PREDICATE register %d.\n", getName(), resReg);
                    )

                    //  The instruction must be stalled.
                    stall = true;

                    return;
                }

                break;
        }
    }

    //
    //
    //  NOTE:  AS THE WRITE BW IS LIMITED BY THE EXECUTION SIGNAL BW INSTRUCTIONS THAT DON'T
    //  WRITE A RESULT REGISTER ARE STILL AFFECTED BY THE REGISTER WRITE PORT LIMITATIONS IN
    //  THE CURRENT IMPLEMENTATION.
    //
    //

    //  Check register write ports availability.
    if (regWrites[GPU_MOD(nextRegWrite + instrLat, MAX_EXEC_LAT)] >= (MAX_EXEC_BW * instrCycle))
    {
       GPU_DEBUG_BOX(
            printf("%s => Register write port limit reached.\n", getName());
       )

        //  The instruction must be stalled.
        stall = true;

        return;
    }

    //  Instruction passes all dependences and conditions.  Allow execution.
    execute = true;
    
    //  Update decode stage structures (for dependence checking, write port checking, etc).
    updateDecodeStage(cycle, vectorFetch[instruction][0]);
}


//  Determines if there is a RAW dependence for a register.
bool cmoShaderDecExeVector::checkRAWDependence(U32 threadID, Bank bank, U32 reg)
{
    bool dependenceFound;
    
    switch(bank)
    {
        case TEMP:
        
            //  Check if there are pending writes for the temporary register. 
            dependenceFound = threadInfo[threadID].tempBankDep[reg];
            break;
      
        case ADDR:
        
            //  Check if there are pending writes for the address register. 
            dependenceFound = threadInfo[threadID].addrBankDep[reg];            
            break;
            
        case PRED:
        
            //  Check if there are pending writes for the predicate register. 
            dependenceFound = threadInfo[threadID].predBankDep[reg];
            break;
            
        default:

            //  All other types of register are not writeable.
            dependenceFound = false;
            break;
    }

    return dependenceFound;
}

//  Clears register write dependences.
void cmoShaderDecExeVector::clearDependences(U64 cycle, cgoShaderInstr *shInstr, U32 threadID)
{
    //  Update pending registers write tables.

    //  Check if the instruction writes a register.
    if (shInstr->hasResult())
    {
        //  Get the result register.
        U32 resReg = shInstr->getResult();
        
        //  Determine register bank written by the instruction.
        switch(shInstr->getBankRes())
        {
            case cg1gpu::ADDR:

                //  Check if clearing the last write pending for the address register.
                if (threadInfo[threadID].addrBankWrite[resReg] == cycle)
                {
                    //  Remove pending status for the address register.
                    threadInfo[threadID].addrBankDep[resReg] = false;
                }
                
                break;

            case cg1gpu::TEMP:

                //  Check if clearing the last write pending for the temporary register.
                if (threadInfo[threadID].tempBankWrite[resReg] == cycle)
                {
                    //  Remove pending status for the temporary register.
                    threadInfo[threadID].tempBankDep[resReg] = false;
                }
                
                break;

            case cg1gpu::PRED:

                //  Check if clearing the last write pending for the predicate register.
                if (threadInfo[threadID].predBankWrite[resReg] == cycle)
                {
                    //  Remove pending status for the predicate register.
                    threadInfo[threadID].predBankDep[resReg] = false;
                }
                
                break;

            case cg1gpu::OUT:

                //  Check if clearing the last write pending for the output register.
                if (threadInfo[threadID].outpBankWrite[resReg] == cycle)
                {
                    //  Remove pending status for the output register.
                    threadInfo[threadID].outpBankDep[resReg] = false;
                }
                
                break;

            default:

                CG_ASSERT("Unsupported bank for register writes.");
                break;
        }
    }
}

//  Resets the Vector Shader decode and execute state.
void cmoShaderDecExeVector::reset()
{
    //  Reset register writes table.
    for(U32 i = 0; i < MAX_EXEC_LAT; i++)
        regWrites[i] = 0;

    //  Reset pointer to the current cycle entry in the register write table.
    nextRegWrite = 0;

    //  Reset decode execute state.
    for(U32 i = 0; i < vectorThreads; i ++)
    {
        //  Reset thread state.
        threadInfo[i].pendingInstructions = 0;
        threadInfo[i].pendingTexElements = 0;
        threadInfo[i].ready = true;
        threadInfo[i].waitTexture = false;
        threadInfo[i].end = false;
        threadInfo[i].pendingJump = false;
        threadInfo[i].zexport = false;

        //  Reset per thread register dependence tables.
        for(U32 j = 0; j < MAX_TEMP_BANK_REGS; j++)
        {
            threadInfo[i].tempBankDep[j] = false;
            threadInfo[i].tempBankWrite[j] = 0;
        }
        for(U32 j = 0; j < MAX_ADDR_BANK_REGS; j++)
        {
            threadInfo[i].addrBankDep[j] = false;
            threadInfo[i].addrBankWrite[j] = 0;
        }
        for(U32 j = 0; j < MAX_PRED_BANK_REGS; j++)
        {
            threadInfo[i].predBankDep[j] = false;
            threadInfo[i].predBankWrite[j] = 0;
        }
        for(U32 j = 0; j < MAX_OUTP_BANK_REGS; j++)
        {
            threadInfo[i].outpBankDep[j] = false;
            threadInfo[i].outpBankWrite[j] = 0;
        }
    }

    //  Reset the queue with identifier of threads pending to be awaken
    //  from Texture Unit blocks.
    threadWakeUpQ.clear();
    
    //  Reset number of texture tickets.
    for(U32 i = 0; i < textureUnits; i++)
        textureTickets[i] = 0;

    //  Reset pointer to the next texture unit to which send requests.
    nextRequestTU = 0;

    //  Reset number of requests send to the current texture unit.
    tuRequests = 0;

    //  Reset the counter of cycles until the next fetch.
    cyclesToNextFetch = 0;

    //  Reset flag that stores if a vector fetch is available.
    vectorFetchAvailable = false;

    //  Clear executing vector instruction flag.
    executingVector = false;       
    
    //  Reset the counter of cycles until the next shader elements can start execution (related with instruction repeat rate).
    cyclesToNextExec = 0;
}

//  Receive a vector instruction from the Vector Shader fetch stage.
void cmoShaderDecExeVector::receiveVectorInstruction(U64 cycle)
{
    VectorInstructionFetch *vectorInstruction;

    //  Update the counter for the cycles expected until the next vector fetch.
    if (cyclesToNextFetch > 0)
    {
        cyclesToNextFetch--;
    }

    //  Check for a fetched instruction from fetch stage.
    if (instructionSignal->read(cycle, (DynamicObject *&) vectorInstruction))
    {
         CG_ASSERT_COND(!(cyclesToNextFetch > 0), "Vector instruction fetch not expected in this cycle.");
        //  A vector fetch is available.
        vectorFetchAvailable = true;

        //  Check that the previous vector instruction was freed.
        CG_ASSERT_COND(!(vectorFetch[0] != NULL), "Array of pointers for a vector instruction was not liberated.");
        //  Store first vector instruction.
        vectorFetch[0] = vectorInstruction->getVectorFetch();

        //  Delete vector instruction fetch carrier object.
        delete vectorInstruction;

        //  Receive the rest of instructions from the vector shader fetch stage.
        for(U32 instruction = 1; instruction < instrCycle; instruction++)
        {
            //  Receive the next vector instruction from fetch stage.
            if (instructionSignal->read(cycle, (DynamicObject *&) vectorInstruction))
            {
                //  Check that the previous vector instruction was freed.
                CG_ASSERT_COND(!(vectorFetch[instruction] != NULL), "Array of pointers for a vector instruction was not liberated.");
                //  Store instruction in the array vector instructions.
                vectorFetch[instruction] = vectorInstruction->getVectorFetch();

                //  Delete vector instruction fetch carrier object.
                delete vectorInstruction;
            }
            else
            {
                CG_ASSERT("Expecting more instructions for vector.");
            }
        }
    }
}

void cmoShaderDecExeVector::printShaderInstructionOperands(cgoShaderInstr::cgoShaderInstrEncoding *shDecInstr)
{
    if (shDecInstr->getShaderInstruction()->getNumOperands() > 0)
    {
        switch(shDecInstr->getShaderInstruction()->getBankOp1())
        {
            case cg1gpu::TEXT:            
                break;
          
            case cg1gpu::PRED:
                {
                    bool *op1 = (bool *) shDecInstr->getShEmulOp1();
                    printf("             OP1 -> %s\n", *op1 ? "true" : "false");
                }
                break;

            default:
            
                //  Check for predicator operator instruction with constant register operator.
                switch(shDecInstr->getShaderInstruction()->getOpcode())
                {
                    case cg1gpu::CG1_ISA_OPCODE_ANDP:
                        {
                            bool *op1 = (bool *) shDecInstr->getShEmulOp1();
                            printf("             OP1 -> {%s, %s, %s, %s}\n", op1[0] ? "true" : "false", op1[1] ? "true" : "false",
                                op1[2] ? "true" : "false", op1[3] ? "true" : "false");
                        }
                        break;
                      
                    case cg1gpu::CG1_ISA_OPCODE_ADDI:
                    case cg1gpu::CG1_ISA_OPCODE_MULI:
                    case cg1gpu::CG1_ISA_OPCODE_STPEQI:
                    case cg1gpu::CG1_ISA_OPCODE_STPGTI:
                    case cg1gpu::CG1_ISA_OPCODE_STPLTI:
                        {
                            S32 *op1 = (S32 *) shDecInstr->getShEmulOp1();
                            printf("             OP1 -> {%d, %d, %d, %d}\n", op1[0], op1[1], op1[2], op1[3]);
                        }                        
                        break;
                          
                    default:
                        {
                            F32 *op1 = (F32 *) shDecInstr->getShEmulOp1();
                            printf("             OP1 -> {%f, %f, %f, %f}\n", op1[0], op1[1], op1[2], op1[3]);
                        }                        
                        break;
                }
                break;                
        }
    }
    
    if (shDecInstr->getShaderInstruction()->getNumOperands() > 1)
    {
        switch(shDecInstr->getShaderInstruction()->getBankOp2())
        {
            case cg1gpu::TEXT:            
                break;
          
            case cg1gpu::PRED:
                {
                    bool *op2 = (bool *) shDecInstr->getShEmulOp2();
                    printf("             OP2 -> %s\n", *op2 ? "true" : "false");
                }
                break;
                
            default:
            
                //  Check for predicator operator instruction with constant register operator.
                switch(shDecInstr->getShaderInstruction()->getOpcode())
                {
                    case cg1gpu::CG1_ISA_OPCODE_ANDP:
                        {
                            bool *op2 = (bool *) shDecInstr->getShEmulOp2();
                            printf("             OP2 -> {%s, %s, %s, %s}\n", op2[0] ? "true" : "false", op2[1] ? "true" : "false",
                                op2[2] ? "true" : "false", op2[3] ? "true" : "false");
                        }
                        break;
                      
                    case cg1gpu::CG1_ISA_OPCODE_ADDI:
                    case cg1gpu::CG1_ISA_OPCODE_MULI:
                    case cg1gpu::CG1_ISA_OPCODE_STPEQI:
                    case cg1gpu::CG1_ISA_OPCODE_STPGTI:
                    case cg1gpu::CG1_ISA_OPCODE_STPLTI:
                        {
                            S32 *op2 = (S32 *) shDecInstr->getShEmulOp2();
                            printf("             OP2 -> {%d, %d, %d, %d}\n", op2[0], op2[1], op2[2], op2[3]);
                        }                        
                        break;
                          
                    default:
                        {
                            F32 *op2 = (F32 *) shDecInstr->getShEmulOp2();
                            printf("             OP2 -> {%f, %f, %f, %f}\n", op2[0], op2[1], op2[2], op2[3]);
                        }                        
                        break;
                }
                
                break;
        }
    }
    
    if (shDecInstr->getShaderInstruction()->getNumOperands() > 2)
    {
        switch(shDecInstr->getShaderInstruction()->getBankOp3())
        {
            case cg1gpu::TEXT:            
                break;
          
            case cg1gpu::PRED:
                {
                    bool *op3 = (bool *) shDecInstr->getShEmulOp3();
                    printf("             OP3 -> %s\n", *op3 ? "true" : "false");
                }
                break;
                
            default:
                //  Check for predicator operator instruction with constant register operator.
                switch(shDecInstr->getShaderInstruction()->getOpcode())
                {
                    case cg1gpu::CG1_ISA_OPCODE_ANDP:
                        {
                            bool *op3 = (bool *) shDecInstr->getShEmulOp3();
                            printf("             OP3 -> {%s, %s, %s, %s}\n", op3[0] ? "true" : "false", op3[1] ? "true" : "false",
                                op3[2] ? "true" : "false", op3[3] ? "true" : "false");
                        }
                        break;
                      
                    case cg1gpu::CG1_ISA_OPCODE_ADDI:
                    case cg1gpu::CG1_ISA_OPCODE_MULI:
                    case cg1gpu::CG1_ISA_OPCODE_STPEQI:
                    case cg1gpu::CG1_ISA_OPCODE_STPGTI:
                    case cg1gpu::CG1_ISA_OPCODE_STPLTI:
                        {
                            S32 *op3 = (S32 *) shDecInstr->getShEmulOp3();
                            printf("             OP3 -> {%d, %d, %d, %d}\n", op3[0], op3[1], op3[2], op3[3]);
                        }                        
                        break;
                          
                    default:
                        {
                            F32 *op3 = (F32 *) shDecInstr->getShEmulOp3();
                            printf("             OP3 -> {%f, %f, %f, %f}\n", op3[0], op3[1], op3[2], op3[3]);
                        }                        
                        break;
                }
                
                break;
        }
    }

    if (shDecInstr->getShaderInstruction()->getPredicatedFlag())
    {
        bool *pred = (bool *) shDecInstr->getShEmulPredicate();
        printf("             PREDICATE -> %s\n", *pred ? "true" : "false");
    }
}

void cmoShaderDecExeVector::printShaderInstructionResult(cgoShaderInstr::cgoShaderInstrEncoding *shDecInstr)
{
    if (shDecInstr->getShaderInstruction()->hasResult())
    {
        switch(shDecInstr->getShaderInstruction()->getBankRes())
        {
            case PRED:
                {
                    bool *res = (bool *) shDecInstr->getShEmulResult();                
                    printf("             RESULT -> %s\n", *res ? "true" : "false");
                }                
                break;

            default:
            
                switch(shDecInstr->getShaderInstruction()->getOpcode())
                {
                    case CG1_ISA_OPCODE_ADDI:
                    case CG1_ISA_OPCODE_MULI:
                        {
                            S32 *res = (S32 *) shDecInstr->getShEmulResult();                
                            printf("             RESULT -> {%d, %d, %d, %d}\n", res[0], res[1], res[2], res[3]);
                        }
                        break;
                        
                    default:
                        {
                            F32 *res = (F32 *) shDecInstr->getShEmulResult();                
                            printf("             RESULT -> {%f, %f, %f, %f}\n", res[0], res[1], res[2], res[3]);
                        }
                        break;
                }
                break;
        }
    }
}
