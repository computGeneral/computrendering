/**************************************************************************
 *
 * cmoStreamController class implementation file.
 *
 */

#include "cmStreamer.h"
#include "cmShaderStateInfo.h"
#include "cmStreamerStateInfo.h"
#include "cmRasterizerStateInfo.h"
#include "cmConsumerStateInfo.h"
#include "cmRasterizer.h"


using namespace cg1gpu;

//  Table with the size in bytes of each Stream data type.  
U32 cg1gpu::streamDataSize[] =
{
    1,      //  SD_UNORM8.  
    1,      //  SD_SNORM8.  
    2,      //  SD_UNORM16.  
    2,      //  SD_SNORM16.  
    4,      //  SD_UNORM32.  
    4,      //  SD_SNORM32.  
    2,      //  SD_FLOAT16.  
    4,      //  SD_FLOAT32.  
    1,      //  SD_UINT8.  
    1,      //  SD_SINT8.  
    2,      //  SD_UINT16.  
    2,      //  SD_SINT16.  
    4,      //  SD_UINT32.  
    4       //  SD_SINT32.  
};

//  cmoStreamController mdu constructor.  
cmoStreamController::cmoStreamController(U32 idxCycle, U32 idxBufferSz, U32 outFIFOSz, U32 outMemSz,
    U32 numSh, U32 shOutLat, U32 vertCycle, U32 paAttrCycle, U32 outLat,
    U32 sLoaderUnits, U32 slIdxCycle, U32 slInputReqQSize, U32 slFillAttrCycle, U32 slInCacheLines,
    U32 slInCacheLineSize, U32 slInCachePortWidth, U32 slInCacheReqQSz, U32 slInCacheInQSz,
    bool slForceAttrLoadBypass,
    char **slPrefixArray, char **shPrefixArray, char *name, cmoMduBase *parent):

    indicesCycle(idxCycle), indexBufferSize(idxBufferSz), outputFIFOSize(outFIFOSz), outputMemorySize(outMemSz),
    numShaders(numSh), maxShOutLat(shOutLat), verticesCycle(vertCycle), paAttributes(paAttrCycle), outputBusLat(outLat),
    streamerLoaderUnits(sLoaderUnits), slIndicesCycle(slIdxCycle), slInputRequestQueueSize(slInputReqQSize),
    slFillAttributes(slFillAttrCycle), slInputCacheLines(slInCacheLines), slInputCacheLineSize(slInCacheLineSize),
    slInputCachePortWidth(slInCachePortWidth), slInputCacheReqQSz(slInCacheReqQSz), slInputCacheInQSz(slInCacheInQSz),
    slForceAttrLoadBypass(slForceAttrLoadBypass),
    cmoMduBase(name, parent)
{
    U32 i;
    U32 numShadersPerSL;
    char **sLoaderName;

    DynamicObject *defaultStreamerState[1];
    //DynamicObject *defaultConsumerState[1];

    //  Check shaders and shader signals prefixes.  
    GPU_ASSERT(
        //  There must at least 1 Vertex Shader.  
        if (numShaders == 0)
            CG_ASSERT("Invalid number of Shaders (must be >0).");

        if (numShaders < streamerLoaderUnits)
           CG_ASSERT("Greater or equal number of Shaders than cmoStreamController Loader units are required to make the binding");
 
        if (GPU_MOD(numShaders, streamerLoaderUnits) != 0)
           CG_ASSERT("Number of Shaders and cmoStreamController Loader units need to be multiples to make the binding");

        //  The shader prefix pointer array must be not null.  
        if (shPrefixArray == NULL)
            CG_ASSERT("The shader prefix array is null.");

        //  Check if the prefix arrays point to null.  
        for(i = 0; i < numShaders; i++)
        {
            if (shPrefixArray[i] == NULL)
                CG_ASSERT("Shader prefix array component is null.");
        }
    )

    //  Create control signal from the Command Processor.  
    streamCtrlSignal = newInputSignal("StreamerControl", 1, 1, NULL);

    //  Create state signal to the Command Processor.  
    streamStateSignal = newOutputSignal("StreamerState", 1, 1, NULL);

    //  Build initial signal data.  
    defaultStreamerState[0] = new StreamerStateInfo(ST_READY);

    //  Set initial signal value.  
    streamStateSignal->setData(defaultStreamerState);

    //  Create command signal to the cmoStreamController Fetch.  
    streamerFetchCom = newOutputSignal("StreamerFetchCommand", 1, 1);

    //  Create state signal from the cmoStreamController Fetch.  
    streamerFetchState = newInputSignal("StreamerFetchState", 1, 1);

    //  Create command signal to the cmoStreamController Output Cache.  
    streamerOCCom = newOutputSignal("StreamerOutputCacheCommand", 1, 1);

    //  Create command signal to the cmoStreamController Commit.  
    streamerCommitCom = newOutputSignal("StreamerCommitCommand", 1, 1);

    //  Create state signal from the cmoStreamController Commit.  
    streamerCommitState = newInputSignal("StreamerCommitState", 1, 1);

    //  Create StreamController sub boxes.  

    //  Create cmoStreamController Fetch mdu.  
    streamerFetch = new StreamerFetch(
        indicesCycle,                                     //  Indices to fetch/generate per cycle.  
        indexBufferSize,                                  //  Size in bytes of the index buffer.  
        outputFIFOSize,                                   //  Entries in the output FIFO (indices).  
        outputMemorySize,                                 //  Size of the output memory (vertices) (Vertex Cache).  
        slInputRequestQueueSize,                          //  Number of entries in each Input Request Queues of the cmoStreamController Loader units (indices).  
        verticesCycle,                                    //  Number of vertices commited per cycle at cmoStreamController Commit.  
        slPrefixArray,                                    //  Array of cmoStreamController Loader units signal prefixes.  
        streamerLoaderUnits,                              //  Number of cmoStreamController Loader units.  
        slIndicesCycle,                                   //  Indices per cycle processed per cmoStreamController Loader Unit.  
        "StreamerFetch", this);

    //  Create cmoStreamController Output Cache mdu.  
    streamerOutputCache = new StreamerOutputCache(
        indicesCycle,               //  Indices received/searched per cycle.  
        outputMemorySize,           //  Size of the output memory (vertices) (Vertex Cache).  
        verticesCycle,              //  Number of vertices commited per cycle at cmoStreamController Commit.  
        streamerLoaderUnits,        //  Number of cmoStreamController Loader Units. 
        slIndicesCycle,             //  Indices consumed each cycle per cmoStreamController Loader Unit. 
        "StreamerOutputCache", this);

    streamerLoader = new StreamerLoader*[streamerLoaderUnits];

    sLoaderName = new char*[streamerLoaderUnits];

    streamerLoaderCom = new Signal*[streamerLoaderUnits];

    //  Compute how many shaders attached to each StreamerLoader unit.  
    numShadersPerSL = numShaders / streamerLoaderUnits;

    for (i = 0; i < streamerLoaderUnits; i++)
    {
        sLoaderName[i] = new char[18];
        sprintf(sLoaderName[i], "StreamerLoader%d",i);
        CG_INFO("Creating StreamerLoader Unit %d", i); 
        //  Create cmoStreamController Loader boxes.  
        streamerLoader[i] = new StreamerLoader(
                i,
                slIndicesCycle,                       //  Number of indices received per cycle.  
                slInputRequestQueueSize,              //  Entries in the input request queue (indices).  
                slFillAttributes,                     //  Number of vertex attributes filled per cycle.  
                slInputCacheLines,                    //  Number of lines of the input cache.  
                slInputCacheLineSize,                 //  Size of the input cache line in bytes.  
                slInputCachePortWidth,                //  Width in bytes of the input cache ports.  
                slInputCacheReqQSz,                   //  Entries in the input cache Request queue.  
                slInputCacheInQSz,                    //  Entries in the input cache input queue.  
                slForceAttrLoadBypass,                //  Force attribute load bypass.  
                numShadersPerSL,                      //  Number of vertex shaders attached to each cmoStreamController Loader Unit.  
                &shPrefixArray[i * numShadersPerSL],  //  Array of prefixes for the vertex shaders.  
                sLoaderName[i],                       //  cmoStreamController Loader Unit name.  
                slPrefixArray[i],                     //  cmoStreamController Loader Unit prefix. 
                this);

        //  Create command signal to the cmoStreamController Loader.  
        streamerLoaderCom[i] = newOutputSignal("StreamerLoaderCommand", 1, 1, slPrefixArray[i]);

        delete[] sLoaderName[i];
    }

    delete[] sLoaderName;

    //  Create cmoStreamController Commit mdu.  
    streamerCommit = new StreamerCommit(
        indicesCycle,               //  Number of indices received per cycle.  
        outputFIFOSize,             //  Number of entries (indices) in the output FIFO.  
        outputMemorySize,           //  Number of entries (vertices) in the output Memory (Vertex Cache).  
        numShaders,                 //  Number of vertex attached to cmoStreamController Commit.  
        shOutLat,                   //  Maximum latency of the shader output signal.  
        verticesCycle,              //  Number of vertices sent to Primitive Assembly per cycle.  
        paAttributes,               //  Vertex attributes sent to Primitive Assembly per cycle.  
        outputBusLat,               //  Latency of the vertex bus with Primitive Assembly.  
        shPrefixArray,              //  Array of prefixes for the vertex shaders.  
        "StreamerCommit", this);

    //  Set initial state.  
    state = ST_RESET;

    //  Set last StreamController command to a dummy StreamController command.  
    lastStreamCom = new StreamerCommand(STCOM_RESET);
}

//  Simulation rutine.  
void cmoStreamController::clock(U64 cycle)
{
    //MemoryTransaction *memTrans;
    StreamerCommand *streamCom;
    StreamerStateInfo *streamState;
    StreamerState streamCommit;
    StreamerState streamFetch;

    int i;

    GPU_DEBUG( printf("cmoStreamController => Clock %lld\n", cycle);)

    //  Clock all cmoStreamController subunits.  

    //  Clock cmoStreamController Fetch.  
    streamerFetch->clock(cycle);

    //  Clock cmoStreamController Output Cache.  
    streamerOutputCache->clock(cycle);

    //  Clock cmoStreamController Loader boxes.  
    for (U32 i = 0; i < streamerLoaderUnits; i++)
    {    
       streamerLoader[i]->clock(cycle);
    }

    //  Clock cmoStreamController Commit.  
    streamerCommit->clock(cycle);

    //  Get state from the cmoStreamController Fetch.  
    if(streamerFetchState->read(cycle, (DynamicObject *&) streamState))
    {
        //  Keep state info.  
        streamFetch = streamState->getState();

        //  Delete StreamController state info object.  
        delete streamState;
    }
    else
    {
        //  Missing state signal.  
        CG_ASSERT("No state signal from the cmoStreamController Fetch.");
    }

    //  Get state from the cmoStreamController Commit.  
    if (streamerCommitState->read(cycle, (DynamicObject *&) streamState))
    {
        //  Keep state info.  
        streamCommit = streamState->getState();

        //  Delete StreamController state info object.  
        delete streamState;
    }
    else
    {
        //  Missing signal.  
        CG_ASSERT("No state signal from the cmoStreamController Commit.");
    }

    //  cmoStreamController tasks.  
    switch(state)
    {
        case ST_RESET:
            //  cmoStreamController is resetting.  

            GPU_DEBUG( printf("cmoStreamController => RESET state.\n"); )

            //  Issue reset signals to all the cmoStreamController units.  


            //  Send reset to the cmoStreamController Fetch.  

            //  Create reset StreamController command.  
            streamCom = new StreamerCommand(STCOM_RESET);

            //  Copy cookies from original reset command.  
            streamCom->copyParentCookies(*lastStreamCom);

            //  Add a new cookie.  
            streamCom->addCookie();

            //  Send signal to the cmoStreamController Fetch.  
            streamerFetchCom->write(cycle, streamCom);


            //  Send reset to the cmoStreamController Output Cache.  

            //  Create reset StreamController command.  
            streamCom = new StreamerCommand(STCOM_RESET);

            //  Copy cookies from original reset command.  
            streamCom->copyParentCookies(*lastStreamCom);

            //  Add a new cookie.  
            streamCom->addCookie();

            //  Send signal to the cmoStreamController Output Cache.  
            streamerOCCom->write(cycle, streamCom);

            //  Send reset to the cmoStreamController Loader Units.  
            for (U32 i = 0; i < streamerLoaderUnits; i++)
            {
                //  Create reset StreamController command.  
                streamCom = new StreamerCommand(STCOM_RESET);

                //  Copy cookies from original reset command.  
                streamCom->copyParentCookies(*lastStreamCom);

                //  Add a new cookie.  
                streamCom->addCookie();

                streamerLoaderCom[i]->write(cycle, streamCom);
            }

            //  Send reset to the cmoStreamController Commit.  

            //  Create reset StreamController command.  
            streamCom = new StreamerCommand(STCOM_RESET);

            //  Copy cookies from original reset command.  
            streamCom->copyParentCookies(*lastStreamCom);

            //  Add a new cookie.  
            streamCom->addCookie();

            //  Send signal to the cmoStreamController Commit.  
            streamerCommitCom->write(cycle, streamCom);

            //  Set initial index stream.  
            indexBuffer = 0;

            //  Change state.  
            state = ST_READY;

            break;

        case ST_READY:
            /*  cmoStreamController can receive new commands and state changes from
                the Command Processor.  */

            GPU_DEBUG( printf("cmoStreamController => READY state.\n"); )

            //  Get commands from the Command Processor.  
            if (streamCtrlSignal->read(cycle, (DynamicObject *&) streamCom))
            {
                //  Process StreamController command.  
                processStreamerCommand(cycle, streamCom);
            }

            break;

        case ST_STREAMING:
            //  cmoStreamController is streaming data to the Shaders.  

            /*  Check if the cmoStreamController Fetch and the cmoStreamController Commit have
                finished.  */
            if ((streamFetch == ST_FINISHED) && (streamCommit == ST_FINISHED))
            {
                //  Change cmoStreamController state to finished.  
                state = ST_FINISHED;
            }

            break;

        case ST_FINISHED:
            /*  cmStreamer.has finished streaming data to the Shader.
                Waiting for Command Processor response.  */

            GPU_DEBUG( printf("cmoStreamController => FINISHED state.\n"); )

            //  Receive a command from the Command Processor 
            if (streamCtrlSignal->read(cycle, (DynamicObject *&) streamCom))
            {
                processStreamerCommand(cycle, streamCom);
            }

            break;

        default:
            CG_ASSERT("Undefined cmoStreamController state.");
            break;
    }

    //  Send cmoStreamController state to the Command Processor.  
    streamStateSignal->write(cycle, new StreamerStateInfo(state));
}

//  Processes a stream command.  
void cmoStreamController::processStreamerCommand(U64 cycle, StreamerCommand *streamCom)
{
    StreamerCommand *auxStCom;

    //  Delete last StreamController command.  
    delete lastStreamCom;

    //  Store as last StreamController command (to keep object info).  
    lastStreamCom = streamCom;

    //  Process stream command.  
    switch(streamCom->getCommand())
    {
        case STCOM_RESET:
            //  Reset command from the Command Processor.  

            GPU_DEBUG(
                printf("cmoStreamController => RESET command.\n");
            )

            //  Do whatever to do.  

            //  Change state to reset.  
            state = ST_RESET;

            break;

        case STCOM_REG_WRITE:
            //  Write to register command.  

            GPU_DEBUG(
                printf("cmoStreamController => REG_WRITE command.\n");
            )

            //  Check cmoStreamController state.  
            CG_ASSERT_COND(!(state != ST_READY), "Command not allowed in this state.");
            processGPURegisterWrite(streamCom->getStreamerRegister(),
                streamCom->getStreamerSubRegister(),
                streamCom->getRegisterData(), cycle);

            break;

        case STCOM_REG_READ:
            //  Read from register command.  

            GPU_DEBUG(
                printf("cmoStreamController => REG_READ command.\n");
            )

            //  Not supported.  
            CG_ASSERT("STCOM_REG_READ not supported.");

            break;

        case STCOM_START:
            {
                //  Start streaming (drawing) command.  

                GPU_DEBUG(
                    printf("cmoStreamController => START command.\n");
                )

                //  Check cmoStreamController state.  
                CG_ASSERT_COND(!(state != ST_READY), "Command not allowed in this state.");
                U32 firstActiveAttribute = 0;
                
                //  Search for the first active attribute.  
                for(; (firstActiveAttribute < MAX_VERTEX_ATTRIBUTES) &&
                    (attributeMap[firstActiveAttribute] == ST_INACTIVE_ATTRIBUTE); firstActiveAttribute++);

                //  Check if there is an active attribute (SHOULD BE!!!).  
                CG_ASSERT_COND(!(firstActiveAttribute == MAX_VERTEX_ATTRIBUTES), "No active attributes.");
                //  Check the number of vertex to draw.  
                CG_ASSERT_COND(!(count == 0), "Trying to draw an empty vertex batch.");
                //  Send START command to all cmoStreamController units.  

                //  Send START command to the cmoStreamController Fetch.  

                //  Create START command.  
                auxStCom = new StreamerCommand(STCOM_START);

                //  Copy parent command cookies.  
                auxStCom->copyParentCookies(*lastStreamCom);

                //  Send command to the cmoStreamController Fetch.  
                streamerFetchCom->write(cycle, auxStCom);


                //  Send START command to the cmoStreamController Output Cache.  

                //  Create START command.  
                auxStCom = new StreamerCommand(STCOM_START);

                //  Copy parent command cookies.  
                auxStCom->copyParentCookies(*lastStreamCom);

                //  Send command to the cmoStreamController Output Cache.  
                streamerOCCom->write(cycle, auxStCom);


                //  Send START command to the cmoStreamController Loader Units.  
                for (U32 i = 0; i < streamerLoaderUnits; i++)
                {
                    //  Create START command.  
                    auxStCom = new StreamerCommand(STCOM_START);

                    //  Copy parent command cookies.  
                    auxStCom->copyParentCookies(*lastStreamCom);

                    streamerLoaderCom[i]->write(cycle, auxStCom);
                }

                //  Send START command to the cmoStreamController Commit.  

                //  Create START command.  
                auxStCom = new StreamerCommand(STCOM_START);

                //  Copy parent command cookies.  
                auxStCom->copyParentCookies(*lastStreamCom);

                //  Send command to the cmoStreamController Commit.  
                streamerCommitCom->write(cycle, auxStCom);


                //  Set StreamController state to streaming.  
                state = ST_STREAMING;

            }
            
            break;

        case STCOM_END:
            //  End streaming (drawing) command.  

            GPU_DEBUG(
                printf("cmoStreamController => END command.\n");
            )

            //  Check cmoStreamController state.  
            CG_ASSERT_COND(!(state != ST_FINISHED), "Command not allowed in this state.");
            //  Send END command to all cmoStreamController Units.  

            //  Send END command to the cmoStreamController Fetch.  

            //  Create END command.  
            auxStCom = new StreamerCommand(STCOM_END);

            //  Copy parent command cookies.  
            auxStCom->copyParentCookies(*lastStreamCom);

            //  Send command to the cmoStreamController Fetch.  
            streamerFetchCom->write(cycle, auxStCom);


            //  Send END command to the cmoStreamController Output Cache.  

            //  Create END command.  
            auxStCom = new StreamerCommand(STCOM_END);

            //  Copy parent command cookies.  
            auxStCom->copyParentCookies(*lastStreamCom);

            //  Send command to the cmoStreamController Output Cache.  
            streamerOCCom->write(cycle, auxStCom);

            
            //  Send END command to the cmoStreamController Loader Units.  
            for (U32 i = 0; i < streamerLoaderUnits; i++)
            {
                //  Create END command.  
                auxStCom = new StreamerCommand(STCOM_END);

                //  Copy parent command cookies.  
                auxStCom->copyParentCookies(*lastStreamCom);

                streamerLoaderCom[i]->write(cycle, auxStCom);
            }


            //  Send END command to the cmoStreamController Commit.  

            //  Create END command.  
            auxStCom = new StreamerCommand(STCOM_END);

            //  Copy parent command cookies.  
            auxStCom->copyParentCookies(*lastStreamCom);

            //  Send command to the cmoStreamController Commit.  
            streamerCommitCom->write(cycle, auxStCom);


            //  Change state to ready.  
            state = ST_READY;

            break;


        default:
            CG_ASSERT("Undefined StreamController command.");
            break;
    }
}

//  Process a register write.  
void cmoStreamController::processGPURegisterWrite(GPURegister gpuReg, U32 gpuSubReg,
     GPURegData gpuData, U64 cycle)
{
    StreamerCommand *streamCom;
    int i;

    //  Process Register.  
    switch(gpuReg)
    {
        case GPU_VERTEX_OUTPUT_ATTRIBUTE:
            //  Vertex output attribute active/written.  

            GPU_DEBUG(
                printf("cmoStreamController => GPU_VERTEX_OUTPUT_ATTRIBTE[%d] = %s.\n",
                    gpuSubReg, gpuData.booleanVal?"TRUE":"FALSE");
            )

            //  Check the vertex attribute ID.  
            CG_ASSERT_COND(!(gpuSubReg >= MAX_VERTEX_ATTRIBUTES), "Illegal vertex attribute ID.");
            //  Update the register.  
            outputAttribute[gpuSubReg] = gpuData.booleanVal;

            //  Send register write to the cmoStreamController Commit.  

            //  Create StreamController register write.  
            streamCom = new StreamerCommand(gpuReg, gpuSubReg, gpuData);

            //  Copy cookies from original StreamController command.  
            streamCom->copyParentCookies(*lastStreamCom);

            //  Write commadn to the cmoStreamController Loader.  
            streamerCommitCom->write(cycle, streamCom);

            break;


        case GPU_VERTEX_ATTRIBUTE_MAP:
            //  Mapping between stream buffers and vertex input parameters.  

            GPU_DEBUG(
                printf("cmoStreamController => GPU_VERTEX_ATTRIBUTE_MAP[%d] = %d.\n",
                    gpuSubReg, gpuData.uintVal);
            )

            //  Check the vertex attribute ID.  
            CG_ASSERT_COND(!(gpuSubReg >= MAX_VERTEX_ATTRIBUTES), "Illegal vertex attribute ID.");
            //  Check the stream buffer ID.  
            CG_ASSERT_COND(!((gpuData.uintVal >= MAX_STREAM_BUFFERS) && (gpuData.uintVal != ST_INACTIVE_ATTRIBUTE)), "Illegal stream buffer ID.");
            //  Write the register.  
            attributeMap[gpuSubReg] = gpuData.uintVal;

            //  Send register write to all the cmoStreamController Loader Units.  
            for (U32 i = 0; i < streamerLoaderUnits; i++)
            {
                //  Create StreamController register write.  
                streamCom = new StreamerCommand(gpuReg, gpuSubReg, gpuData);

                //  Copy cookies from original StreamController command.  
                streamCom->copyParentCookies(*lastStreamCom);

                streamerLoaderCom[i]->write(cycle, streamCom);
            }

            break;

        case GPU_VERTEX_ATTRIBUTE_DEFAULT_VALUE:
            //  Mapping between stream buffers and vertex input parameters.  

            GPU_DEBUG(
                printf("cmoStreamController => GPU_VERTEX_ATTRIBUTE_DEFAULT_VALUE[%d] = {%f, %f, %f, %f}.\n",
                    gpuSubReg, gpuData.qfVal[0], gpuData.qfVal[1], gpuData.qfVal[2], gpuData.qfVal[3]);
            )

            //  Check the vertex attribute ID.  
            CG_ASSERT_COND(!(gpuSubReg >= MAX_VERTEX_ATTRIBUTES), "Illegal vertex attribute ID.");
            //  Write the register.  
            attrDefValue[gpuSubReg][0] = gpuData.qfVal[0];
            attrDefValue[gpuSubReg][1] = gpuData.qfVal[1];
            attrDefValue[gpuSubReg][2] = gpuData.qfVal[2];
            attrDefValue[gpuSubReg][3] = gpuData.qfVal[3];


            //  Send register write to all the cmoStreamController Loader Units.  
            for (U32 i = 0; i < streamerLoaderUnits; i++)
            {
                //  Create StreamController register write.  
                streamCom = new StreamerCommand(gpuReg, gpuSubReg, gpuData);

                //  Copy cookies from original StreamController command.  
                streamCom->copyParentCookies(*lastStreamCom);

                streamerLoaderCom[i]->write(cycle, streamCom);
            }

            break;

        case GPU_STREAM_ADDRESS:
            //  Stream buffer address.  

            //  Check the stream buffer ID.  
            CG_ASSERT_COND(!(gpuSubReg >= MAX_STREAM_BUFFERS), "Illegal stream buffer ID.");
            //  Check start address.  


            //  Write the register.  
            bufferAddress[gpuSubReg] = gpuData.uintVal;

            GPU_DEBUG(
                printf("cmoStreamController => GPU_STREAM_ADDRESS[%d] = %x.\n",
                    gpuSubReg, gpuData.uintVal);
            )

            //  Send register write to the cmoStreamController Loader Units.  
            for (U32 i = 0; i < streamerLoaderUnits; i++)
            {
                //  Create StreamController register write.  
                streamCom = new StreamerCommand(gpuReg, gpuSubReg, gpuData);

                //  Copy cookies from original StreamController command.  
                streamCom->copyParentCookies(*lastStreamCom);

                streamerLoaderCom[i]->write(cycle, streamCom);
            }

            //  Check if it is the index stream.  
            if (gpuSubReg == indexBuffer)
            {
                //  Send register write to the cmoStreamController Fetch.  

                //  Create StreamController register write.  
                streamCom = new StreamerCommand(gpuReg, gpuSubReg, gpuData);

                //  Copy cookies from original StreamController command.  
                streamCom->copyParentCookies(*lastStreamCom);

                //  Write commadn to the cmoStreamController Fetch.  
                streamerFetchCom->write(cycle, streamCom);
            }

            break;

        case GPU_STREAM_STRIDE:
            //  Stream buffer stride.  

            //  Check the stream buffer ID.  
            CG_ASSERT_COND(!(gpuSubReg >= MAX_STREAM_BUFFERS), "Illegal stream buffer ID.");
            //  Write the register.  
            bufferStride[gpuSubReg] = gpuData.uintVal;

            GPU_DEBUG(
                printf("cmoStreamController => GPU_STREAM_STRIDE[%d] = %d.\n",
                    gpuSubReg, gpuData.uintVal);
            )

            //  Send register write to the cmoStreamController Loader Units.  
            for (U32 i = 0; i < streamerLoaderUnits; i++)
            {
                //  Create StreamController register write.  
                streamCom = new StreamerCommand(gpuReg, gpuSubReg, gpuData);

                //  Copy cookies from original StreamController command.  
                streamCom->copyParentCookies(*lastStreamCom);

                streamerLoaderCom[i]->write(cycle, streamCom);
            }

            break;

        case GPU_STREAM_DATA:
            //  Stream buffer data type.  

            //  Check the stream buffer ID.  
            CG_ASSERT_COND(!(gpuSubReg >= MAX_STREAM_BUFFERS), "Illegal stream buffer ID.");
            //  Write the register.  
            bufferData[gpuSubReg] = gpuData.streamData;

            GPU_DEBUG(
                printf("cmoStreamController => GPU_STREAMDATA[%d] = ", gpuSubReg);
                switch(gpuData.streamData)
                {
                    case SD_UNORM8:
                        printf("SD_UNORM8");
                        break;
                    case SD_SNORM8:
                        printf("SD_SNORM8");
                        break;
                    case SD_UNORM16:
                        printf("SD_UNORM16");
                        break;
                    case SD_SNORM16:
                        printf("SD_SNORM16");
                        break;
                    case SD_UNORM32:
                        printf("SD_UNORM32");
                        break;
                    case SD_SNORM32:
                        printf("SD_SNORM32");
                        break;
                    case SD_FLOAT16:
                        printf("SD_FLOAT16");
                        break;                    
                    case SD_FLOAT32:
                        printf("SD_FLOAT32");
                        break;
                    case SD_UINT8:
                        printf("SD_UINT8");
                        break;
                    case SD_SINT8:
                        printf("SD_SINT8");
                        break;
                    case SD_UINT16:
                        printf("SD_UINT16");
                        break;
                    case SD_SINT16:
                        printf("SD_SINT16");
                        break;
                    case SD_UINT32:
                        printf("SD_UINT32");
                        break;
                    case SD_SINT32:
                        printf("SD_SINT32");
                        break;
                    default:
                        CG_ASSERT("Undefined format.");
                        break;                        
                }
            )

            //  Send register write to the cmoStreamController Loader Units.  
            for (U32 i = 0; i < streamerLoaderUnits; i++)
            {
                //  Create StreamController register write.  
                streamCom = new StreamerCommand(gpuReg, gpuSubReg, gpuData);

                //  Copy cookies from original StreamController command.  
                streamCom->copyParentCookies(*lastStreamCom);

                streamerLoaderCom[i]->write(cycle, streamCom);
            }

            //  Check if it is the index stream.  
            if (gpuSubReg == indexBuffer)
            {
                //  Send register write to the cmoStreamController Fetch.  

                //  Create StreamController register write.  
                streamCom = new StreamerCommand(gpuReg, gpuSubReg, gpuData);

                //  Copy cookies from original StreamController command.  
                streamCom->copyParentCookies(*lastStreamCom);

                //  Write command to the cmoStreamController Fetch.  
                streamerFetchCom->write(cycle, streamCom);
            }

            break;

        case GPU_STREAM_ELEMENTS:
            //  Stream buffer elements (of any stream data type).  

            //  Check the stream buffer ID.  
            CG_ASSERT_COND(!(gpuSubReg >= MAX_STREAM_BUFFERS), "Illegal stream buffer ID.");
            //  Write the register.  
            bufferElements[gpuSubReg] = gpuData.uintVal;

            GPU_DEBUG(
                printf("cmoStreamController => GPU_STREAM_ELEMENTS[%d] = %d.\n",
                    gpuSubReg, gpuData.uintVal);
            )

            //  Send register write to the cmoStreamController Loader Units.  
            for (U32 i = 0; i < streamerLoaderUnits; i++)
            {
                //  Create StreamController register write.  
                streamCom = new StreamerCommand(gpuReg, gpuSubReg, gpuData);

                //  Copy cookies from original StreamController command.  
                streamCom->copyParentCookies(*lastStreamCom);

                streamerLoaderCom[i]->write(cycle, streamCom);
            }

            break;

        case GPU_STREAM_FREQUENCY:
            
            //  Stream buffer update/read frequency (indices/vertices).

            //  Check the stream buffer ID.
            CG_ASSERT_COND(!(gpuSubReg >= MAX_STREAM_BUFFERS), "Illegal stream buffer ID.");
            //  Write the register.
            bufferFrequency[gpuSubReg] = gpuData.uintVal;

            GPU_DEBUG(
                printf("cmoStreamController => GPU_STREAM_FREQUENCY[%d] = %d.\n",
                    gpuSubReg, gpuData.uintVal);
            )

            //  Send register write to the cmoStreamController Loader Units.
            for (U32 i = 0; i < streamerLoaderUnits; i++)
            {
                //  Create StreamController register write.
                streamCom = new StreamerCommand(gpuReg, gpuSubReg, gpuData);

                //  Copy cookies from original StreamController command.
                streamCom->copyParentCookies(*lastStreamCom);

                streamerLoaderCom[i]->write(cycle, streamCom);
            }

            break;

        case GPU_STREAM_START:
            //  Streaming start position.  

            start = gpuData.uintVal;

            GPU_DEBUG(
                printf("cmoStreamController => GPU_STREAM_START = %d.\n", gpuData.uintVal);
            )

            //  Send register write to the cmoStreamController Fetch.  

            //  Create StreamController register write.  
            streamCom = new StreamerCommand(gpuReg, gpuSubReg, gpuData);

            //  Copy cookies from original StreamController command.  
            streamCom->copyParentCookies(*lastStreamCom);

            //  Write command to the cmoStreamController Fetch.  
            streamerFetchCom->write(cycle, streamCom);

            //  Send register write to the cmoStreamController Loader Units.  
            for (U32 i = 0; i < streamerLoaderUnits; i++)
            {
                //  Create StreamController register write.  
                streamCom = new StreamerCommand(gpuReg, gpuSubReg, gpuData);

                //  Copy cookies from original StreamController command.  
                streamCom->copyParentCookies(*lastStreamCom);

                streamerLoaderCom[i]->write(cycle, streamCom);
            }

            break;

        case GPU_STREAM_COUNT:
            //  Streaming vertex count.  

            count = gpuData.uintVal;

            GPU_DEBUG(
                printf("cmoStreamController => GPU_STREAM_COUNT = %d.\n", gpuData.uintVal);
            )

            //  Send register write to the cmoStreamController Fetch.  

            //  Create StreamController register write.  
            streamCom = new StreamerCommand(gpuReg, gpuSubReg, gpuData);

            //  Copy cookies from original StreamController command.  
            streamCom->copyParentCookies(*lastStreamCom);

            //  Write command to the cmoStreamController Fetch.  
            streamerFetchCom->write(cycle, streamCom);

            //  Send register write to the cmoStreamController Commit.  

            //  Create StreamController register write.  
            streamCom = new StreamerCommand(gpuReg, gpuSubReg, gpuData);

            //  Copy cookies from original StreamController command.  
            streamCom->copyParentCookies(*lastStreamCom);

            //  Write command to the cmoStreamController Commit.  
            streamerCommitCom->write(cycle, streamCom);

            break;

        case GPU_STREAM_INSTANCES:
            
            //  Streaming instance count.

            instances = gpuData.uintVal;

            GPU_DEBUG(
                printf("cmoStreamController => GPU_STREAM_INSTANCES = %d.\n", gpuData.uintVal);
            )

            //  Send register write to the cmoStreamController Fetch.

            //  Create StreamController register write.
            streamCom = new StreamerCommand(gpuReg, gpuSubReg, gpuData);

            //  Copy cookies from original StreamController command.
            streamCom->copyParentCookies(*lastStreamCom);

            //  Write command to the cmoStreamController Fetch.
            streamerFetchCom->write(cycle, streamCom);

            //  Send register write to the cmoStreamController Commit.

            //  Create StreamController register write.
            streamCom = new StreamerCommand(gpuReg, gpuSubReg, gpuData);

            //  Copy cookies from original StreamController command.
            streamCom->copyParentCookies(*lastStreamCom);

            //  Write command to the cmoStreamController Commit.
            streamerCommitCom->write(cycle, streamCom);

            break;

        case GPU_INDEX_MODE:
            //  Indexed streaming mode enabled/disabled.  

            indexedMode = gpuData.booleanVal;

            GPU_DEBUG(
                printf("cmoStreamController => GPU_INDEX_MODE = %s.\n",
                    gpuData.booleanVal?"TRUE":"FALSE");
            )

            //  Send register write to the cmoStreamController Fetch.  

            //  Create StreamController register write.  
            streamCom = new StreamerCommand(gpuReg, gpuSubReg, gpuData);

            //  Copy cookies from original StreamController command.  
            streamCom->copyParentCookies(*lastStreamCom);

            //  Write commadn to the cmoStreamController Loader.  
            streamerFetchCom->write(cycle, streamCom);

            break;

        case GPU_INDEX_STREAM:
            //  Index stream buffer ID.  

            //  Check stream buffer ID.  
            CG_ASSERT_COND(!(gpuData.uintVal >= MAX_STREAM_BUFFERS), "Illegal stream buffer ID.");
            indexBuffer = gpuData.uintVal;

            GPU_DEBUG(
                printf("cmoStreamController => GPU_INDEX_STREAM = %d.\n", gpuData.uintVal);
            )

            break;

        case GPU_D3D9_COLOR_STREAM:
    
            //  Sets if the color stream has be read using the component order defined by D3D9.

            //  Check stream buffer ID.  
            CG_ASSERT_COND(!(gpuSubReg >= MAX_STREAM_BUFFERS), "Stream buffer ID out of range.");
            d3d9ColorStream[gpuSubReg] = gpuData.booleanVal;

            GPU_DEBUG(
                printf("cmoStreamController => GPU_D3D9_COLOR_STREAM[%d] = %s.\n", gpuSubReg, gpuData.booleanVal ? "TRUE" : "FALSE");
            )

            //  Send register write to the cmoStreamController Loader Units.
            for (U32 i = 0; i < streamerLoaderUnits; i++)
            {
                //  Create StreamController register write.
                streamCom = new StreamerCommand(gpuReg, gpuSubReg, gpuData);

                //  Copy cookies from original StreamController command.
                streamCom->copyParentCookies(*lastStreamCom);

                streamerLoaderCom[i]->write(cycle, streamCom);
            }
            
            break;

        case GPU_ATTRIBUTE_LOAD_BYPASS:
    
            //  Sets if the attribute load stage is bypassed to implement attribute load in the shader.
            attributeLoadBypass = gpuData.booleanVal;

            GPU_DEBUG(
                printf("cmoStreamController => GPU_ATTRIBUTE_LOAD_BYPASS = %s.\n", gpuData.booleanVal ? "TRUE" : "FALSE");
            )

            //  Send register write to the cmoStreamController Loader Units.
            for (U32 i = 0; i < streamerLoaderUnits; i++)
            {
                //  Create StreamController register write.
                streamCom = new StreamerCommand(gpuReg, gpuSubReg, gpuData);

                //  Copy cookies from original StreamController command.
                streamCom->copyParentCookies(*lastStreamCom);

                streamerLoaderCom[i]->write(cycle, streamCom);
            }

            break;

        default:
            CG_ASSERT("Not a cmoStreamController register.");
            break;
    }
}

void cmoStreamController::setValidationMode(bool enable)
{
    streamerCommit->setValidationMode(enable);
    for(U32 u = 0; u < streamerLoaderUnits; u++)
        streamerLoader[u]->setValidationMode(enable);
}

ShadedVertexMap &cmoStreamController::getShadedVertexInfo()
{
    return streamerCommit->getShadedVertexInfo();
}

VertexInputMap &cmoStreamController::getVertexInputInfo(U32 unit)
{
    return streamerLoader[unit]->getVertexInputInfo();
}




