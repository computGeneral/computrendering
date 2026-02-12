/**************************************************************************
 *
 * Z Stencil Test mdu implementation file.
 *
 */

/**
 *
 *  @file ZStencilTest.cpp
 *
 *  This file implements the Z Stencil Test mdu.
 *
 *  This class implements the Z and Stencil test
 *  stages of the GPU pipeline.
 *
 */

#include "cmZStencilTest.h"
#include "cmMemoryTransaction.h"
#include "cmZOperation.h"
#include "cmRasterizerStateInfo.h"
#include "cmZStencilStatusInfo.h"
#include "cmColorWriteStatusInfo.h"
#include "cmHZUpdate.h"
#include "GPUMath.h"
#include "GPUReg.h" // Access to STAMP_FRAGMENTS
#include <sstream>

using namespace arch;

//  Z Stencil Test mdu constructor.  
ZStencilTest::ZStencilTest(U32 numStamps, U32 overWidth, U32 overHeight,
    U32 scanWidth, U32 scanHeight, U32 genWidth, U32 genHeight, U32 depthBytes,
    bool zComprDisabled, U32 cacheWays, U32 cacheLines, U32 stampsPerLine,
    U32 portWidth, bool extraReadP, bool extraWriteP, U32 cacheReqQSz, U32 inputReqQSz, U32 outputReqQSz,
    U32 zBlocks, U32 blocksCycle, U32 comprCycles,
    U32 decomprCycles, U32 zQSize, U32 zTestRate, U32 testLat,
    bool hzUpdDisabled, U32 updateLatency, bmoFragmentOperator &fragEmu, bmoRasterizer &rastEmul,
    char *name, char *prefix, cmoMduBase *parent) :

    stampsCycle(numStamps), overH(overHeight), overW(overWidth),
    scanH((U32) ceil(scanHeight / (F32) genHeight)), scanW((U32) ceil(scanWidth / (F32) genWidth)),
    genH(genHeight / STAMP_HEIGHT), genW(genWidth / STAMP_WIDTH), bytesPixel(depthBytes),
    disableZCompression(zComprDisabled), zCacheWays(cacheWays), zCacheLines(cacheLines),
    stampsLine(stampsPerLine), cachePortWidth(portWidth), extraReadPort(extraReadP), extraWritePort(extraWriteP),
    zQueueSize(zQSize), testRate(zTestRate), testLatency(testLat),
    disablecmHZUpdate.hzUpdDisabled), hzUpdateLatency(updateLatency),
    frEmu(fragEmu), bmRast(rastEmul), cmoMduBase(name, parent)

{
    DynamicObject *defaultState[1];
    U32 i;
    char fullName[64];
    char postfix[32];

    //  Check parameters.  
    GPU_ASSERT(
        if (stampsCycle == 0)
            CG_ASSERT("At least a stamp must be received per cycle.");
        if (overW == 0)
            CG_ASSERT("Over scan tile must be at least 1 scan tile wide.");
        if (overH == 0)
            CG_ASSERT("Over scan tile must be at least 1 scan tile high.");
        if (scanW == 0)
            CG_ASSERT("Scan tile must be at least 1 generation tile wide.");
        if (scanH == 0)
            CG_ASSERT("Scan tile must be at least 1 generation tile high.");
        if (genW == 0)
            CG_ASSERT("Scan tile must be at least 1 stamp wide.");
        if (genH == 0)
            CG_ASSERT("Scan tile must be at least 1 stamp high.");
        if (bytesPixel != 4)
            CG_ASSERT("Only supported 32bit Z/Stencil values.");
        if (zCacheWays < 1)
            CG_ASSERT("Z Cache requires at least 1 way.");
        if (zCacheLines < 1)
            CG_ASSERT("Z Cache requires at least 1 line.");
        if (stampsLine < 1)
            CG_ASSERT("Z Cache requires at least one stamp per line.");
        if (cachePortWidth == 0)
            CG_ASSERT("Z Cache ports must be at least 1 byte wide.");
        if ((!disableZCompression) && (stampsLine < 16))
            CG_ASSERT("Z Compression only supported for 8x8 fragment tiles and 256 byte cache lines.");
        if (zQueueSize < 2)
            CG_ASSERT("Z queue requires at least two entries.");
        if (testRate == 0)
            CG_ASSERT("Z/Stencil Test ALU stamp start latency must be at least 1.");
        if (testLatency < 1)
            CG_ASSERT("Z/Stencil Test ALU latency must be 1 or greater.");
        if (disableZCompression && !disableHZUpdate)
            CG_ASSERT("HZ update requires Z compression.");
        if (hzUpdateLatency < 1)
            CG_ASSERT("HZ update signal latency must be 1 or greater.");
    )

    //  Create the full name and postfix for the statistics.  
    sprintf(fullName, "%s::%s", prefix, name);
    sprintf(postfix, "ZST-%s", prefix);

    //  Create statistics.  
    inputs = &getSM().getNumericStatistic("InputFragments", U32(0), fullName, postfix);
    outputs = &getSM().getNumericStatistic("OutputFragments", U32(0), fullName, postfix);
    tested = &getSM().getNumericStatistic("TestedFragments", U32(0), fullName, postfix);
    passed = &getSM().getNumericStatistic("PassedFragments", U32(0), fullName, postfix);
    failed = &getSM().getNumericStatistic("FailedFragments", U32(0), fullName, postfix);
    outside = &getSM().getNumericStatistic("OutsideFragments", U32(0), fullName, postfix);
    culled = &getSM().getNumericStatistic("CulledFragments", U32(0), fullName, postfix);
    readTrans = &getSM().getNumericStatistic("ReadTransactions", U32(0), fullName, postfix);
    writeTrans = &getSM().getNumericStatistic("WriteTransactions", U32(0), fullName, postfix);
    readBytes = &getSM().getNumericStatistic("ReadBytes", U32(0), fullName, postfix);
    writeBytes = &getSM().getNumericStatistic("WriteBytes", U32(0), fullName, postfix);
    fetchOK = &getSM().getNumericStatistic("FetchOK", U32(0), fullName, postfix);
    fetchFail = &getSM().getNumericStatistic("FetchFailed", U32(0), fullName, postfix);
    readOK = &getSM().getNumericStatistic("ReadOK", U32(0), fullName, postfix);
    readFail = &getSM().getNumericStatistic("ReadFailed", U32(0), fullName, postfix);
    writeOK = &getSM().getNumericStatistic("WriteOK", U32(0), fullName, postfix);
    writeFail = &getSM().getNumericStatistic("WriteFailed", U32(0), fullName, postfix);
    updatesHZ = &getSM().getNumericStatistic("UpdatesHZ", U32(0), fullName, postfix);
    rawDep = &getSM().getNumericStatistic("RAWDependence", U32(0), fullName, postfix);

    //  Create signals.  


    //  Create signals with the main Rasterizer mdu.  

    //  Create command signal from the main Rasterizer mdu.  
    rastCommand = newInputSignal("ZStencilTestCommand", 1, 1, prefix);

    //  Create state signal to the main Rasterizer mdu.  
    rastState = newOutputSignal("ZStencilTestRasterizerState", 1, 1, prefix);

    //  Create default signal value.  
    defaultState[0] = new RasterizerStateInfo(RAST_RESET);

    //  Set default signal value.  
    rastState->setData(defaultState);


    //  Create signals with Color Write mdu.  

    //  Create output fragments signal to Color Write mdu.   
    //colorWrite[COLOR_WRITE] = newOutputSignal("ColorWrite", stampsCycle * STAMP_FRAGMENTS, 1, prefix);

    //  Create state signal from Color Write Boxt.  
    //colorWriteState[COLOR_WRITE] = newInputSignal("ColorWriteState", 1, 1, prefix);

    //  Create signals with Fragment FIFO mdu.  

    //  Create output fragments signal to FragmentFIFO mdu.   
    colorWrite[FRAGMENT_FIFO] = newOutputSignal("ZStencilOutput", stampsCycle * STAMP_FRAGMENTS, 1, prefix);

    //  Create state signal from Fragment FIFO Boxt.  
    colorWriteState[FRAGMENT_FIFO] = newInputSignal("FFIFOZStencilState", 1, 1, prefix);


    //  Create signals with Fragment FIFO (Interpolator).  

    //  Create input fragment signal from Fragment FIFO (Interpolator).  
    fragments = newInputSignal("ZStencilInput", stampsCycle * STAMP_FRAGMENTS, 1, prefix);

    //  Create state signal to Fragment FIFO (Interpolator).  
    zStencilState = newOutputSignal("ZStencilState", 1, 1, prefix);

    //  Create default signal value.  
    defaultState[0] = new ZStencilStatusInfo(ZST_READY);

    //  Set default signal value.  
    zStencilState->setData(defaultState);


    //  Create signals with Memory Controller mdu.  

    //  Create request signal to the Memory Controller.  
    memRequest = newOutputSignal("ZStencilTestMemoryRequest", 1, 1, prefix);

    //  Create data signal from the Memory Controller.  
    memData = newInputSignal("ZStencilTestMemoryData", 2, 1, prefix);


    //  Create signals that simulate the blend latency.  

    //  Create start signal for test operation.  
    testStart = newOutputSignal("ZStencilTest", 1, testLatency, prefix);

    //  Create end signal for test operation.  
    testEnd = newInputSignal("ZStencilTest", 1, testLatency, prefix);


    //  Create signal with Hierarchical Z.  

    //  Create update signal to the Hierarchical Z mdu.  
    hzUpdate = newOutputSignal("HZUpdate", 1, hzUpdateLatency, prefix);


    /*
        !!!NOTE: A cache line contains a number of stamps from
        the same tile.  The stamps in a line must be in sequential
        lines.  !!!

     */
    bytesLine = stampsLine * STAMP_FRAGMENTS * bytesPixel;

    //  Calculate the line shift and mask.  
    lineShift = GPUMath::calculateShift(bytesLine);
    lineMask = GPUMath::buildMask(zCacheLines);

    //  Calculate stamp mask.  
    stampMask = GPUMath::buildMask(STAMP_FRAGMENTS * bytesPixel);

    //  Initialize Z cache.  
    zCache = new ZCache(
        zCacheWays,                         //  Number of ways in the Z Cache.  
        zCacheLines,                        //  Number of lines per way.  
        stampsLine,                         //  Stamps per line.  
        STAMP_FRAGMENTS * bytesPixel,       //  Bytes per stamp.  
        1 + (extraReadPort?1:0),            //  Read ports.  
        1 + (extraWritePort?1:0),           //  Write ports.  
        cachePortWidth,                     //  Width of the z cache ports (bytes).  
        cacheReqQSz,                        //  Size of the memory request queue.  
        inputReqQSz,                        //  Size of the input request queue.  
        outputReqQSz,                       //  Size of the output request queue.  
        disableZCompression,                //  Disables Z Compression and HZ update.  
        zBlocks,                            //  Number of Z blocks in the state memory.  
        blocksCycle,                        //  Number of Z blocks cleared per cycle.  
        comprCycles,                        //  Number of cycles to compress a Z block.  
        decomprCycles,                      //  Number of cycles to decompress a Z block.  
        postfix                             //  Postfix for the cache statistics.  
        );

    //  Allocate the blend queue.  
    zQueue = new ZQueue[zQueueSize];

    //  Check allocation.  
    CG_ASSERT_COND(!(zQueue == NULL), "Error allocating the Z queue.");
    //  Allocate the buffers for the stored stamps.  
    for(i = 0; i < zQueueSize; i++)
    {
        //  Allocate stamp input depth (U32).  
        zQueue[i].inDepth = new U32[STAMP_FRAGMENTS];

        //  Check allocation.  
        CG_ASSERT_COND(!(zQueue[i].inDepth == NULL), "Error allocating stamp input depth in the Z queue.");
        //  Allocate stamp fragment cull flags.  
        zQueue[i].culled= new bool[STAMP_FRAGMENTS];

        //  Check allocation.  
        CG_ASSERT_COND(!(zQueue[i].culled == NULL), "Error allocating stamp fragment cull flags.");
        //  Allocate stamp Z Buffer data array.  
        zQueue[i].zData = new U32[STAMP_FRAGMENTS];

        //  Allocate stamp fragment array.  
        zQueue[i].stamp = NULL;

    }

    //  Set memory ticket for clear.  
    //ticket = 0;

    //  Create a dummy initial rasterizer command.  
    lastRSCommand = new RasterizerCommand(RSCOM_RESET);

    //  Set early z enabled at start.  
    earlyZ = TRUE;

    //  Set initial state to reset.  
    state = RAST_RESET;
}

//  ZStentilTest simulation function.  
void ZStencilTest::clock(U64 cycle)
{
    MemoryTransaction *memTrans;
    RasterizerCommand *rastComm;
    ColorWriteStatusInfo *cwsInfo;
    ColorWriteState cwState;
    FragmentInput *frInput;
    ZOperation *zStencilOp;
    U32 zOperation;
    U32 hzBlock;
    U32 hzBlockZ;
    bool waitWrite;
    U32 i, j;

    GPU_DEBUG(
        printf("ZStencilTest => Clock %lld\n", cycle);
    )

    //  Receive state from Memory Controller.  
    while(memData->read(cycle, (DynamicObject *&) memTrans))
        processMemoryTransaction(cycle, memTrans);

    //  Update Z cache structures.  
    memTrans = zCache->update(cycle, memoryState);

    //  Check updates for Hierarchical Z from the Z cache.  
    if ((!disableHZUpdate) && zCache->updateHZ(hzBlock, hzBlockZ))
    {
        GPU_DEBUG(
            printf("ZStencilTest => Sending update to Hierarchical Z.  Block %x Z %x\n",
                hzBlock, hzBlockZ);
        )

        //  Send update to the Hierarchical Z.  
        hzUpdate->write(cycle, new cmHZUpdate.hzBlock, hzBlockZ));

        //  Update statistics.  
        updatesHZ->inc();
    }

    //  Check if a memory transaction was generated.  
    if (memTrans != NULL)
    {
        //  Send memory transaction to the Memory Controller.  
        memRequest->write(cycle, memTrans);

        //  Update statistics.  
        switch(memTrans->getCommand())
        {
            case MT_READ_REQ:

                readTrans->inc();
                break;

            case MT_WRITE_DATA:

                writeTrans->inc();
                writeBytes->inc(memTrans->getSize());
                break;

            default:
                CG_ASSERT("Unknown memory transaction.");
        }
    }

    //  Receive state from Color Write.  
    //if (colorWriteState[COLOR_WRITE]->read(cycle, (DynamicObject *&) cwsInfo))
    //{
        //  Get color write state.  
    //    cwState = cwsInfo->getState();

        //  Delete state container object.  
    //    delete cwsInfo;
    //}
    //else
    //{
    //    CG_ASSERT("Missing state signal from ColorWrite.");
    //}

    //  Receive state from Fragment FIFO.  
    if (colorWriteState[FRAGMENT_FIFO]->read(cycle, (DynamicObject *&) cwsInfo))
    {
        //  Check if early z is enabled.  
        if (earlyZ)
        {
            //  Get color write state.  
            cwState = cwsInfo->getState();
        }

        //  Delete state container object.  
        delete cwsInfo;
    }
    else
    {
        CG_ASSERT("Missing state signal from Fragment FIFO.");
    }

/*if ((cycle > 5024880) && (GPU_MOD(cycle, 1000) == 0))
{
printf("ZST %lld => state ", cycle);
switch(state)
{
    case RAST_RESET:    printf("RESET\n"); break;
    case RAST_READY:    printf("READY\n"); break;
    case RAST_DRAWING:  printf("DRAWING\n"); break;
    case RAST_END:      printf("END\n"); break;
}
printf("ZST => fetchStamps %d readStamps %d writeStamps %d freeStamps %d\n",
fetchStamps, readStamps, writeStamps, freeStamps);
}*/


    //  Simulate current cycle.  
    switch(state)
    {
        case RAST_RESET:

            //  Reset state.  

            GPU_DEBUG(
                printf("ZStencilTest => Reset state.\n");
            )

            //  Reset the Z cache.  
            zCache->reset();
            zCache->update(cycle, memoryState);

            //  Reset Z Stencil registers to default values.  
            hRes = 400;
            vRes = 400;
            startX = 0;
            startY = 0;
            width = 400;
            height = 400;
            clearDepth = 0x00ffffff;
            clearStencil = 0x00;
            earlyZ = TRUE;
            //earlyZ = FALSE;
            modifyDepth = FALSE;
            stencilTest = FALSE;
            stencilFunction = GPU_ALWAYS;
            stencilReference = 0x00;
            stencilTestMask = 0xff;
            stencilUpdateMask = 0xff;
            stencilFail = STENCIL_KEEP;
            depthFail = STENCIL_KEEP;
            depthPass = STENCIL_KEEP;
            zTest = FALSE;
            depthFunction = GPU_LESS;
            depthMask =  TRUE;

            //  By default set Z buffer at 4 MB.  
            zBuffer = 0x600000;

            //  Set Z buffer address in the Z cache.  
            zCache->swap(zBuffer);

            //  Reset free stamps counter.  
            freeStamps = zQueueSize;

            //  Reset test start cycle counter.  
            testCycles = 0;

            //  Change state to ready.  
            state = RAST_READY;

            break;

        case RAST_READY:

            //  Ready state.  

            GPU_DEBUG(
                printf("ZStencilTest => Ready state.\n");
            )

            //  Receive and process a rasterizer command.  
            if(rastCommand->read(cycle, (DynamicObject *&) rastComm))
                processCommand(rastComm, cycle);

            break;

        case RAST_DRAWING:

            //  Draw state.  

            GPU_DEBUG(
                printf("ZStencilTest => Drawing state.\n");
            )

            //  Check if there are free z queue entries.  
            if (freeStamps >= stampsCycle)
            {
                //  Receive stamps from Z Test mdu.  
                for(i = 0; i < stampsCycle; i++)
                {
                    //  Allocate the current stamp.  
                    stamp = new FragmentInput*[STAMP_FRAGMENTS];

                    //  Reset received fragment flag.  
                    receivedFragment = TRUE;

                    //  Receive fragments in a stamp.  
                    for(j = 0; (j < STAMP_FRAGMENTS) && receivedFragment; j++)
                    {
                        //  Get fragment from Z Test.  
                        receivedFragment = fragments->read(cycle,
                            (DynamicObject *&) frInput);

                        //  Check if a fragment has been received.  
                        if (receivedFragment)
                        {
                            //  Store fragment in the current stamp.  
                            stamp[j] = frInput;

                            //  Count triangles.  
                            if (frInput->getTriangleID() != currentTriangle)
                            {
                                //  Change current triangle.  
                                currentTriangle = frInput->getTriangleID();

                                //  Update triangle counter.  
                                triangleCounter++;
                            }

                            //  Update fragment counter.  
                            fragmentCounter++;

                            //  Update statistics.  
                            inputs->inc();
                        }
                    }

                    //  Check if a whole stamp has been received.  
                    CG_ASSERT_COND(!(!receivedFragment && (j > 1)), "Missing fragments in a stamp.");
                    //  Check if a stamp has been received.  
                    if (receivedFragment)
                    {
                        //  Process the received stamp.  
                        processStamp();
                    }
                    else
                    {
                        //  Delete the stamp.  
                        delete[] stamp;
                    }
                }
            }

            //  Check if there are stamps waiting to be fetched.  
            if (fetchStamps > 0)
            {
                /*  Check if z or stencil test are enabled and it is not
                    the last stamp.  */
                if ((zTest || stencilTest))
                {
                    //  Try to fetch the next stamp in the queue.  
                    if (zCache->fetch(zQueue[fetchZ].address,
                        zQueue[fetchZ].way, zQueue[fetchZ].line,
                        zQueue[fetchZ].stamp[0]))
                    {
                        GPU_DEBUG(
                            printf("ZStencilTest => Fetched stamp at %x to cache line (%d, %d)\n",
                                zQueue[fetchZ].address, zQueue[fetchZ].way,
                                zQueue[fetchZ].line);
                        )

                        //  Update fetch stamp counter.  
                        fetchStamps--;

                        //  Update read stamps counter.  
                        readStamps++;

                        //  Move the fetch pointer.  
                        fetchZ = GPU_MOD(fetchZ + 1, zQueueSize);

                        //  Update statistics.  
                        fetchOK->inc();
                    }
                    else
                    {
                        //  Update statistics.  
                        fetchFail->inc();
                    }
                }
                else
                {
                    GPU_DEBUG(
                        printf("ZStencilTest => Passing stamp to Color Write.\n");
                    )

                    //  Update fetch stamps counter.  
                    fetchStamps--;

                    //  Update fetch pointer.  
                    fetchZ = GPU_MOD(fetchZ + 1, zQueueSize);

                    //  Update write stamps counter.  
                    colorStamps++;
                }
            }

            //  Update cycles until the next stamp can be tested.  
            if (testCycles > 0)
            {
                testCycles--;
            }

            //  Check if there are stamps waiting to be read.  
            if ((stencilTest || zTest) && (readStamps > 0) && (testCycles == 0))
            {
                //  Reset wait flag.  
                waitWrite = FALSE;

                /*  Check if the same stamp is tested ahead in the
                    queue.  */
                for(i = writeZ; (i != readZ);
                    i = GPU_MOD(i + 1, zQueueSize))
                {
                    //  Check stamp address.  
                    if (zQueue[i].address == zQueue[readZ].address)
                    {
                        GPU_DEBUG(
                            printf("ZStencilTest => Reading a stamp at %x before writing it.\n",
                                zQueue[readZ].address);
                        )

                        //  Wait for the stamp to be written.  
                        waitWrite = TRUE;
                    }
                }

                //  Check if it must wait to a write.  
                if (!waitWrite)
                {
                    //  Try to read the next stamp in the queue.  
                    if (zCache->read(zQueue[readZ].address,
                        zQueue[readZ].way, zQueue[readZ].line,
                        (U08 *) zQueue[readZ].zData))
                    {
                        GPU_DEBUG(
                            printf("ZStencilTest => Reading stamp at %x\n",
                                zQueue[readZ].address);
                        )

                        //  Create blend operation object.  
                        zStencilOp = new ZOperation(readZ);

                        //  Copy parent command cookies.  
                        zStencilOp->copyParentCookies(*zQueue[readZ].stamp[0]);

                        //  Remove the fragment level cookie.  
                        zStencilOp->removeCookie();

                        //  Start test operation.  
                        testStart->write(cycle, zStencilOp);

                        //  Set start test cycle counter.  
                        testCycles = testRate;

                        //  Update read stamps counter.  
                        readStamps--;

                        //  Move the read pointer.  
                        readZ = GPU_MOD(readZ + 1, zQueueSize);

                        //  Update statistics.  
                        readOK->inc();
                    }
                    else
                    {
                        //  Update statistics.  
                        readFail->inc();
                    }
                }
                else
                {
                    //  Update statistics.  
                    rawDep->inc();
                }
            }

            //  Check end of test operation.  
            if (testEnd->read(cycle, (DynamicObject *&) zStencilOp))
            {
                //  Get operation entry.  
                zOperation = zStencilOp->getID();

                GPU_DEBUG(
                    printf("ZStencilTest => Z and Stencil testing stamp.\n");
                )

//if ((cycle > 5710000) && (cycle < 5750000))
//printf("ZST %lld => inDepth %x %x %x %x zData %x %x %x %x\n", cycle,
//zQueue[zOperation].inDepth[0], zQueue[zOperation].inDepth[1], zQueue[zOperation].inDepth[2], zQueue[zOperation].inDepth[3],
//zQueue[zOperation].zData[0], zQueue[zOperation].zData[1], zQueue[zOperation].zData[2], zQueue[zOperation].zData[3]);

//if ((cycle > 5710000) && (cycle < 5750000))
//printf("ZST %lld => culled (IN) %d %d %d %d\n", cycle, zQueue[zOperation].culled[0], zQueue[zOperation].culled[1],
//zQueue[zOperation].culled[2], zQueue[zOperation].culled[3]);

                //  Perform Stencil and Z tests.  
                frEmu.stencilZTest(zQueue[zOperation].inDepth,
                    zQueue[zOperation].zData,
                    zQueue[zOperation].culled);

//if ((cycle > 5710000) && (cycle < 5750000))
//printf("ZST %lld => culled (OUT) %d %d %d %d\n", cycle, zQueue[zOperation].culled[0], zQueue[zOperation].culled[1],
//zQueue[zOperation].culled[2], zQueue[zOperation].culled[3]);

                //  Delete Z Stencil test object.  
                delete zStencilOp;

                //  Upddate write stamps counter.  
                writeStamps++;

                //  Update statistics.  
                tested->inc(STAMP_FRAGMENTS);
                for(i = 0; i < STAMP_FRAGMENTS; i++)
                {
                    if (zQueue[zOperation].culled[i])
                        failed->inc();
                    else
                        passed->inc();
                }
            }

            //  Check if there are stamps waiting to be written.  
            if (writeStamps > 0)
            {
                //  Try to write stamp.  
                if (zCache->write(
                    zQueue[writeZ].address,
                    zQueue[writeZ].way,
                    zQueue[writeZ].line,
                    (U08 *) zQueue[writeZ].zData))
                {
                    GPU_DEBUG(
                        printf("ZStencilTest => Writing stamp at %x\n",
                           zQueue[writeZ].address);
                    )

                    //  Update write stamps counter.  
                    writeStamps--;

                    //  Update number of stamps to send to Color Write.  
                    colorStamps++;

                    //  Move write pointer.  
                    writeZ = GPU_MOD(writeZ + 1, zQueueSize);

                    //  Update statistics.  
                    writeOK->inc();
                }
                else
                {
                    //  Update statistics.  
                    writeFail->inc();
                }
            }

            //  Check end of the batch.  
            if (lastFragment && (freeStamps == zQueueSize) && (memTrans == NULL))
            {
                GPU_DEBUG(
		            printf("ZStencilTest (%p) => Sending last stamp to %s from position %d\n",
		            this, fragmentDestination==FRAGMENT_FIFO?"FFIFO":"COLOR WRITE", colorZ);
		        )

		//  Send empty last stamp to Color Write.  
                for(i = 0; i < STAMP_FRAGMENTS; i++)
                {
                    //  Send stamp to Color Write.  
                    colorWrite[fragmentDestination]->write(cycle, zQueue[colorZ].stamp[i]);

                    //  Update statistics.  
                    outputs->inc();
                }

                //  Delete the stamp.  
                delete[] zQueue[colorZ].stamp;

                //  Change state to end.  
                state = RAST_END;
            }

            //  Send all the processed stamps to Color Write.  
            if ((colorStamps > 0) && (cwState == CWS_READY))
            {
                /*  NOTE:  STAMPS TO BE SENT TO COLOR WRITE MUST COME IN ORDER
                    THIS MEANS THAT IT MAY NOT WORK IF BATCHES WITH Z-STENCIL
                    ENABLED AND BATCHES WITH Z-STENCIL DISABLED ARE PIPELINED.  */

                //  Update fragment culled flags.  
                for(i = 0; i < STAMP_FRAGMENTS; i++)
                {
                    //  Set the fragment new cull value.  
                    zQueue[colorZ].stamp[i]->setCull(zQueue[colorZ].culled[i]);

                    //  Send stamp to Color Write.  
                    colorWrite[fragmentDestination]->write(cycle, zQueue[colorZ].stamp[i]);

                    //  Update statistics.  
                    outputs->inc();
                }

                //  Delete the stamp.  
                delete[] zQueue[colorZ].stamp;

                //  Update pointer to the next stamp to send to color write.  
                colorZ = GPU_MOD(colorZ + 1, zQueueSize);

                //  Update number of stamps waiting to be sent to color write.  
                colorStamps--;

                //  Update number of free z queue entries.  
                freeStamps++;
            }

            break;


        case RAST_END:

            //  End of batch state.  

            GPU_DEBUG(
                printf("ZStencilTest => End state.\n");
            )

            //  Wait for end command.  
            if(rastCommand->read(cycle, (DynamicObject *&) rastComm))
                processCommand(rastComm, cycle);

            break;

        case RAST_SWAP:

            //  Swap buffer state.  

            GPU_DEBUG(
                printf("ZStencilTest => Swap state.\n");
            )

            CG_ASSERT("Z SWAP STATE NOT IMPLEMENTED!!!");

            break;

        case RAST_CLEAR:

            //  Clear Z and stencil buffer state.  

            GPU_DEBUG(
                printf("ZStencilTest => Clear state.\n");
            )

            //  Check if the Z cache has finished the clear.  
            if (!zCache->clear(clearDepth, clearStencil))
            {
                //  Change to end state.  
                state = RAST_END;
            }

            break;

        default:

            CG_ASSERT("Unsupported state.");
            break;

    }

    /*  NOTE: RESERVE BUFFER FOR ON THE FLY FRAGMENTS IN THE
        INTERPOLATOR (INTERPOLATOR LATENCY = 8!!).  ONLY FOR
        CURRENT TEST IMPLEMENTATION.  */

    //  Send current state to Fragment FIFO (Interpolator).  
    if (freeStamps >= ((2 + 8) * stampsCycle))
    {
        GPU_DEBUG(
            printf("ZStencilTest => Sending READY.\n");
        )

        //  Send a ready signal.  
        zStencilState->write(cycle, new ZStencilStatusInfo(ZST_READY));
    }
    else
    {
        GPU_DEBUG(
            printf("ZStencilTest => Sending BUSY.\n");
        )

        //  Send a busy signal.  
        zStencilState->write(cycle, new ZStencilStatusInfo(ZST_BUSY));
    }

    //  Send current rasterizer state.  
    rastState->write(cycle, new RasterizerStateInfo(state));
}


//  Processes a rasterizer command.  
void ZStencilTest::processCommand(RasterizerCommand *command, U64 cycle)
{

    //  Delete last command.  
    delete lastRSCommand;

    //  Store current command as last received command.  
    lastRSCommand = command;

    //  Process command.  
    switch(command->getCommand())
    {
        case RSCOM_RESET:
            //  Reset command from the Rasterizer main mdu.  

            GPU_DEBUG(
                printf("ZStencilTest => RESET command received.\n");
            )

            //  Change to reset state.  
            state = RAST_RESET;

            break;

        case RSCOM_DRAW:
            //  Draw command from the Rasterizer main mdu.  

            GPU_DEBUG(
                printf("ZStencilTest %lld => DRAW command received.\n", cycle);
            )

             //  Check current state.  
            CG_ASSERT_COND(!(state != RAST_READY), "DRAW command can only be received in READY state.");
            //  Configure Z test emulation.  
            frEmu.configureZTest(depthFunction, depthMask);

            frEmu.setZTest(zTest);

            //  Configure stencil test emulation.  
            frEmu.configureStencilTest(stencilFunction, stencilReference,
                stencilTestMask, stencilUpdateMask, stencilFail,
                depthFail, depthPass);

            frEmu.setStencilTest(stencilTest);

            //  Reset next free entry in the Z queue entry.  
            freeZ = 0;

            //  Reset next stamp to fetch pointer.  
            fetchZ = 0;

            //  Reset next stamp to read pointer.  
            readZ = 0;

            //  Reset next stamp to write pointer.  
            writeZ = 0;

            //  Reset next stamp to be sent to Color Write.  
            colorZ = 0;

            //  Reset free Z queue entries counter.  
            freeStamps = zQueueSize;

            //  Reset fetch stamps counter.  
            fetchStamps = 0;

            //  Reset read stamps counter.  
            readStamps = 0;

            //  Reset write stamps counter.  
            writeStamps = 0;

            //  Reset total number of stamps in the z queue counter.  
            testStamps = 0;

            //  Reset number of stamps in the z queue waiting to be sent to Color Write.  
            colorStamps = 0;

            //  Reset triangle counter.  
            triangleCounter = 0;

            //  Reset batch fragment counter.  
            fragmentCounter = 0;

            //  Reset previous processed triangle.  
            currentTriangle = (U32) -1;

            //  Last batch fragment no received yet.  
            lastFragment = FALSE;

            //  Set where the fragments are going to go.  
            fragmentDestination = earlyZ?FRAGMENT_FIFO:COLOR_WRITE;

            //  Change to drawing state.  
            state = RAST_DRAWING;

            break;

        case RSCOM_END:
            //  End command received from Rasterizer main mdu.  

            GPU_DEBUG(
                printf("ZStencilTest => END command received.\n");
            )

            //  Check current state.  
            CG_ASSERT_COND(!(state != RAST_END), "END command can only be received in END state.");
            //  Change to ready state.  
            state = RAST_READY;

            break;

        case RSCOM_REG_WRITE:
            //  Write register command from the Rasterizer main mdu.  

            GPU_DEBUG(
                printf("ZStencilTest => REG_WRITE command received.\n");
            )

             //  Check current state.  
            CG_ASSERT_COND(!(state != RAST_READY), "REGISTER WRITE command can only be received in READY state.");
            //  Process register write.  
            processRegisterWrite(command->getRegister(),
                command->getSubRegister(), command->getRegisterData());

            break;

        case RSCOM_SWAP:
            //  Swap front and back buffer command.  

            GPU_DEBUG(
                printf("ZStencilTest => SWAP command received.\n");
            )

            //  Check current state.  
            CG_ASSERT_COND(!(state != RAST_READY), "SWAP command can only be received in READY state.");
            //  Update frame counter.  
            frameCounter++;

            //  Reset Z cache flush flag.  
            endFlush = FALSE;

            //  Change state to swap state.  
            state = RAST_SWAP;

            break;

        case RSCOM_CLEAR_ZSTENCIL_BUFFER:
            //  Clear Z and stencil buffer command.  

            /*  !!! ONLY SUPPORTED CLEARING BOTH THE Z AND STENCIL BUFFER
                AT THE SAME TIME !!!  */


            GPU_DEBUG(
                printf("ZStencilTest => CLEAR_ZSTENCIL_BUFFER command received.\n");
            )

            //  Check current state.  
            CG_ASSERT_COND(!(state != RAST_READY), "CLEAR command can only be received in READY state.");
            //  Clear the Z cache as any info there will be invalid now.  
            zCache->clear(clearDepth, clearStencil);

            //  Change to clear state.  
            state = RAST_CLEAR;

            break;

        default:
            CG_ASSERT("Unsupported command.");
            break;
    }
}

//  Processes a gpu register write.  
void ZStencilTest::processRegisterWrite(GPURegister reg, U32 subreg,
    GPURegData data)
{
    //  Process register write.  
    switch(reg)
    {
        case GPU_DISPLAY_X_RES:
            //  Write display horizontal resolution register.  
            hRes = data.uintVal;

            GPU_DEBUG(
                printf("ZStencilTest => Write GPU_DISPLAY_X_RES = %d.\n", hRes);
            )

            break;

        case GPU_DISPLAY_Y_RES:
            //  Write display vertical resolution register.  
            vRes = data.uintVal;

            GPU_DEBUG(
                printf("ZStencilTest => Write GPU_DISPLAY_Y_RES = %d.\n", vRes);
            )

            break;

        case GPU_VIEWPORT_INI_X:
            //  Write viewport initial x coordinate register.  
            startX = data.intVal;

            GPU_DEBUG(
                printf("ZStencilTest => Write GPU_VIEWPORT_INI_X = %d.\n", startX);
            )

            break;

        case GPU_VIEWPORT_INI_Y:
            //  Write viewport initial y coordinate register.  
            startY = data.intVal;

            GPU_DEBUG(
                printf("ZStencilTest => Write GPU_VIEWPORT_INI_Y = %d.\n", startY);
            )

            break;

        case GPU_VIEWPORT_WIDTH:
            //  Write viewport width register.  
            width = data.uintVal;

            GPU_DEBUG(
                printf("ZStencilTest => Write GPU_VIEWPORT_WIDTH = %d.\n", width);
            )

            break;

        case GPU_VIEWPORT_HEIGHT:
            //  Write viewport height register.  
            height = data.uintVal;

            GPU_DEBUG(
                printf("ZStencilTest => Write GPU_VIEWPORT_HEIGHT = %d.\n", height);
            )

            break;

        case GPU_Z_BUFFER_CLEAR:
            //  Write depth clear value.  
            clearDepth = data.uintVal;

            GPU_DEBUG(
                printf("ZStencilTest => Write GPU_Z_BUFFER_CLEAR = %x.\n", clearDepth);
            )

            break;

        case GPU_STENCIL_BUFFER_CLEAR:

            //  Write stencil clear value.  
            clearStencil = (U08) (data.uintVal & 0xff);

            GPU_DEBUG(
                printf("ZStencilTest => Write GPU_STENCIL_BUFFER_CLEAR = %x.\n", clearStencil);
            )

            break;

        case GPU_Z_BUFFER_BIT_PRECISSION:

            //  Write z buffer bit precission register.  
            depthPrecission = data.uintVal;

            GPU_DEBUG(
                printf("ZStencilTest => Write GPU_Z_BUFFER_BIT_PRECISSION = %x.\n", depthPrecission);
            )

            break;

        case GPU_ZSTENCILBUFFER_ADDR:
            //  Write z and stencil buffer address register.  
            zBuffer = data.uintVal;

            //  Set Z buffer address in the Z cache.  
            zCache->swap(zBuffer);

            GPU_DEBUG(
                printf("ZStencilTest => Write GPU_ZSTENCILBUFFER_ADDR = %x.\n",
                    zBuffer);
            )

            break;

        case GPU_EARLYZ:

            //  Set early Z test flag.  
            //earlyZ = data.booleanVal;

            GPU_DEBUG(
                printf("ZStencilTest => Write GPU_EARLY_Z = %s.\n",
                    data.booleanVal?"ENABLED":"DISABLED");
            )

            break;

        case GPU_MODIFY_FRAGMENT_DEPTH:

            //  Write fragment shader modifies depth flag.  
            modifyDepth = data.booleanVal;

            GPU_DEBUG(
                printf("ZStencilTest => Write GPU_MODIFY_FRAGMENT_DEPTH = %s\n",
                    modifyDepth?"TRUE":"FALSE");
            )

            break;

        case GPU_STENCIL_TEST:

            //  Write stencil enabla flag register.  
            stencilTest = data.booleanVal;

            GPU_DEBUG(
                printf("ZStencilTest => Write GPU_STENCIL_TEST = %s.\n",
                    stencilTest?"TRUE":"FALSE");
            )

            break;

        case GPU_STENCIL_FUNCTION:

            //  Write Stencil function register.  
            stencilFunction = data.compare;

            GPU_DEBUG(
                printf("ZStencilTest => Write GPU_STENCIL_FUNCTION = ");

                switch(stencilFunction)
                {
                    case GPU_NEVER:
                        printf("NEVER.\n");
                        break;

                    case GPU_ALWAYS:
                        printf("ALWAYS.\n");
                        break;

                    case GPU_LESS:
                        printf("LESS.\n");
                        break;

                    case GPU_LEQUAL:
                        printf("LEQUAL.\n");
                        break;

                    case GPU_EQUAL:
                        printf("EQUAL.\n");
                        break;

                    case GPU_GEQUAL:
                        printf("GEQUAL.\n");
                        break;

                    case GPU_GREATER:
                        printf("GREATER.\n");
                        break;

                    case GPU_NOTEQUAL:
                        printf("NOTEQUAL.\n");

                    default:
                        CG_ASSERT("Undefined compare function mode");
                        break;
                }
            )

            break;

        case GPU_STENCIL_REFERENCE:

            //  Write stencil reference value register.  
            stencilReference = (U08) (data.uintVal & 0xff);

            GPU_DEBUG(
                printf("ZStencilTest => Write GPU_STENCIL_REFERENCE = %x.\n", stencilReference);
            )

            break;

        case GPU_STENCIL_COMPARE_MASK:

            //  Write stencil compare mask.  
            stencilTestMask = (U08) (data.uintVal & 0xff);

            GPU_DEBUG(
                printf("ZStencilTest => Write GPU_STENCIL_COMPARE_MASK = %x.\n", stencilTestMask);
            )

            break;

        case GPU_STENCIL_UPDATE_MASK:

            //  Write stencil update mask.  
            stencilUpdateMask = (U08) (data.uintVal & 0xff);

            GPU_DEBUG(
                printf("ZStencilTest => Write GPU_STENCIL_UPDATE_MASK = %x.\n", stencilUpdateMask);
            )

            break;

        case GPU_STENCIL_FAIL_UPDATE:

            //  Write stencil fail update function.  
            stencilFail = data.stencilUpdate;

            GPU_DEBUG(
                printf("ZStencilTest => Write GPU_STENCIL_FAIL_UPDATE = ");

                switch(stencilFail)
                {
                    case STENCIL_KEEP:
                        printf("KEEP.\n");
                        break;

                    case STENCIL_ZERO:
                        printf("ZERO.\n");
                        break;

                    case STENCIL_REPLACE:
                        printf("REPLACE.\n");
                        break;

                    case STENCIL_INCR:
                        printf("INCREMENT.\n");
                        break;

                    case STENCIL_DECR:
                        printf("DECREMENT.\n");
                        break;

                    case STENCIL_INVERT:
                        printf("INVERT.\n");
                        break;

                    case STENCIL_INCR_WRAP:
                        printf("INCREMENT AND WRAP.\n");
                        break;

                    case STENCIL_DECR_WRAP:
                        printf("DECREMENT AND WRAP.\n");
                        break;

                    default:
                        CG_ASSERT("Undefined stencil update function");
                        break;
                }
            )

            break;

        case GPU_DEPTH_FAIL_UPDATE:

            //  Write depth fail update function.  
            depthFail = data.stencilUpdate;

            GPU_DEBUG(
                printf("ZStencilTest => Write GPU_DEPTH_FAIL_UPDATE = ");

                switch(depthFail)
                {
                    case STENCIL_KEEP:
                        printf("KEEP.\n");
                        break;

                    case STENCIL_ZERO:
                        printf("ZERO.\n");
                        break;

                    case STENCIL_REPLACE:
                        printf("REPLACE.\n");
                        break;

                    case STENCIL_INCR:
                        printf("INCREMENT.\n");
                        break;

                    case STENCIL_DECR:
                        printf("DECREMENT.\n");
                        break;

                    case STENCIL_INVERT:
                        printf("INVERT.\n");
                        break;

                    case STENCIL_INCR_WRAP:
                        printf("INCREMENT AND WRAP.\n");
                        break;

                    case STENCIL_DECR_WRAP:
                        printf("DECREMENT AND WRAP.\n");
                        break;

                    default:
                        CG_ASSERT("Undefined stencil update function");
                        break;
                }
            )

            break;

        case GPU_DEPTH_PASS_UPDATE:

            //  Write depth pass stencil update function.  
            depthPass = data.stencilUpdate;

            GPU_DEBUG(
                printf("ZStencilTest => Write GPU_DEPTH_PASS_UPDATE = ");

                switch(depthPass)
                {
                    case STENCIL_KEEP:
                        printf("KEEP.\n");
                        break;

                    case STENCIL_ZERO:
                        printf("ZERO.\n");
                        break;

                    case STENCIL_REPLACE:
                        printf("REPLACE.\n");
                        break;

                    case STENCIL_INCR:
                        printf("INCREMENT.\n");
                        break;

                    case STENCIL_DECR:
                        printf("DECREMENT.\n");
                        break;

                    case STENCIL_INVERT:
                        printf("INVERT.\n");
                        break;

                    case STENCIL_INCR_WRAP:
                        printf("INCREMENT AND WRAP.\n");
                        break;

                    case STENCIL_DECR_WRAP:
                        printf("DECREMENT AND WRAP.\n");
                        break;

                    default:
                        CG_ASSERT("Undefined stencil update function");
                        break;
                }
            )

            break;

        case GPU_DEPTH_TEST:

            //  Write depth test enable register.  
            zTest = data.booleanVal;

            GPU_DEBUG(
                printf("ZStencilTest => Write GPU_DEPTH_TEST = %s.\n",
                    zTest?"TRUE":"FALSE");
            )

            break;

        case GPU_DEPTH_FUNCTION:

            //  Write depth compare function register.  
            depthFunction = data.compare;

            GPU_DEBUG(
                printf("ZStencilTest => Write GPU_DEPTH_FUNCTION = ");

                switch(depthFunction)
                {
                    case GPU_NEVER:
                        printf("NEVER.\n");
                        break;

                    case GPU_ALWAYS:
                        printf("ALWAYS.\n");
                        break;

                    case GPU_LESS:
                        printf("LESS.\n");
                        break;

                    case GPU_LEQUAL:
                        printf("LEQUAL.\n");
                        break;

                    case GPU_EQUAL:
                        printf("EQUAL.\n");
                        break;

                    case GPU_GEQUAL:
                        printf("GEQUAL.\n");
                        break;

                    case GPU_GREATER:
                        printf("GREATER.\n");
                        break;

                    case GPU_NOTEQUAL:
                        printf("NOTEQUAL.\n");
                        break;

                    default:
                        CG_ASSERT("Undefined compare function mode");
                        break;
                }
            )

            break;

        case GPU_DEPTH_MASK:

            //  Write depth update mask register.  
            depthMask = data.booleanVal;

            GPU_DEBUG(
                printf("ZStencilTest => Write GPU_DEPTH_MASK = %s.\n",
                    depthMask?"TRUE":"FALSE");
            )

            break;

        default:
            CG_ASSERT("Unsupported register.");
            break;
    }

}

//  Processes a stamp.  
void ZStencilTest::processStamp()
{
    Fragment *fr;
    Vec4FP32 *attrib;
    U32 address;
    S32 x, y;
    U32 i;
    bool cullStamp;

    //  Keep the stamp in the queue.  
    zQueue[freeZ].stamp = stamp;

    //  Get first stamp fragment.  
    fr = stamp[0]->getFragment();

    //  NOTE: ALL THE FRAGMENTS IN THE LAST STAMP MUST BE NULL.  

    //  Check if first fragment exists (not last fragment).  
    if (fr != NULL)
    {
        /*  !!! NOTE:  Z BUFFER AND COLOR BUFFER COULD USE DIFFERENT MEMORY LAYOUTS, SO THEY MAY HAVE
            TO USE DIFFERENT FUNCTIONS TO CALCULATE THE ADDRESS.

            IF IT IS ALWAYS THE SAME ADDRESS IT WOULD BE BETTER TO CALCULATE THE ADDRESS AT RASTERIZATION
            AND STORE IT AS A FRAGMENT PARAMETER.
        */

        //  Get fragment attributes.  
        attrib = stamp[0]->getAttributes();

        //  Get fragment position.  
        //x = (S32) attrib[POSITION_ATTRIBUTE][0];
        //y = (S32) attrib[POSITION_ATTRIBUTE][1];
        x = fr->getX();
        y = fr->getY();

        //  Calculate address of the first stamp fragment pixel in the Z buffer.  
        address = GPUMath::pixel2Memory(x, y, startX, startY, width, height,
            hRes, vRes, STAMP_WIDTH, STAMP_WIDTH, genW, genH, scanW, scanH, overW, overH, bytesPixel);

        //  Add the current Z buffer base address.  
        address += zBuffer;

        //  Set Z buffer address for the stamp.  
        zQueue[freeZ].address = address;

        //  Calculate offset inside a line cache.  
        zQueue[freeZ].offset = address & stampMask;

        //  Store the stamp fragment data in the Z queue entry.  
        for(i = 0; (i < STAMP_FRAGMENTS) && (!lastFragment); i++)
        {
            //  Get stamp fragment.  
            fr = stamp[i]->getFragment();

            //  Check if fragment is NULL.  
            CG_ASSERT_COND(!(fr == NULL), "NULL fragment not in last stamp.");
            //  Get fragment cull bit.  
            zQueue[freeZ].culled[i] = stamp[i]->isCulled();

            //  Check if the fragment is culled.  
            if (!zQueue[freeZ].culled[i])
            {
                //  Not last fragment and fragment inside the triangle.  

                //  Get fragment attributes.  
                attrib = stamp[i]->getAttributes();

                GPU_DEBUG(
                    //  Get fragment position.  
                    //x = (S32) attrib[POSITION_ATTRIBUTE][0];
                    //y = (S32) attrib[POSITION_ATTRIBUTE][1];
                    x = fr->getX();
                    y = fr->getY();
                    printf("ZStencilTest => Received Fragment Not Culled (%d, %d) Inside? %s.\n",
                        x, y, fr->isInsideTriangle()?"IN":"OUT");
                )

                //  Check if fragment depth was modified by the fragment shader.  
                if (modifyDepth)
                {
                    //  Convert modified fragment depth to integer format.  
                    zQueue[freeZ].inDepth[i] = bmRast.convertZ(GPU_CLAMP(attrib[POSITION_ATTRIBUTE][3], 0.0f, 1.0f));
                }
                else
                {
                    //  Queue the fragment in the blend queue.  
                    zQueue[freeZ].inDepth[i] = fr->getZ();
                }
            }
            else
            {
                //  Fragment culled.  
                GPU_DEBUG(
                    printf("ZStencilTest => Received Fragment Culled (%d, %d) Inside? %s.\n",
                        fr->getX(), fr->getY(),
                        fr->isInsideTriangle()?"IN":"OUT");
                )

                //  Eliminates a valgrind warning (data isn't used or is masked so it shouldn't affect the result).  
                zQueue[freeZ].inDepth[i] = fr->getZ();

                //  Update statistics.  
                outside->inc();
            }
        }

        //  Calculate if the stamp should be culled (all fragments masked).  
        for(i = 0, cullStamp = TRUE; i < STAMP_FRAGMENTS; i++)
            cullStamp = cullStamp && zQueue[freeZ].culled[i];

        //  Check if the whole stamp can be culled.  
        if (!cullStamp)
        {
            //  Update fetch stamp counter.  
            fetchStamps++;

            //  Update Z blend queue entries counter.  
            freeStamps--;

            //  Move pointer to the next Z queue free entry.  
            freeZ = GPU_MOD(freeZ + 1, zQueueSize);
        }
        else
        {
            GPU_DEBUG(
                printf("ZStencilTest => Culling whole stamp.\n");
            )

            //  Delete stamp.  
            for(i = 0; i < STAMP_FRAGMENTS; i++)
            {
                //  Get fragment.  
                fr = stamp[i]->getFragment();

                //  Get fragment attribute.  
                attrib = stamp[i]->getAttributes();

                //  Delete fragment.  
                delete fr;

                //  Delete attributes.  
                delete[] attrib;

                //  Delete fragment input.  
                delete stamp[i];

                //  Update statistics.  
                culled->inc();
            }

            //  Delete stamp.  
            delete[] stamp;
        }
    }
    else
    {
        //  Last fragment/stamp received.  
        lastFragment = TRUE;

        //  Set queue entry as last stamp.  
        zQueue[freeZ].lastStamp = TRUE;

        //  Copy empty last stamp to Z queue.  
        zQueue[freeZ].stamp = stamp;

        GPU_DEBUG(
	    printf("ZStencilTest (%p) => Received last stamp stored at %d\n", this, freeZ);
	)

	/*  NOTE:  THE LAST STAMP MUST ALWAYS BE THE LAST STAMP IN A BATCH.  IT IS STORED IN THE LAST
            QUEUE ENTRY BUT THE COUNTERS AND POINTERS ARE NOT UPDATED TO AVOID PROCESSING IT AT THE
            Z STENCIL TEST PIPELINE.  */

        //  Update fetch stamp counter.  
        //fetchStamps++;

        //  Update Z blend queue entries counter.  
        //freeStamps--;

        //  Move pointer to the next Z queue free entry.  
        //freeZ = GPU_MOD(freeZ + 1, zQueueSize);
    }
}


//  Processes a memory transaction.  
void ZStencilTest::processMemoryTransaction(U64 cycle,
    MemoryTransaction *memTrans)
{
    //  Get transaction type.  
    switch(memTrans->getCommand())
    {
        case MT_STATE:

            //  Received state of the Memory controller.  
            memoryState = memTrans->getState();

            GPU_DEBUG(
                printf("ZStencilTest => Memory Controller State = %d\n",
                    memoryState);
            )

            break;

        default:

            GPU_DEBUG(
                printf("ZStencilTest => Memory Transaction to the Z Cache.\n");
            )

            //  Update statistics.  
            readBytes->inc(memTrans->getSize());

            //  Let the z cache process any other transaction.  
            zCache->processMemoryTransaction(memTrans);

            break;
    }

    //  Delete processed memory transaction.  
    delete memTrans;
}

void ZStencilTest::getState(string &stateString)
{
    stringstream stateStream;

    stateStream.clear();

    stateStream << " state = ";

    switch(state)
    {
        case RAST_RESET:
            stateStream << "RAST_RESET";
            break;
        case RAST_READY:
            stateStream << "RAST_READY";
            break;
        case RAST_DRAWING:
            stateStream << "RAST_DRAW";
            break;
        case RAST_END:
            stateStream << "RAST_END";
            break;
        case RAST_CLEAR:
            stateStream << "RAST_CLEAR";
            break;
        case RAST_CLEAR_END:
            stateStream << "RAST_CLEAR_END";
            break;
        case RAST_SWAP:
            stateStream << "RAST_SWAP";
            break;
        default:
            stateStream << "undefined";
            break;
    }

    stateStream << " | Fragment Counter = " << fragmentCounter;
    stateStream << " | Triangle Counter = " << triangleCounter;
    stateStream << " | Last Fragment = " << lastFragment;
    stateStream << " | Fetch Stamps = " << fetchStamps;
    stateStream << " | Read Stamps = " << readStamps;
    stateStream << " | Test Stamps = " << testStamps;
    stateStream << " | Write Stamps = " << writeStamps;
    stateStream << " | Color Stamps = " << colorStamps;

    stateString.assign(stateStream.str());
}
