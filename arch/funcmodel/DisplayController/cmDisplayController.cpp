/**************************************************************************
 *
 * cmoDisplayController mdu implementation file.
 *
 */

 /**
 *
 *  @file cmoDisplayController.cpp
 *
 *  This file implements the cmoDisplayController mdu.
 *
 *  This class implements the cmoDisplayController mdu.  Consumes GPU memory
 *  bandwidth for displaying the current color buffer.  Outputs
 *  the color buffer to a file when a swap buffer signal is
 *  received.
 *
 */

#include "cmDisplayController.h"
#include "cmFragmentInput.h"
#include "cmRasterizerStateInfo.h"
#include "cmColorBlockStateInfo.h"
#include "GPUMath.h"
#include "bmFragmentOperator.h"
#include "DepthCompressor.h"
#include "ColorCompressor.h"
#include "ImageSaver.h"

using namespace arch;

//  cmoDisplayController constructor.  
cmoDisplayController::cmoDisplayController(U32 overWidth, U32 overHeight, U32 scanWidth, U32 scanHeight,
    U32 genWidth, U32 genHeight, U32 mortonBlockDim, U32 mortonSBlockDim, 
    U32 blockSz, U32 blockLatency, U32 blocksPerCycle, U32 blockQSize, U32 decompLatency, 
    U32 nStampUnits, char **suPrefixes, U32 startF, U64 refresh, bool synched, bool refFrame,
    bool saveBlitSourceData, char *name, cmoMduBase *parent) :

    stateBlocks(getSM().getNumericStatistic("ColorStateBlocksTotal", U32(0), "cmoDisplayController", "cmoDisplayController")),
    stateBlocksClear(getSM().getNumericStatistic("ColorStateBlocksClear", U32(0), "cmoDisplayController", "cmoDisplayController")),
    stateBlocksUncompressed(getSM().getNumericStatistic("ColorStateBlocksUncompressed", U32(0), "cmoDisplayController", "cmoDisplayController")),
    stateBlocksCompressedBest(getSM().getNumericStatistic("ColorStateBlocksCompressedBest", U32(0), "cmoDisplayController", "cmoDisplayController")),
    stateBlocksCompressedNormal(getSM().getNumericStatistic("ColorStateBlocksCompressedNormal", U32(0), "cmoDisplayController", "cmoDisplayController")),
    stateBlocksCompressedWorst(getSM().getNumericStatistic("ColorStateBlocksCompressedWorst", U32(0), "cmoDisplayController", "cmoDisplayController")),

    overH(overHeight), overW(overWidth), scanH((U32) ceil(scanHeight / (F32) genHeight)),
    scanW((U32) ceil(scanWidth / (F32) genWidth)), genH(genHeight / STAMP_HEIGHT), genW(genWidth / STAMP_WIDTH),
    blockSize(blockSz), blocksCycle(blocksPerCycle),
    blockStateLatency(blockLatency), blockQueueSize(blockQSize), decompressLatency(decompLatency),
    numStampUnits(nStampUnits), startFrame(startF), refreshRate(refresh), synchedRefresh(synched), refreshFrame(refFrame),
    saveBlitSourceData(saveBlitSourceData),
    bltTransImpl(*this), cmoMduBase(name, parent)

{
    DynamicObject *defaultState[1];
    U32 i;

    //  Check parameters.  
    GPU_ASSERT(
        if (overH == 0)
            CG_ASSERT("Over scan tile height (in scan tiles) must be at least 1.");
        if (overW == 0)
            CG_ASSERT("Over scan tile width (in scan tiles) must be at least 1.");
        if (scanHeight < genHeight)
            CG_ASSERT("The scan tile height (in fragments) must be at least the generation tile height.");
        if (scanWidth < genWidth)
            CG_ASSERT("The scan tile width (in fragments) must be at least the generation tile width.");
        if (scanH == 0)
            CG_ASSERT("Scan tile height (in generation tiles) must be at least 1.");
        if (scanW == 0)
            CG_ASSERT("Scan tile width (in generation tiles) must be at least 1.");
        if (genHeight < STAMP_HEIGHT)
            CG_ASSERT("The generation tile height (in fragments) must be at least the stamp tile height.");
        if (genWidth < STAMP_WIDTH)
            CG_ASSERT("The generation tile width (in fragments) must be at least the stamp tile width.");
        if (genH == 0)
            CG_ASSERT("Generation tile height (in stamps) must be at least 1.");
        if (genW == 0)
            CG_ASSERT("Generation tile width (in stamps) must be at least 1.");
        if (blockSize == 0)
            CG_ASSERT("The (uncompressed) block size can not be 0.");
        //if (blockSize < (genWidth * genHeight * bytesPixel))
        //    CG_ASSERT("The (uncompressed) block size must be a multiple of the generation tile size.");
        //if (GPU_MOD(blockSize, (genWidth * genHeight * bytesPixel)) != 0)
        //    CG_ASSERT("The (uncompressed) block size must be a multiple of the generation tile size.");
        if (blocksCycle == 0)
            CG_ASSERT("At least one color block state should be updated per cycle.");
        if (blockStateLatency == 0)
            CG_ASSERT("Block state update signal latency must be at least 1.");
        if (blockQueueSize == 0)
            CG_ASSERT("Block queue requires at least one entry.");
        if (decompressLatency == 0)
            CG_ASSERT("Decompress unit latency must be at least 1.");
        if (numStampUnits == 0)
            CG_ASSERT("At least one stamp pipe required.");
    )

    //  Create signals.  

    //  Create signals with the Command Processor.  

    //  Create command signal from Command Processor.  
    dacCommand = newInputSignal("DACCommand", 1, 1, NULL);

    //  Create state signal to Command Processor.  
    dacState = newOutputSignal("DACState", 1, 1, NULL);

    //  Create default signal value.  
    defaultState[0] = new RasterizerStateInfo(RAST_RESET);

    //  Set default signal value.  
    dacState->setData(defaultState);

    //  Create signals with Memory Controller mdu.  

    //  Create request signal to the Memory Controller.  
    memoryRequest = newOutputSignal("DisplayControllerMemoryRequest", 1, 1);

    //  Create data signal from the Memory Controller.  
    memoryData = newInputSignal("DisplayControllerMemoryData", 2, 1);


    //  Create signals with Color Write units  

    //  Check number of attached Color WRite units.  
    GPU_ASSERT(
        if (numStampUnits < 1)
            CG_ASSERT("At least one Color Write unit must be attached.");
        if (suPrefixes == NULL)
            CG_ASSERT("Color Write prefix array required.");
        for(i = 0; i < numStampUnits; i++)
            if (suPrefixes[i] == NULL)
                CG_ASSERT("Color Write prefix missing.");
    )


    //  Allocate the signals with the color write units.
    blockStateCW = new Signal*[numStampUnits];

    //  Check allocation.
    CG_ASSERT_COND(!(blockStateCW == NULL), "Error allocating array of block state signals from Color Write.");
    //  Create block state signal with all the Color Write units.
    for(i = 0; i < numStampUnits; i++)
    {
        //  Create color buffer block state signal.
        blockStateCW[i] = newInputSignal("ColorBlockState", 1, blockStateLatency, suPrefixes[i]);
    }

    //  Allocate the signals with the z stencil test units.
    blockStateZST = new Signal*[numStampUnits];

    //  Check allocation.  
    CG_ASSERT_COND(!(blockStateZST == NULL), "Error allocating array of block state signals from Z Stencil Test.");
    //  Create block state signal with all Z Stencil Test units.
    for(i = 0; i < numStampUnits; i++)
    {
        //  Create block state signal with Z Stencil Test unit.
        blockStateZST[i] = newInputSignal("ZStencilBlockState", 1, blockStateLatency, suPrefixes[i]);
    }

    //  Calculate the shift bits for block addresses.  
    blockShift = GPUMath::calculateShift(blockSize);

    //  Create the block request queue.  
    blockQueue = new BlockRequest[blockQueueSize];

    //  Check allocation.  
    CG_ASSERT_COND(!(blockQueue == NULL), "Could not allocate the block request queue.\n");
    //  Allocate the block buffer.  
    blockBuffer = new U08*[blockQueueSize];

    //  Check allocation.  
    CG_ASSERT_COND(!(blockBuffer == NULL), "Could not allocate the block buffer.\n");
    //  Allocate each block buffer.  
    for (i = 0; i < blockQueueSize; i++)
    {
        //  Allocate buffer.  
        blockBuffer[i] = new U08[ColorCacheV2::UNCOMPRESSED_BLOCK_SIZE];

        //  Check allocation.  
        CG_ASSERT_COND(!(blockBuffer[i] == NULL), "Could not allocate block buffer.\n");    }

    //  Create buffers.  

    //  Set color buffer to NULL.  
    colorBuffer = NULL;

    //  Set z stencil buffer to NULL.
    zstBuffer = NULL;
    
    //  Reset frame counter.  
    frameCounter = startFrame;
    
    //  Reset batch counter.
    batchCounter = 0;

    //  Reset state.  
    state = RAST_RESET;

    //  Reset free ticket counter.  
    freeTickets = MAX_MEMORY_TICKETS;

    //  Reset memory bus cycle counter.  
    busCycles = 0;

    //  Reset block state update cycle counter.  
    stateUpdateCycles = 0;

    //  Reset size of the color state buffer.  
    colorStateBufferBlocks = 0;
    
    //  Reset size of the z stencil state buffer.
    zStencilStateBufferBlocks = 0;

    //  Set a fake last command.  
    lastRSCommand = new RasterizerCommand(RSCOM_RESET);

    //  Compute the size in bytes of a texture block.
    U32 texBlockSize = 4 * (1 << (mortonBlockDim * 2));
    
    //  Set number of ROPs units in the Pixel Mapper.
    colorPixelMapper.setupUnit(numStampUnits, 0);
    zstPixelMapper.setupUnit(numStampUnits, 0);
    
    //  Set the ticket list to the maximum number of memory tickets.
    ticketList.resize(MAX_MEMORY_TICKETS);
    
    string queueName;
    queueName.clear();
    queueName = "FreeTicketQueueDAC";
    ticketList.setName(queueName);    

    //  Create blitter unit.      
    blt = new Blitter(bltTransImpl,
                STAMP_HEIGHT, STAMP_WIDTH, genHeight / STAMP_HEIGHT, genWidth / STAMP_WIDTH, (U32) ceil(scanHeight / (F32) genHeight),
                (U32) ceil(scanWidth / (F32) genWidth), overHeight, overWidth,
                mortonBlockDim, mortonSBlockDim, texBlockSize,
                blockLatency, blocksPerCycle, blockSz, blockQSize, decompLatency, saveBlitSourceData
            );
}

//  cmoDisplayController simulation function.  
void cmoDisplayController::clock(U64 cycle)
{
    ColorBlockStateInfo *colorBlockStateInfo;
    ROPBlockState *colorBufferStateAux;
    RasterizerCommand *rastComm;
    MemoryTransaction *memTrans;
    U32 i;
    U32 j;
    U32 numBlocks;
    RasterizerState lastState;

    /**  NOTE: SCREEN REFRESH (MEMORY CONSUMPTION) IS NOT IMPLEMENTED
         YET, ONLY FRAMEBUFFER READ AT SWAP (FAKE REFRESH).  A REFRESH
         RATE SHOULD BE ADDED TO THE GPU REGISTERS.  **/


    //  Keep cmoDisplayController state.  
    lastState = state;

    GPU_DEBUG_BOX(
        printf("cmoDisplayController => clock %lld.\n", cycle);
    )

    //  Receive transactions from the memory controller.  
    while(memoryData->read(cycle, (DynamicObject *&) memTrans))
    {
        //  Process memory transaction.  
        processMemoryTransaction(memTrans);
    }

    //  Update color buffer block state update cycles.  
    if (stateUpdateCycles > 0)
    {
        GPU_DEBUG_BOX(
            printf("cmoDisplayController => Remaining update cycles %d.\n",
                stateUpdateCycles);
        )

        //  Update cycles remaining.  
        stateUpdateCycles--;
    }

    //  Receive block state updates.  
    for(i = 0; i < numStampUnits; i++)
    {
        if (blockStateCW[i]->read(cycle, (DynamicObject *&) colorBlockStateInfo))
        {
            // NOTE:  REMOVED AS A FAST FIX. (???)
            
            //  Check that the color buffer state memory is not being updated.
            //GPU_ASSERT(
            //    if (stateUpdateCycles > 0)
            //        CG_ASSERT("Still updating the previous color buffer state memory.");
            //)


            GPU_DEBUG_BOX(
                printf("cmoDisplayController (%lld) => Received color buffer state memory from Color Write unit %d.  Blocks %d.\n",
                    cycle, i, colorBlockStateInfo->getNumBlocks());
            )

            //  Get number of blocks in the update object.  
            numBlocks = colorBlockStateInfo->getNumBlocks() * numStampUnits;

            //  Check if color buffer state was created.  
            if ((colorBufferState == NULL) || (colorStateBufferBlocks != numBlocks))
            {
                //  Deleting a NULL pointer shouldn't be a problem.  
                delete[] colorBufferState;

                //  Allocate the new buffer for the color state.  
                colorBufferState = new ROPBlockState[numBlocks];

                //  Set size of the color state buffer.  
                colorStateBufferBlocks = numBlocks;
            }

            //  Get the new color buffer state memory table.  
            colorBufferStateAux = colorBlockStateInfo->getColorBufferState();

            U32 unitStride = scanH * scanW;

            //  Update the color buffer state table.  Each stamp unit modifies its assigned scan tiles.  
            for(j = 0; j < numBlocks; j++)
            {
                U32 k = (j / (unitStride * numStampUnits)) * unitStride + GPU_MOD(j, unitStride);
                
                //  Check stamp unit.  
                if (blockUnit(colorPixelMapper, j) == i)
                    colorBufferState[j] = colorCompression ? colorBufferStateAux[k] : ROPBlockState(ROPBlockState::UNCOMPRESSED);
            }

            //  Set the block state update cycles.  
            stateUpdateCycles += (U32) ceil((F32) numBlocks / ((F32) blocksCycle * numStampUnits));

            //  Delete  color buffer state memory table.  
            delete[] colorBufferStateAux;

            //  Delete state info object.  
            delete colorBlockStateInfo;
        }

        if (blockStateZST[i]->read(cycle, (DynamicObject *&) colorBlockStateInfo))
        {
            // NOTE:  REMOVED AS A FAST FIX.  
            //  Check that the color buffer state memory is not being updated.  
            //GPU_ASSERT(
            //    if (stateUpdateCycles > 0)
            //        CG_ASSERT("Still updating the previous color buffer state memory.");
            //)


            GPU_DEBUG_BOX(
                printf("cmoDisplayController (%lld) => Received z stencil buffer state memory from Z Stencil Test unit %d.  Blocks %d.\n",
                    cycle, i, colorBlockStateInfo->getNumBlocks());
            )

            //  Get number of blocks in the update object.
            numBlocks = colorBlockStateInfo->getNumBlocks() * numStampUnits;

            //  Check if z stencil buffer state was created.
            if ((zStencilBufferState == NULL) || (zStencilStateBufferBlocks != numBlocks))
            {
                //  Deleting a NULL pointer shouldn't be a problem.
                delete[] zStencilBufferState;

                //  Allocate the new buffer for the z stencil state.
                zStencilBufferState = new ROPBlockState[numBlocks];

                //  Set size of the z stencil state buffer.
                zStencilStateBufferBlocks = numBlocks;
            }

            //  Get the new color buffer state memory table.  */
            ROPBlockState *zStencilBufferStateAux = colorBlockStateInfo->getColorBufferState();

            U32 unitStride = scanH * scanW;

            //  Update the z stencil buffer state table.  Each stamp unit modifies its assigned scan tiles.
            for(j = 0; j < numBlocks; j++)
            {
                U32 k = (j / (unitStride * numStampUnits)) * unitStride + GPU_MOD(j, unitStride);
                
                //  Check stamp unit.
                if (blockUnit(zstPixelMapper, j) == i)
                    zStencilBufferState[j] = zStencilCompression ? zStencilBufferStateAux[k] : ROPBlockState(ROPBlockState::UNCOMPRESSED);
            }

            //  Set the block state update cycles.
            stateUpdateCycles += (U32) ceil((F32) numBlocks / ((F32) blocksCycle * numStampUnits));

            //  Delete z stencil buffer state memory table.
            delete[] zStencilBufferStateAux;

            //  Delete state info object.
            delete colorBlockStateInfo;
        }
    }

    //  Simulate current cycle.
    switch(state)
    {
        case RAST_RESET:

            //  Reset state.

            GPU_DEBUG_BOX(
                printf("cmoDisplayController => RESET state.\n");
            )

            //  Reset the state of the cmoDisplayController unit.
            reset();

            //  Change to ready state.
            state = RAST_READY;

            break;

        case RAST_READY:
            //  Ready state.  

            GPU_DEBUG_BOX(
                printf("cmoDisplayController => READY state.\n");
            )

            //  Receive and process command from the Command Processor.  
            if (dacCommand->read(cycle, (DynamicObject *&) rastComm))
                processCommand(cycle, rastComm);

            //  Check if frame refresh/dumping must start.  
            if (!synchedRefresh && refreshFrame && (GPU_MOD(cycle, refreshRate) == (refreshRate - 1)))
            {
                //  Initialize frame refresh/dumping state.  
                startRefresh();
            }

            break;

        case RAST_END:
            //  End state.  

            GPU_DEBUG_BOX(
                if (state == RAST_END)
                    printf("cmoDisplayController => END state.\n");
            )

            //  Wait for END command from the Command Processor.  
            if (dacCommand->read(cycle, (DynamicObject *&) rastComm))
                processCommand(cycle, rastComm);

            break;

        case RAST_SWAP:

            //  Swap state.

            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Swap state.\n");
            )

            //  Receive and process command from the Command Processor.
            if (dacCommand->read(cycle, (DynamicObject *&) rastComm))
                processCommand(cycle, rastComm);

            //  Update memory transmission.
            updateMemory(cycle);
            
            //  Update block decompressor stage.
            updateDecompressor(cycle, &bmoColorCompressor::getInstance(), colorBuffer, clearColorData, bytesPixel, frontBuffer);
           
            //  Update block request stage.
            updateBlockRequest(cycle);
            
            //  Update request queue stage.
            updateRequestQueue(cycle, &bmoColorCompressor::getInstance(), frontBuffer, colorBufferSize, colorBufferState, blockSize);
                                                                
            //  Check if the full color buffer has been read.
            if ((busCycles == 0) && (numFree == blockQueueSize) && (stateUpdateCycles == 0))
            {
                //  Write the ppm file with the current framebuffer.  
                writeColorBuffer();

                //GPU_DEBUG_BOX(
                    printf("cmoDisplayController => Cycle %lld Color Buffer Dumped.\n", cycle);
                //)

                //  Delete the color buffer.
                delete[] colorBuffer;

                //  Update frame counter.
                frameCounter++;
                
                //  Reset batch counter.
                batchCounter = 0;

                //  Check if synched refresh.
                if (synchedRefresh)
                {
                    //  Change to end state.
                    state = RAST_END;
                }
                else
                {
                    //  Change to end state.
                    state = RAST_READY;
                }
            }                        

            break;

        case RAST_DUMP_BUFFER:

            //  Dump buffer state.

            GPU_DEBUG_BOX(
                printf("cmoDisplayController => DUMP BUFFER state.\n");
            )

            //  Receive and process command from the Command Processor.
            if (dacCommand->read(cycle, (DynamicObject *&) rastComm))
                processCommand(cycle, rastComm);

            //  Update memory transmission.
            updateMemory(cycle);
            
            switch(lastRSCommand->getCommand())
            {
                case RSCOM_DUMP_COLOR:
                
                    //  Update block decompressor stage.
                    updateDecompressor(cycle, &bmoColorCompressor::getInstance(), colorBuffer, clearColorData, bytesPixel, backBuffer);
                    
                    break;

                case RSCOM_DUMP_DEPTH:
                case RSCOM_DUMP_STENCIL:
                
                    //  Update block decompressor stage.
                    updateDecompressor(cycle, &bmoDepthCompressor::getInstance(), zstBuffer, (U08 *) &clearZStencilData,
                                       zStencilBytesPixel, zStencilBuffer);
                    
                    break;
                    
                default:
                
                    CG_ASSERT("Expected a dump buffer command.");
                    break;
            }
            
                       
            //  Update block request stage.
            updateBlockRequest(cycle);
            
            switch(lastRSCommand->getCommand())
            {
                case RSCOM_DUMP_COLOR:
                
                    //  Update request queue stage.
                    updateRequestQueue(cycle, &bmoColorCompressor::getInstance(), backBuffer, colorBufferSize, colorBufferState, blockSize);
                    
                    break;
                    
                case RSCOM_DUMP_DEPTH:
                case RSCOM_DUMP_STENCIL:
                
                    //  Update request queue stage.
                    updateRequestQueue(cycle, &bmoDepthCompressor::getInstance(), zStencilBuffer, zStencilBufferSize, zStencilBufferState, zStencilBlockSize);
                    
                    break;
                    
                default:
                
                    CG_ASSERT("Expected a dump buffer command.");
                    break;
            }
                    
                
                                                                
            //  Check if the full color buffer has been read.
            if ((busCycles == 0) && (numFree == blockQueueSize) && (stateUpdateCycles == 0))
            {
                switch(lastRSCommand->getCommand())
                {
                    case RSCOM_DUMP_COLOR:
                    
                        //  Dump the color buffer as a PPM file.
                        writeColorBuffer();
                        
                        //GPU_DEBUG_BOX(
                            printf("cmoDisplayController => Cycle %lld Color Buffer Dumped.\n", cycle);
                        //)
                        
                        //  Delete the color buffer.
                        delete[] colorBuffer;
                        
                        break;
                  
                    case RSCOM_DUMP_DEPTH:
                    
                        //  Dump the z buffer as a PPM file.
                        writeDepthBuffer();
                        
                        //GPU_DEBUG_BOX(
                            printf("cmoDisplayController => Cycle %lld Depth Buffer Dumped.\n", cycle);
                        //)

                        //  Delete the z stencil buffer.
                        delete[] zstBuffer;
                        
                        break;
                        
                    case RSCOM_DUMP_STENCIL:
                    
                        //  Dump the stencil buffer as a PPM file.
                        writeStencilBuffer();
                        
                        //GPU_DEBUG_BOX(
                            printf("cmoDisplayController => Cycle %lld Stencil Buffer Dumped.\n", cycle);
                        //)

                        //  Delete the z stencil buffer.
                        delete[] zstBuffer;
                        
                        break;
                        
                    default:                        
                        CG_ASSERT("Expected a dump buffer command.");
                        break;
                }

                //  Change to end state.
                state = RAST_END;
            }                        

            break;

        case RAST_BLIT:

            //  Blit state.  

            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Blit state.\n");
            )

            //  Receive and process command from the Command Processor.  
            if (dacCommand->read(cycle, (DynamicObject *&) rastComm))
                processCommand(cycle, rastComm);

            //  Check if memory bus is busy.  
            if (busCycles > 0)
            {
                //  Update bus use counter.  
                busCycles--;

                GPU_DEBUG_BOX(
                    printf("cmoDisplayController %lld => Transmission in progress.  Remaining cycles %d\n",
                        cycle, busCycles);
                )

                //  Check end of transmission.  
                if (busCycles == 0)
                {
                    GPU_DEBUG_BOX(
                        printf("cmoDisplayController => End of memory read.\n");
                    )

                    blt->receivedReadTransaction(lastTicket, lastSize);
                    
                    //  Update block queue entry received bytes.  
                    //blockQueue[ticket2queue[GPU_MOD(lastTicket, MAX_MEMORY_TICKETS)]].received += lastSize;

                    //  Update free memory tickets counter.  
                    //freeTickets++;
                }
            }

            if (stateUpdateCycles == 0 && !bufferStateUpdatedAtBlitter)
            {
                //printf("cmoDisplayController (%lld) => Sending color buffer state to Blitter | num block = %d\n", cycle, address2block(colorBufferSize));
                blt->setColorBufferState(colorBufferState, address2block(colorBufferSize));
                bufferStateUpdatedAtBlitter = true;
            }
            
            blt->clock(cycle);

            if (blt->currentFinished())
            {
                GPU_DEBUG_BOX(
                    printf("cmoDisplayController::Blitter %lld => End of bit blit operation.\n", cycle);
                )

                blt->dumpBlitSource(frameCounter, blitCounter);
                
                blitCounter++;
                
                //  Change to end state.  
                state = RAST_END;
            }
                
            break;
            
        default:
            CG_ASSERT("Undefined cmoDisplayController state.");
            break;
    }

    //  Send current state to the Command Processor.  
    dacState->write(cycle, new RasterizerStateInfo(state));

    //  Do not go to END state in unsynched mode, just send the state to Command Processor.  
    if (!synchedRefresh && (state == RAST_END))
    {
        state = lastState;
    }
}

//  Processes a rasterizer command.  
void cmoDisplayController::processCommand(U64 cycle, RasterizerCommand *command)
{
    U32 aux;
    U32 samples;

    //  Delete last command.  
    delete lastRSCommand;

    //  Store current command as last received command.  
    lastRSCommand = command;

    //  Process command.  
    switch(command->getCommand())
    {
        case RSCOM_RESET:
            //  Reset command from the  main mdu.  

            GPU_DEBUG_BOX(
                printf("cmoDisplayController => RESET command received.\n");
            )

            //  Change to reset state.  
            state = RAST_RESET;

            break;

        case RSCOM_END:
            //  End command received from  main mdu.  

            GPU_DEBUG_BOX(
                printf("cmoDisplayController => END command received.\n");
            )

            //  Check current state.  
            CG_ASSERT_COND(!((synchedRefresh && (state != RAST_END)) || (!synchedRefresh && (state != RAST_SWAP) && (state != RAST_READY))), "END command can only be received in END state.");
            //  Check if synchronized refresh is enabled.  
            if (synchedRefresh)
            {
                //  Change to ready state.  
                state = RAST_READY;
            }

            break;

        case RSCOM_REG_WRITE:
            //  Write register command from the  main mdu.  

            GPU_DEBUG_BOX(
                printf("cmoDisplayController => REG_WRITE command received.\n");
            )

             //  Check current state.  
            CG_ASSERT_COND(!((state != RAST_READY) && (state != RAST_SWAP)), "REGISTER WRITE command can only be received in READY state.");
            //  Process register write.  
            processRegisterWrite(command->getRegister(),
                command->getSubRegister(), command->getRegisterData());

            break;

        case RSCOM_SWAP:
            //  Swap front and back buffer command.  

            GPU_DEBUG_BOX(
                printf("cmoDisplayController => SWAP command received.\n");
            )

            //  Check if syncrhonized refresh.  
            if (synchedRefresh && refreshFrame)
            {
                //  Initialize frame refresh/dumping state.  
                startRefresh();
            }
            else if (!refreshFrame)
            {
                //  Update frame counter.  
                frameCounter++;
                
                //  Reset batch counter.
                batchCounter = 0;

                //  Change to end state.  
                state = RAST_END;
            }
            else
            {
                //  Change to end state.  
                state = RAST_END;
            }

            //  Reset blit counter.
            blitCounter = 0;

            //  Swap front and back buffer address.  
            aux = frontBuffer;
            frontBuffer = backBuffer;
            backBuffer = aux;
            blt->setBackBufferAddress(backBuffer);

            break;

        case RSCOM_DUMP_COLOR:
        
            //  Dump the color buffer.
            
            GPU_DEBUG_BOX(
                printf("cmoDisplayController => DUMP_COLOR command received.\n");
            )

            //  Reset number of free queue entries.
            numFree = blockQueueSize;

            //  Reset number of blocks to request.
            numToRequest = 0;

            //  Reset number of blocks to decompress.
            numToDecompress = 0;

            //  Reset free, to requeuest and to decompress queue pointers.
            nextFree = nextRequest = nextDecompress = 0;

            //  Reset decompress cycles.
            decompressCycles = 0;

            //  Reset color buffer requested bytes (blocks).
            requested = 0;
            
            //  Compute color buffer size.
            colorBufferSize = colorPixelMapper.computeFrameBufferSize();
          
            //GPU_DEBUG_BOX(
                printf("cmoDisplayController => Color Buffer Address %08x | Color Buffer Size %d (%d x %d)\n", backBuffer, colorBufferSize, hRes, vRes);
            //)

            //  Allocate space for the color buffer.
            colorBuffer = new U08[colorBufferSize];

            //  Change to dump buffer state.
            state = RAST_DUMP_BUFFER;
            
            break;
            
        case RSCOM_DUMP_DEPTH:
        
            //  Dump the depth buffer.
            
            GPU_DEBUG_BOX(
                printf("cmoDisplayController => DUMP_COLOR command received.\n");
            )

            //  Reset number of free queue entries.
            numFree = blockQueueSize;

            //  Reset number of blocks to request.
            numToRequest = 0;

            //  Reset number of blocks to decompress.
            numToDecompress = 0;

            //  Reset free, to requeuest and to decompress queue pointers.
            nextFree = nextRequest = nextDecompress = 0;

            //  Reset decompress cycles.
            decompressCycles = 0;

            //  Reset color buffer requested bytes (blocks).
            requested = 0;
            
            //  Compute z stencil buffer size.
            zStencilBufferSize = zstPixelMapper.computeFrameBufferSize();
          
            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Z Stencil Buffer Address %08x Size %d (%d x %d)\n", zStencilBuffer, zStencilBufferSize, hRes, vRes);
            )

            //  Allocate space for the z stencil buffer.
            zstBuffer = new U08[zStencilBufferSize];

            //  Set z stencil block size.
            zStencilBlockSize = blockSize;
            
            //  Set z stencil bytes per pixel.
            zStencilBytesPixel = 4;
            
            //  Set z stencil clear value.
            clearZStencilData = ((clearStencil & 0xff) << 24) | (clearDepth & 0x00ffffff);
            
            //  Change to dump buffer state.
            state = RAST_DUMP_BUFFER;

            break;
            
        case RSCOM_DUMP_STENCIL:
        
            //  Dump the stencil buffer.
            
            GPU_DEBUG_BOX(
                printf("cmoDisplayController => DUMP_STENCIL command received.\n");
            )

            //  Reset number of free queue entries.
            numFree = blockQueueSize;

            //  Reset number of blocks to request.
            numToRequest = 0;

            //  Reset number of blocks to decompress.
            numToDecompress = 0;

            //  Reset free, to requeuest and to decompress queue pointers.
            nextFree = nextRequest = nextDecompress = 0;

            //  Reset decompress cycles.
            decompressCycles = 0;

            //  Reset color buffer requested bytes (blocks).
            requested = 0;
            
            //  Compute z stencil buffer size.
            zStencilBufferSize = zstPixelMapper.computeFrameBufferSize();
          
            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Z Stencil Buffer Address %08x Size %d (%d x %d)\n", zStencilBuffer, zStencilBufferSize, hRes, vRes);
            )

            //  Allocate space for the z stencil buffer.
            zstBuffer = new U08[zStencilBufferSize];

            //  Set z stencil block size.
            zStencilBlockSize = blockSize;
            
            //  Set z stencil bytes per pixel.
            zStencilBytesPixel = 4;
            
            //  Set z stencil clear value.
            clearZStencilData = ((clearStencil & 0xff) << 24) | (clearDepth & 0x00ffffff);

            //  Change to dump buffer state.
            state = RAST_DUMP_BUFFER;

            break;
            
        case RSCOM_BLIT:
            
            //  Blit a framebuffer region to a texture destination address.

            GPU_DEBUG_BOX(
                printf("cmoDisplayController => BLIT command received at cycle %lld.\n", cycle);
            )

            //  Initialize blitter unit 
            
            blt->startBitBlit();
            
            /*colorBufferSize = ((U32) ceil(vRes / (F32) (overH * scanH * genH * STAMP_HEIGHT))) *
                ((U32) ceil(hRes / (F32) (overW * scanW * genW * STAMP_WIDTH))) *
                overW * overH * scanW * scanH * genW * genH * STAMP_WIDTH * STAMP_HEIGHT * bytesPixel;*/
            //samples = multisampling ? msaaSamples : 1;
            //pixelMapper.setupDisplay(hRes, vRes, STAMP_WIDTH, STAMP_HEIGHT, genW, genH, scanW, scanH, overW, overH, samples, bytesPixel);
            colorBufferSize = colorPixelMapper.computeFrameBufferSize();
        
            bufferStateUpdatedAtBlitter = false;
            
            //  Change state to BLIT.  
            state = RAST_BLIT;

            break;

        case RSCOM_DRAW:
        
            //  Draw command.  Used to count draw calls.
            
            //  Update batch counter.
            batchCounter++;
            
            break;
            
        case RSCOM_FRAME_CHANGE:

            //  Fake swap buffer command for draw command skipping mode.

            GPU_DEBUG_BOX(
                printf("cmoDisplayController => FRAME CHANGE command received.\n");
            )

            //  Update frame counter.
            frameCounter++;
            
            //  Reset batch counter.
            batchCounter = 0;

            //  Reset blit counter.
            blitCounter = 0;

            break;

        default:
            CG_ASSERT("Unsupported command.");
            break;
    }
}

//  Processes a memory transaction.  
void cmoDisplayController::processMemoryTransaction(MemoryTransaction *memTrans)
{
    //  Get transaction type.  
    switch(memTrans->getCommand())
    {
        case MT_STATE:

            //  Received state of the Memory controller.  
            memState = memTrans->getState();

            break;

        case MT_READ_DATA:

            //  Check if the bus is ready.  
            CG_ASSERT_COND(!(busCycles != 0), "Memory bus was still in use.");
            //  Get transaction size.  
            lastSize = memTrans->getSize();

            //  Get transaction ticket.  
            lastTicket = memTrans->getID();

            //  Get number of cycles.  
            busCycles = memTrans->getBusCycles();

            break;

        default:

            CG_ASSERT("Unsupported memory transaction.");

            break;
    }

    //  Delete processed memory transaction.  
    delete memTrans;
}

//  Process a register write.  
void cmoDisplayController::processRegisterWrite(GPURegister reg, U32 subreg, GPURegData data)
{
    U32 samples;
    
    //  Process register write.  
    switch(reg)
    {
        case GPU_DISPLAY_X_RES:
            //  Write display horizontal resolution register.  
            hRes = data.uintVal;

            blt->setDisplayXRes(hRes);
            
            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Write GPU_DISPLAY_X_RES = %d.\n", hRes);
            )

            //  Update the pixel mapper.
            samples = multisampling ? msaaSamples : 1;
            colorPixelMapper.setupDisplay(hRes, vRes, STAMP_WIDTH, STAMP_HEIGHT, genW, genH, scanW, scanH, overW, overH, samples, bytesPixel);
            zstPixelMapper.setupDisplay(hRes, vRes, STAMP_WIDTH, STAMP_HEIGHT, genW, genH, scanW, scanH, overW, overH, samples, 4);

            break;

        case GPU_DISPLAY_Y_RES:
            //  Write display vertical resolution register.  
            vRes = data.uintVal;

            blt->setDisplayYRes(vRes);
            
            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Write GPU_DISPLAY_Y_RES = %d.\n", vRes);
            )

            //  Update the pixel mapper.
            samples = multisampling ? msaaSamples : 1;
            colorPixelMapper.setupDisplay(hRes, vRes, STAMP_WIDTH, STAMP_HEIGHT, genW, genH, scanW, scanH, overW, overH, samples, bytesPixel);
            zstPixelMapper.setupDisplay(hRes, vRes, STAMP_WIDTH, STAMP_HEIGHT, genW, genH, scanW, scanH, overW, overH, samples, 4);

            break;

        case GPU_D3D9_PIXEL_COORDINATES:
        
            //  Write use D3D9 pixel coordinates convention register.
            d3d9PixelCoordinates = data.booleanVal;
            
            //blt->setUseD3D9PixelCoordinates(d3d9PixelCoordinates);
            
            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Write GPU_D3D9_PIXEL_COORDINATE = %s\n", d3d9PixelCoordinates ? "TRUE" : "FALSE");
            )
            
            break;
            
        case GPU_VIEWPORT_INI_X:
            //  Write viewport initial x coordinate register.  
            startX = data.intVal;

            blt->setViewportStartX(startX);
            
            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Write GPU_VIEWPORT_INI_X = %d.\n", startX);
            )

            break;

        case GPU_VIEWPORT_INI_Y:
            //  Write viewport initial y coordinate register.  
            startY = data.intVal;

            blt->setViewportStartY(startY);
            
            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Write GPU_VIEWPORT_INI_Y = %d.\n", startY);
            )

            break;

        case GPU_VIEWPORT_WIDTH:
            //  Write viewport width register.  
            width = data.uintVal;

            blt->setViewportWidth(width);
            
            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Write GPU_VIEWPORT_WIDTH = %d.\n", width);
            )

            break;

        case GPU_VIEWPORT_HEIGHT:
            //  Write viewport height register.  
            height = data.uintVal;

            blt->setViewportHeight(height);
            
            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Write GPU_VIEWPORT_HEIGHT = %d.\n", height);
            )

            break;

        case GPU_COLOR_BUFFER_FORMAT:

            //  Write color buffer format.
            colorBufferFormat = data.txFormat;
            
            //  Aliased to render target 0.
            rtFormat[0] = colorBufferFormat;
            
            //  Set the bytes per pixel for the current color buffer format.
            switch(colorBufferFormat)
            {
                case GPU_RGBA8888:
                case GPU_RG16F:
                case GPU_R32F:
                    bytesPixel = 4;
                    break;
                case GPU_RGBA16:
                case GPU_RGBA16F:
                    bytesPixel = 8;
                    break;
                default:
                    CG_ASSERT("Unsupported color buffer format.");
                    break;
            }
            
            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Write GPU_COLOR_BUFFER_FORMAT = ");
                switch(colorBufferFormat)
                {
                    case GPU_RGBA8888:
                        printf(" GPU_RGBA8888.\n");
                        break;
                        
                    case GPU_RG16F:
                        printf(" GPU_RG16F.\n");
                        break;

                    case GPU_R32F:
                        printf(" GPU_R32F.\n");
                        break;
                        
                    case GPU_RGBA16:
                        printf(" GPU_RGBA16.\n");
                        break;
                        
                    case GPU_RGBA16F:
                        printf(" GPU_RGBA16F.\n");
                        break;
                }
            )
            
            //  Set color buffer format in the blitter.
            blt->setColorBufferFormat(colorBufferFormat);
            
            //  Convert the float point clear color to the color buffer format.
            switch(colorBufferFormat)
            {
                case GPU_RGBA8888:
                
                    clearColorData[0] = U08(clearColor[0] * 255.0f);
                    clearColorData[1] = U08(clearColor[1] * 255.0f);
                    clearColorData[2] = U08(clearColor[2] * 255.0f);
                    clearColorData[3] = U08(clearColor[3] * 255.0f);
                    
                    break;

                case GPU_RG16F:
                
                    ((f16bit *) clearColorData)[0] = GPUMath::convertFP32ToFP16(clearColor[0]);
                    ((f16bit *) clearColorData)[1] = GPUMath::convertFP32ToFP16(clearColor[1]);
                    break;
                    
                case GPU_R32F:
                
                    ((F32 *) clearColorData)[0] = clearColor[0];
                    break;
                
                case GPU_RGBA16:
                
                    ((U16 *) clearColorData)[0] = U16(clearColor[0] * 65535.0f);
                    ((U16 *) clearColorData)[1] = U16(clearColor[1] * 65535.0f);
                    ((U16 *) clearColorData)[2] = U16(clearColor[2] * 65535.0f);
                    ((U16 *) clearColorData)[3] = U16(clearColor[3] * 65535.0f);
                    
                    break;

                case GPU_RGBA16F:
                
                    ((f16bit *) clearColorData)[0] = GPUMath::convertFP32ToFP16(clearColor[0]);
                    ((f16bit *) clearColorData)[1] = GPUMath::convertFP32ToFP16(clearColor[1]);
                    ((f16bit *) clearColorData)[2] = GPUMath::convertFP32ToFP16(clearColor[2]);
                    ((f16bit *) clearColorData)[3] = GPUMath::convertFP32ToFP16(clearColor[3]);
                    
                    break;
                    
                default:
                    CG_ASSERT("Unsupported color buffer format.");
                    break;
            }
            
            //  Update the pixel mapper.
            samples = multisampling ? msaaSamples : 1;
            colorPixelMapper.setupDisplay(hRes, vRes, STAMP_WIDTH, STAMP_HEIGHT, genW, genH, scanW, scanH, overW, overH, samples, bytesPixel);

            break;
            
        case GPU_COLOR_BUFFER_CLEAR:
        
            //  Write color buffer clear color value.  
            clearColor[0] = data.qfVal[0];
            clearColor[1] = data.qfVal[1];
            clearColor[2] = data.qfVal[2];
            clearColor[3] = data.qfVal[3];
            
            //  Convert the float point clear color to the color buffer format.
            switch(colorBufferFormat)
            {
                case GPU_RGBA8888:
                
                    clearColorData[0] = U08(clearColor[0] * 255.0f);
                    clearColorData[1] = U08(clearColor[1] * 255.0f);
                    clearColorData[2] = U08(clearColor[2] * 255.0f);
                    clearColorData[3] = U08(clearColor[3] * 255.0f);
                    
                    break;

                case GPU_RG16F:
                
                    ((f16bit *) clearColorData)[0] = GPUMath::convertFP32ToFP16(clearColor[0]);
                    ((f16bit *) clearColorData)[1] = GPUMath::convertFP32ToFP16(clearColor[1]);
                    break;
                    
                case GPU_R32F:
                
                    ((F32 *) clearColorData)[0] = clearColor[0];
                    break;
                
                case GPU_RGBA16:
                
                    ((U16 *) clearColorData)[0] = U16(clearColor[0] * 65535.0f);
                    ((U16 *) clearColorData)[1] = U16(clearColor[1] * 65535.0f);
                    ((U16 *) clearColorData)[2] = U16(clearColor[2] * 65535.0f);
                    ((U16 *) clearColorData)[3] = U16(clearColor[3] * 65535.0f);
                    
                    break;

                case GPU_RGBA16F:
                
                    ((f16bit *) clearColorData)[0] = GPUMath::convertFP32ToFP16(clearColor[0]);
                    ((f16bit *) clearColorData)[1] = GPUMath::convertFP32ToFP16(clearColor[1]);
                    ((f16bit *) clearColorData)[2] = GPUMath::convertFP32ToFP16(clearColor[2]);
                    ((f16bit *) clearColorData)[3] = GPUMath::convertFP32ToFP16(clearColor[3]);
                    
                    break;
                    
                default:
                    CG_ASSERT("Unsupported color buffer format.");
                    break;
            }

            blt->setClearColor(clearColor);

            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Write GPU_COLOR_BUFFER_CLEAR = (%f, %f, %f, %f).\n", clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
            )

            break;

        case GPU_MULTISAMPLING:
        
            //  Write Multisampling enable flag.
            multisampling = data.booleanVal;
            
            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Write GPU_MULTISAMPLING = %s\n", multisampling?"TRUE":"FALSE");
            )
            
            //  Update the pixel mapper.
            samples = multisampling ? msaaSamples : 1;
            colorPixelMapper.setupDisplay(hRes, vRes, STAMP_WIDTH, STAMP_HEIGHT, genW, genH, scanW, scanH, overW, overH, samples, bytesPixel);
            zstPixelMapper.setupDisplay(hRes, vRes, STAMP_WIDTH, STAMP_HEIGHT, genW, genH, scanW, scanH, overW, overH, samples, 4);

            blt->setMultisampling(multisampling);
                 
            break;
            
        case GPU_MSAA_SAMPLES:
        
            //  Write MSAA z samples per fragment register.
            msaaSamples = data.uintVal;
            

            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Write GPU_MSAA_SAMPLES = %d\n", msaaSamples);
            )

            //  Update the pixel mapper.
            samples = multisampling ? msaaSamples : 1;
            colorPixelMapper.setupDisplay(hRes, vRes, STAMP_WIDTH, STAMP_HEIGHT, genW, genH, scanW, scanH, overW, overH, samples, bytesPixel);
            zstPixelMapper.setupDisplay(hRes, vRes, STAMP_WIDTH, STAMP_HEIGHT, genW, genH, scanW, scanH, overW, overH, samples, 4);

            blt->setMSAASamples(msaaSamples);
            
            break;            

        case GPU_ZSTENCIL_COMPRESSION:
        
            //  Write the z/stencil buffer compression enabled register.
            zStencilCompression = data.booleanVal;
            
            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Write GPU_ZSTENCIL_COMPRESSION = %s\n", zStencilCompression ? "ENABLED" : "DISABLED");
            )
            
            break;
            
        case GPU_COLOR_COMPRESSION:
        
            //  Write the color buffer compression enabled register.
            colorCompression = data.booleanVal;
            
            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Write GPU_COLOR_COMPRESSION = %s\n", colorCompression ? "ENABLED" : "DISABLED");
            )
            
            break;
            
        case GPU_BLIT_INI_X:
            //  Write bit blit operation initial x coordinate register. 

            blt->setBlitIniX(data.uintVal);
            
            GPU_DEBUG_BOX(
                printf("cmoDisplayController::Blitter => Write GPU_BLIT_INI_X = %x.\n", data.uintVal);
            )

            break;
            
        case GPU_BLIT_INI_Y:
            //  Write blit initial y coordinate register. 

            blt->setBlitIniY(data.uintVal);
            
            GPU_DEBUG_BOX(
                printf("cmoDisplayController::Blitter => Write GPU_BLIT_INI_Y = %x.\n", data.uintVal);
            )

            break;
            
        case GPU_BLIT_X_OFFSET:
            //  Write bit blit operation offset x coordinate register. 

            blt->setBlitXOffset(data.uintVal);
            
            GPU_DEBUG_BOX(
                printf("cmoDisplayController::Blitter => Write GPU_BLIT_X_OFFSET = %x.\n", data.uintVal);
            )

            break;
            
        case GPU_BLIT_Y_OFFSET:
            //  Write bit blit operation offset y coordinate register. 

            blt->setBlitYOffset(data.uintVal);
            
            GPU_DEBUG_BOX(
                printf("cmoDisplayController::Blitter => Write GPU_BLIT_Y_OFFSET = %x.\n", data.uintVal);
            )

            break;
                    
        case GPU_BLIT_WIDTH:
            //  Write blit width register. 
            
            blt->setBlitWidth(data.uintVal);
            
            GPU_DEBUG_BOX(
                printf("cmoDisplayController::Blitter => Write GPU_BLIT_WIDTH = %x.\n", data.uintVal);
            )

            break;
            
        case GPU_BLIT_HEIGHT:
            //  Write blit height register. 
            
            blt->setBlitHeight(data.uintVal);

            GPU_DEBUG_BOX(
                printf("cmoDisplayController::Blitter => Write GPU_BLIT_HEIGHT = %x.\n", data.uintVal);
            )

            break;
            
        case GPU_BLIT_DST_ADDRESS:
            //  Write blit GPU memory destination address register. 
            
            blt->setDestAddress(data.uintVal);

            GPU_DEBUG_BOX(
                printf("cmoDisplayController:Blitter => Write GPU_BLIT_DST_ADDRESS = %x.\n", data.uintVal);
            )

            break;

        case GPU_BLIT_DST_TX_WIDTH2:
            //  Write blit ceiling log 2 of texture width register. 
            
            blt->setDestTextWidth2(data.uintVal);

            GPU_DEBUG_BOX(
                printf("cmoDisplayController::Blitter => Write GPU_BLIT_DST_TX_WIDTH2 = %x.\n", data.txFormat);
            )

            break;
            
        case GPU_BLIT_DST_TX_FORMAT:
            //  Write blit texture destination format register. 
            
            blt->setDestTextFormat(data.txFormat);

            GPU_DEBUG_BOX(
                printf("cmoDisplayController::Blitter => Write GPU_BLIT_DST_TX_FORMAT = ");
                
                switch(data.txFormat)
                {
                    case GPU_ALPHA8:
                        printf("GPU_ALPHA8\n");
                        break;
                    case GPU_ALPHA12:
                        printf("GPU_ALPHA12\n");
                        break;
                    
                    case GPU_ALPHA16:
                        printf("GPU_ALPHA16\n");
                        break;
                    
                    case GPU_DEPTH_COMPONENT16:
                        printf("GPU_DEPTH_COMPONENT16\n");
                        break;
                    
                    case GPU_DEPTH_COMPONENT24:
                        printf("GPU_DEPTH_COMPONENT24\n");
                        break;

                    case GPU_DEPTH_COMPONENT32:
                        printf("GPU_DEPTH_COMPONENT32\n");
                        break;
                    
                    case GPU_LUMINANCE8:
                        printf("GPU_LUMINANCE8\n");
                        break;

                    case GPU_LUMINANCE8_SIGNED:
                        printf("GPU_LUMINANCE8_SIGNED\n");
                        break;

                    case GPU_LUMINANCE12:
                        printf("GPU_LUMINANCE12\n");
                        break;

                    case GPU_LUMINANCE16:
                        printf("GPU_LUMINANCE16\n");
                        break;
                    
                    case GPU_LUMINANCE4_ALPHA4:
                        printf("GPU_LUMINANCE4_ALPHA4\n");
                        break;
                        
                    case GPU_LUMINANCE6_ALPHA2:
                        printf("GPU_LUMINANCE6_ALPHA2\n");
                        break;
                        
                    case GPU_LUMINANCE8_ALPHA8:
                        printf("GPU_LUMINANCE8_ALPHA8\n");
                        break;

                    case GPU_LUMINANCE8_ALPHA8_SIGNED:
                        printf("GPU_LUMINANCE8_ALPHA8_SIGNED\n");
                        break;
                    
                    case GPU_LUMINANCE12_ALPHA4:
                        printf("GPU_LUMINANCE12_ALPHA4\n");
                        break;
                    
                    case GPU_LUMINANCE12_ALPHA12:
                        printf("GPU_LUMINANCE12_ALPHA12\n");
                        break;

                    case GPU_LUMINANCE16_ALPHA16:
                        printf("GPU_LUMINANCE16_ALPHA16\n");
                        break;

                    case GPU_INTENSITY8:
                        printf("GPU_INTENSITY8\n");
                        break;
                    
                    case GPU_INTENSITY12:
                        printf("GPU_INTENSITY12\n");
                        break;
                    
                    case GPU_INTENSITY16:
                        printf("GPU_INTENSITY16\n");
                        break;
                    case GPU_RGB332:
                        printf("GPU_RGB332\n");
                        break;

                    case GPU_RGB444:
                        printf("GPU_RGB444\n");
                        break;
                    
                    case GPU_RGB555:
                        printf("GPU_RGB555\n");
                        break;

                    case GPU_RGB565:
                        printf("GPU_RGB565\n");
                        break;

                    case GPU_RGB888:
                        printf("GPU_RGB888\n");
                        break;
                    
                    case GPU_RGB101010:
                        printf("GPU_RGB101010\n");
                        break;
                    case GPU_RGB121212:
                        printf("GPU_RGB121212\n");
                        break;
                    
                    case GPU_RGBA2222:
                        printf("GPU_RGBA2222\n");
                        break;
                    
                    case GPU_RGBA4444:
                        printf("GPU_RGBA4444\n");
                        break;
                        
                    case GPU_RGBA5551:
                        printf("GPU_RGBA5551\n");
                        break;
                    
                    case GPU_RGBA8888:
                        printf("GPU_RGBA8888\n");
                        break;
                    
                    case GPU_RGBA1010102:
                        printf("GPU_RGB1010102\n");
                        break;
                    
                    default:
                        CG_ASSERT("Unsupported texture format.");
                        break;
                }
            )

            break;

        case GPU_BLIT_DST_TX_BLOCK:
            //  Write blit texture destination format register. 
            
            blt->setDestTextBlock(data.txBlocking);

            GPU_DEBUG_BOX(
                printf("cmoDisplayController::Blitter => Write GPU_BLIT_DST_TX_BLOCK = ");
                
                switch(data.txBlocking)
                {
                    case GPU_TXBLOCK_TEXTURE:
                        printf("GPU_TXBLOCK_TEXTURE.\n");
                        break;

                    case GPU_TXBLOCK_FRAMEBUFFER:
                        printf("GPU_TXBLOCK_FRAMEBUFFER.\n");
                        break;

                    default:
                        CG_ASSERT("Unsupported texture blocking mode.");
                        break;
                }
            )

            break;

        case GPU_FRONTBUFFER_ADDR:
            //  Write front color buffer address register.  
            frontBuffer = data.uintVal;

            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Write GPU_FRONTBUFFER_ADDR = %08x.\n",
                    frontBuffer);
            )
            
            break;

        case GPU_BACKBUFFER_ADDR:
            //  Write back color buffer address register.  
            backBuffer = data.uintVal;

            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Write GPU_BACKBUFFER_ADDR = %08x.\n",
                    backBuffer);
            )
            
            blt->setBackBufferAddress(backBuffer);
            
            //  Aliased to render target 0.
            rtAddress[0] = backBuffer;
            
            break;

        case GPU_Z_BUFFER_CLEAR:
        
            //  Write depth clear value.
            clearDepth = data.uintVal;

            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Write GPU_Z_BUFFER_CLEAR = %x.\n", clearDepth);
            )

            break;

        case GPU_STENCIL_BUFFER_CLEAR:

            //  Write stencil clear value.
            clearStencil = (U08) (data.uintVal & 0xff);

            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Write GPU_STENCIL_BUFFER_CLEAR = %x.\n", clearStencil);
            )

            break;

        case GPU_Z_BUFFER_BIT_PRECISSION:

            //  Write z buffer bit precission register.
            depthPrecission = data.uintVal;

            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Write GPU_Z_BUFFER_BIT_PRECISSION = %x.\n", depthPrecission);
            )

            break;

        case GPU_ZSTENCILBUFFER_ADDR:
        
            //  Write z stencil buffer address register.
            zStencilBuffer = data.uintVal;

            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Write GPU_ZSTENCILBUFFER_ADDR = %08x.\n", zStencilBuffer);
            )

            break;

        case GPU_RENDER_TARGET_ENABLE:
        
            //  Check render target index.
            CG_ASSERT_COND(!(subreg >= MAX_RENDER_TARGETS), "Render target index out of range.");
            //  Write render target enable register.
            rtEnable[subreg] = data.booleanVal;
            
            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Write GPU_RENDER_TARGET_ENABLE[%d] = %s\n", subreg, rtEnable[subreg] ? "TRUE" : "FALSE");
            )            
            
            break;

        case GPU_RENDER_TARGET_FORMAT:
        
            //  Check render target index.
            CG_ASSERT_COND(!(subreg >= MAX_RENDER_TARGETS), "Render target index out of range.");
            //  Write render target format register.
            rtFormat[subreg] = data.txFormat;
            
            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Write GPU_RENDER_TARGET_FORMAT[%d] = ", subreg);
                switch(rtFormat[subreg])
                {
                    case GPU_RGBA8888:
                        printf(" GPU_RGBA8888.\n");
                        break;
                        
                    case GPU_RG16F:
                        printf(" GPU_RG16F.\n");
                        break;

                    case GPU_R32F:
                        printf(" GPU_R32F.\n");
                        break;
                        
                    case GPU_RGBA16:
                        printf(" GPU_RGBA16.\n");
                        break;
                        
                    case GPU_RGBA16F:
                        printf(" GPU_RGBA16F.\n");
                        break;
                    default:
                        CG_ASSERT("Unsupported render target format.");
                        break;
                }
            )            
            
            //  Render target 0 is aliased to the backbuffer.
            if (subreg == 0)
            {
                colorBufferFormat = rtFormat[0];
                
                //  Set the bytes per pixel for the current color buffer format.
                switch(colorBufferFormat)
                {
                    case GPU_RGBA8888:
                    case GPU_RG16F:
                    case GPU_R32F:
                        bytesPixel = 4;
                        break;
                    case GPU_RGBA16:
                    case GPU_RGBA16F:
                        bytesPixel = 8;
                        break;
                    default:
                        CG_ASSERT("Unsupported color buffer format.");
                        break;
                }
                
                //  Set color buffer format in the blitter.
                blt->setColorBufferFormat(colorBufferFormat);
                
                //  Convert the float point clear color to the color buffer format.
                switch(colorBufferFormat)
                {
                    case GPU_RGBA8888:
                    
                        clearColorData[0] = U08(clearColor[0] * 255.0f);
                        clearColorData[1] = U08(clearColor[1] * 255.0f);
                        clearColorData[2] = U08(clearColor[2] * 255.0f);
                        clearColorData[3] = U08(clearColor[3] * 255.0f);
                        
                        break;

                    case GPU_RG16F:
                    
                        ((f16bit *) clearColorData)[0] = GPUMath::convertFP32ToFP16(clearColor[0]);
                        ((f16bit *) clearColorData)[1] = GPUMath::convertFP32ToFP16(clearColor[1]);
                        break;
                        
                    case GPU_R32F:
                    
                        ((F32 *) clearColorData)[0] = clearColor[0];
                        break;
                    
                    case GPU_RGBA16:
                    
                        ((U16 *) clearColorData)[0] = U16(clearColor[0] * 65535.0f);
                        ((U16 *) clearColorData)[1] = U16(clearColor[1] * 65535.0f);
                        ((U16 *) clearColorData)[2] = U16(clearColor[2] * 65535.0f);
                        ((U16 *) clearColorData)[3] = U16(clearColor[3] * 65535.0f);
                        
                        break;

                    case GPU_RGBA16F:
                    
                        ((f16bit *) clearColorData)[0] = GPUMath::convertFP32ToFP16(clearColor[0]);
                        ((f16bit *) clearColorData)[1] = GPUMath::convertFP32ToFP16(clearColor[1]);
                        ((f16bit *) clearColorData)[2] = GPUMath::convertFP32ToFP16(clearColor[2]);
                        ((f16bit *) clearColorData)[3] = GPUMath::convertFP32ToFP16(clearColor[3]);
                        
                        break;
                        
                    default:
                        CG_ASSERT("Unsupported color buffer format.");
                        break;
                }
                
                //  Update the pixel mapper.
                samples = multisampling ? msaaSamples : 1;
                colorPixelMapper.setupDisplay(hRes, vRes, STAMP_WIDTH, STAMP_HEIGHT, genW, genH, scanW, scanH, overW, overH, samples, bytesPixel);
            }
            
            break;

        case GPU_RENDER_TARGET_ADDRESS:
        
            //  Check render target index.
            CG_ASSERT_COND(!(subreg >= MAX_RENDER_TARGETS), "Render target index out of range.");
            //  Write render target address register.
            rtAddress[subreg] = data.uintVal;
            
            GPU_DEBUG_BOX(
                printf("cmoDisplayController => Write GPU_RENDER_TARGET_ADDRESS[%d] = %08x\n", subreg, rtAddress[subreg]);
            )
            
            //  Render target 0 is aliased to the back buffer.
            if (subreg == 0)
            {
                backBuffer = rtAddress[0];
                blt->setBackBufferAddress(backBuffer);
            }
            
            break;
                    
        default:
            CG_ASSERT("Unsupported register.");
            break;
    }

}

//  Initializes the refresh/dumping of the color buffer.
void cmoDisplayController::startRefresh()
{
    //  Reset number of free queue entries.
    numFree = blockQueueSize;

    //  Reset number of blocks to request.
    numToRequest = 0;

    //  Reset number of blocks to decompress.
    numToDecompress = 0;

    //  Reset free, to requeuest and to decompress queue pointers.
    nextFree = nextRequest = nextDecompress = 0;

    //  Reset decompress cycles.  */
    decompressCycles = 0;

    //  Reset color buffer requested bytes (blocks).  
    requested = 0;

    //  Setup the display in the pixel mapper.
    //U32 samples = multisampling ? msaaSamples : 1;
    //pixelMapper.setupDisplay(hRes, vRes, STAMP_WIDTH, STAMP_HEIGHT, genW, genH, scanW, scanH, overW, overH, samples, bytesPixel);
    
    //  Check if multisampling is enabled.
    /*if (!multisampling)
    {
        //  Set color buffer size.
        //  Takes into account that the color buffer is stored in tiles
        //  of stamps:
        //
        //    size = horizontal tiles x vertical tiles x tile size in bytes
        //
        colorBufferSize2 = ((U32) ceil(vRes / (F32) (overH * scanH * genH * STAMP_HEIGHT))) *
            ((U32) ceil(hRes / (F32) (overW * scanW * genW * STAMP_WIDTH))) *
            overW * overH * scanW * scanH * genW * genH * STAMP_WIDTH * STAMP_HEIGHT * bytesPixel;
    }
    else
    {
        //  Compute color buffer size for all the samples.
        colorBufferSize2 = ((U32) ceil(vRes / (F32) (overH * scanH * genH * STAMP_HEIGHT))) *
            ((U32) ceil(hRes / (F32) (overW * scanW * genW * STAMP_WIDTH))) *
            overW * overH * scanW * scanH * genW * genH * STAMP_WIDTH * STAMP_HEIGHT * bytesPixel * msaaSamples;
    }*/
    
    //  Compute color buffer size.
    colorBufferSize = colorPixelMapper.computeFrameBufferSize();
  
    GPU_DEBUG_BOX(
        printf("cmoDisplayController => Color Buffer Size %d (%d x %d)\n",
            colorBufferSize, hRes, vRes);
    )

    //  NOTE THERE ARE MORE EFFICIENT WAYS TO DO THIS.  

    //  Allocate space for the color buffer.  
    colorBuffer = new U08[colorBufferSize];

    //  Change state to SWAP.  
    state = RAST_SWAP;
}

#define GAMMA(x) F32(GPU_POWER(F64(x), F64(1.0f / 2.2f)))
#define LINEAR(x) F32(GPU_POWER(F64(x), F64(2.2f)))
//#define GAMMA(x) (x)

//#define SAVE_AS_PNG

//  Writes the current color buffer as a ppm file.  
void cmoDisplayController::writeColorBuffer()
{
    char filename[256];

#ifdef SAVE_AS_PNG
    static U08 *data = 0;
    static U32 dataSize = 0;
    
    //  Check if the buffer is large enough.
    if (dataSize < (hRes * vRes * 4))
    {
        //  Check if the old buffer must be deleted.
        if (data != NULL)
            delete[] data;
            
        //  Set the new buffer size.            
        dataSize = hRes * vRes * 4;
        
        //  Create the new buffer.
        data = new U08[dataSize];
    }
#endif

    if (lastRSCommand->getCommand() == RSCOM_DUMP_COLOR)
    {
        //  Create current frame filename.
        sprintf(filename, "frame%04d-batch%05d.cm", frameCounter, batchCounter);
    }
    else
    {
        //  Create current frame filename.
        sprintf(filename, "frame%04d.cm", frameCounter);
    }

    
#ifndef SAVE_AS_PNG
    FILE *fout;

    char filenamePPM[128];
    
    sprintf(filenamePPM, "%s.ppm", filename);
    
    //  Open/Create the file for the current frame.
    fout = fopen(filenamePPM, "wb");

    //  Check if the file was correctly created.
    CG_ASSERT_COND(!(fout == NULL), "Error creating frame color output file.");
    //  Write file header.

    //  Write magic number.
    fprintf(fout, "P6\n");

    //  Write frame size.
    fprintf(fout, "%d %d\n", hRes, vRes);

    //  Write color component maximum value.
    fprintf(fout, "255\n");
#endif    


    U08 red = 255;
    U08 green = 255;
    U08 blue = 255;

    //  Check if multisampling is enabled.
    if (!multisampling)
    {    
        S32 top = d3d9PixelCoordinates ? 0 : (vRes -1);
        S32 bottom = d3d9PixelCoordinates ? (vRes - 1) : 0;
        S32 nextLine = d3d9PixelCoordinates ? +1 : -1;
        
        //  Do this for the whole picture now.
        for (S32 y = top; y != (bottom + nextLine); y = y + nextLine)
        {
            for (S32 x = 0; x < S32(hRes); x++)
            {
                //*  NOTE THERE SURE ARE FASTER WAYS TO DO THIS ...  *
                
                U32 address = colorPixelMapper.computeAddress(x, y);
                
                //  Convert data from the color color buffer to 8-bit PPM format.
                switch(colorBufferFormat)
                {
                    case GPU_RGBA8888:
                        red   = colorBuffer[address];
                        green = colorBuffer[address + 1];
                        blue  = colorBuffer[address + 2];
                        break;
                        
                    case GPU_RG16F:
                        red   = U08(GPU_MIN(GPU_MAX(GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 0])), 0.0f), 1.0f) * 255.0f);
                        green = U08(GPU_MIN(GPU_MAX(GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 2])), 0.0f), 1.0f) * 255.0f);
                        blue  = 0;
                        break;
                        
                    case GPU_R32F:
                    
                        red   = U08(GPU_MIN(GPU_MAX(*((F32 *) &colorBuffer[address]), 0.0f), 1.0f) * 255.0f);
                        green = 0;
                        blue  = 0;
                        break;

                    case GPU_RGBA16:
                        red   = U08(GPU_MIN(GPU_MAX((*((U16 *) &colorBuffer[address + 0])) / 65535.0f, 0.0f), 1.0f) * 255.0f);
                        green = U08(GPU_MIN(GPU_MAX((*((U16 *) &colorBuffer[address + 2])) / 65535.0f, 0.0f), 1.0f) * 255.0f);
                        blue  = U08(GPU_MIN(GPU_MAX((*((U16 *) &colorBuffer[address + 4])) / 65535.0f, 0.0f), 1.0f) * 255.0f);
                        break;
                        
                    case GPU_RGBA16F:
                        red   = U08(GPU_MIN(GPU_MAX(GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 0])), 0.0f), 1.0f) * 255.0f);
                        green = U08(GPU_MIN(GPU_MAX(GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 2])), 0.0f), 1.0f) * 255.0f);
                        blue  = U08(GPU_MIN(GPU_MAX(GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 4])), 0.0f), 1.0f) * 255.0f);
                        break;
                }
                
                //  Write color to the file.
#ifndef SAVE_AS_PNG                
                fputc((char) red, fout);
                fputc((char) green, fout);
                fputc((char) blue, fout);
#else                
                U32 yImage = d3d9PixelCoordinates ? y : (vRes - 1) - y;
                data[(yImage * hRes + x) * 4 + 2] = red;
                data[(yImage * hRes + x) * 4 + 1] = green;
                data[(yImage * hRes + x) * 4 + 0] = blue;
                //data[(yImage * hRes + x) * 4 + 3] = alpha;
                data[(yImage * hRes + x) * 4 + 3] = 255;
#endif                
            }
        }
    }
    else
    {
        Vec4FP32 sampleColors[MAX_MSAA_SAMPLES];
        Vec4FP32 resolvedColor;
        Vec4FP32 referenceColor;
        Vec4FP32 currentColor;
        
        S32 top = d3d9PixelCoordinates ? 0 : (vRes - 1);
        S32 bottom = d3d9PixelCoordinates ? (vRes - 1) : 0;
        
        //  Do this for the whole picture now.
        for (S32 y = top; y >= bottom; y--)
        {
            for(S32 x = 0; x < S32(hRes); x++)
            {               
                U32 address = colorPixelMapper.computeAddress(x, y);

                //  Zero resolved color.
                resolvedColor[0] = 0.0f;
                resolvedColor[1] = 0.0f;
                resolvedColor[2] = 0.0f;
                
                //  Convert data from the color color buffer 32 bit fp internal format.
                switch(colorBufferFormat)
                {
                    case GPU_RGBA8888:
                        referenceColor[0] = F32(colorBuffer[address + 0]) / 255.0f;
                        referenceColor[1] = F32(colorBuffer[address + 1]) / 255.0f;
                        referenceColor[2] = F32(colorBuffer[address + 2]) / 255.0f;
                        break;
                
                    case GPU_RG16F:
                        referenceColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 0]));
                        referenceColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 2]));
                        referenceColor[2] = 0.0f;
                        break;
                        
                    case GPU_R32F:
                        referenceColor[0] = *((F32 *) &colorBuffer[address]);
                        referenceColor[1] = 0.0f;
                        referenceColor[2] = 0.0f;
                        break;                        
                                
                    case GPU_RGBA16:
                        referenceColor[0] = (*((U16 *) &colorBuffer[address + 0])) / 65535.0f;
                        referenceColor[1] = (*((U16 *) &colorBuffer[address + 2])) / 65535.0f;
                        referenceColor[2] = (*((U16 *) &colorBuffer[address + 4])) / 65535.0f;
                        break;

                    case GPU_RGBA16F:
                        referenceColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 0]));
                        referenceColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 2]));
                        referenceColor[2] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 4]));
                        break;
                }
                
                bool fullCoverage = true;

                //  Accumulate the color for all the samples in the pixel
                for(U32 i = 0; i < msaaSamples; i++)
                {
                    //  Convert data from the color color buffer 32 bit fp internal format.
                    switch(colorBufferFormat)
                    {
                        case GPU_RGBA8888:
                            currentColor[0] = F32(colorBuffer[address + i * 4 + 0]) / 255.0f;
                            currentColor[1] = F32(colorBuffer[address + i * 4 + 1]) / 255.0f;
                            currentColor[2] = F32(colorBuffer[address + i * 4 + 2]) / 255.0f;
                            break;
                            
                        case GPU_RG16F:
                            currentColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + i * 4 + 0]));
                            currentColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + i * 4 + 2]));
                            currentColor[2] = 0.0f;
                            break;
                            
                        case GPU_R32F:
                            currentColor[0] = *((F32 *) &colorBuffer[address + i * 4]);
                            currentColor[1] = 0.0f;
                            currentColor[2] = 0.0f;
                            break;                        

                        case GPU_RGBA16:
                            currentColor[0] = (*((U16 *) &colorBuffer[address + i * 8 + 0])) / 65535.0f;
                            currentColor[1] = (*((U16 *) &colorBuffer[address + i * 8 + 2])) / 65535.0f;
                            currentColor[2] = (*((U16 *) &colorBuffer[address + i * 8 + 4])) / 65535.0f;
                            break;
                                    
                        case GPU_RGBA16F:
                            currentColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + i * 8 + 0]));
                            currentColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + i * 8 + 2]));
                            currentColor[2] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + i * 8 + 4]));
                            break;
                    }

                    sampleColors[i][0] = currentColor[0];
                    sampleColors[i][1] = currentColor[1];
                    sampleColors[i][2] = currentColor[2];
                    
                    resolvedColor[0] += LINEAR(currentColor[0]);
                    resolvedColor[1] += LINEAR(currentColor[1]);
                    resolvedColor[2] += LINEAR(currentColor[2]);
                    
                    fullCoverage = fullCoverage && (referenceColor[0] == currentColor[0])
                                                && (referenceColor[1] == currentColor[1])
                                                && (referenceColor[2] == currentColor[2]);
                }
                
                //  Check if there is a single sample for the pixel
                if (fullCoverage)
                {
                    //  Convert from RGBA32F (internal format) to RGBA8 (PPM format)
                    red   = U08(GPU_MIN(GPU_MAX(referenceColor[0], 0.0f), 1.0f) * 255.0f);
                    green = U08(GPU_MIN(GPU_MAX(referenceColor[1], 0.0f), 1.0f) * 255.0f);
                    blue  = U08(GPU_MIN(GPU_MAX(referenceColor[2], 0.0f), 1.0f) * 255.0f);
                    
                }
                else
                {
                    //  Resolve color as the average of all the sample colors.
                    resolvedColor[0] = GAMMA(resolvedColor[0] / F32(msaaSamples));
                    resolvedColor[1] = GAMMA(resolvedColor[1] / F32(msaaSamples));
                    resolvedColor[2] = GAMMA(resolvedColor[2] / F32(msaaSamples));

                    //  Convert from RGBA32F (internal format) to RGBA8 (PPM format)
                    red   = U08(GPU_MIN(GPU_MAX(resolvedColor[0], 0.0f), 1.0f) * 255.0f);
                    green = U08(GPU_MIN(GPU_MAX(resolvedColor[1], 0.0f), 1.0f) * 255.0f);
                    blue  = U08(GPU_MIN(GPU_MAX(resolvedColor[2], 0.0f), 1.0f) * 255.0f);
                }

#ifndef SAVE_AS_PNG
                //  Write pixel color into the output file.
                fputc(char(red)  , fout);
                fputc(char(green), fout);
                fputc(char(blue) , fout);
#else   //  SAVE_AS_PNG
                U32 yImage = d3d9PixelCoordinates ? y : (vRes - 1) - y;
                data[(yImage * hRes + x) * 4 + 2] = red;
                data[(yImage * hRes + x) * 4 + 1] = green;
                data[(yImage * hRes + x) * 4 + 0] = blue;
                //data[(yImage * hRes + x) * 4 + 3] = alpha;
                data[(yImage * hRes + x) * 4 + 3] = 255;
#endif  //  SAVE_AS_PNG                    

            }                    
        }                
    }
    
#ifndef SAVE_AS_PNG    
    fclose(fout);
#else    
    ImageSaver::getInstance().savePNG(filename, hRes, vRes, data);
#endif    
}


//  Writes the current depth buffer to a file.
void cmoDisplayController::writeDepthBuffer()
{
    char filename[128];

#ifdef SAVE_AS_PNG
    static U08 *data = 0;
    static U32 dataSize = 0;
    
    //  Check if the buffer is large enough.
    if (dataSize < (hRes * vRes * 4))
    {
        //  Check if the old buffer must be deleted.
        if (data != NULL)
            delete[] data;
            
        //  Set the new buffer size.            
        dataSize = hRes * vRes * 4;
        
        //  Create the new buffer.
        data = new U08[dataSize];
    }

#endif

    if (lastRSCommand->getCommand() == RSCOM_DUMP_DEPTH)
    {
        //  Create current frame filename.
        sprintf(filename, "depth%04d-batch%05d.sim", frameCounter, batchCounter);
    }
    else
    {
        //  Create current frame filename.
        sprintf(filename, "depth%04d.sim", frameCounter);
    }
    
#ifndef SAVE_AS_PNG
    
    char filenamePPM[128];

    //  Add file extension.
    sprintf(filenamePPM, "%s.ppm", filename);
    
    FILE *fout;

    //  Open/Create the file for the current frame.
    fout = fopen(filenamePPM, "wb");

    //  Check if the file was correctly created.
    CG_ASSERT_COND(!(fout == NULL), "Error creating depth output file.");
    //  Write file header.

    //  Write magic number.
    fprintf(fout, "P6\n");

    //  Write frame size.  */
    fprintf(fout, "%d %d\n", hRes, vRes);

    //  Write color component maximum value.
    fprintf(fout, "255\n");
#endif

    U08 red = 255;
    U08 green = 255;
    U08 blue = 255;

    //  Check if multisampling is enabled.
    if (!multisampling)
    {    
        S32 top = d3d9PixelCoordinates ? 0 : (vRes -1);
        S32 bottom = d3d9PixelCoordinates ? (vRes - 1) : 0;
        S32 nextLine = d3d9PixelCoordinates ? +1 : -1;
        
        //  Do this for the whole picture now.
        for (S32 y = top; y != (bottom + nextLine); y = y + nextLine)
        {
            for (S32 x = 0; x < S32(hRes); x++)
            {
                //  Compute address of the pixel.
                U32 address = zstPixelMapper.computeAddress(x, y);
                
                // Invert for better reading with irfan view.
                red   = zstBuffer[address + 2];
                green = zstBuffer[address + 1];
                blue  = zstBuffer[address + 0];                      
                
                //  Write color to the file.
#ifndef SAVE_AS_PNG                
                fputc((char) red, fout);
                fputc((char) green, fout);
                fputc((char) blue, fout);
#else                
                U32 yImage = d3d9PixelCoordinates ? y : (vRes - 1) - y;
                data[(yImage * hRes + x) * 4 + 2] = red;
                data[(yImage * hRes + x) * 4 + 1] = green;
                data[(yImage * hRes + x) * 4 + 0] = blue;
                //data[(yImage * hRes + x) * 4 + 3] = alpha;
                data[(yImage * hRes + x) * 4 + 3] = 255;
#endif                
            }
        }
    }
    else
    {
    
        CG_ASSERT("Multisampling not supported.");
        
        Vec4FP32 sampleColors[MAX_MSAA_SAMPLES];
        Vec4FP32 resolvedColor;
        Vec4FP32 referenceColor;
        Vec4FP32 currentColor;
        
        S32 top = d3d9PixelCoordinates ? 0 : (vRes - 1);
        S32 bottom = d3d9PixelCoordinates ? (vRes - 1) : 0;
        
        //  Do this for the whole picture now.
        for (S32 y = top; y >= bottom; y--)
        {
            for(S32 x = 0; x < S32(hRes); x++)
            {               
                U32 address = zstPixelMapper.computeAddress(x, y);

                //  Zero resolved color.
                resolvedColor[0] = 0.0f;
                resolvedColor[1] = 0.0f;
                resolvedColor[2] = 0.0f;
                
                //  Convert data from the color color buffer 32 bit fp internal format.
                switch(colorBufferFormat)
                {
                    case GPU_RGBA8888:
                        referenceColor[0] = F32(colorBuffer[address + 0]) / 255.0f;
                        referenceColor[1] = F32(colorBuffer[address + 1]) / 255.0f;
                        referenceColor[2] = F32(colorBuffer[address + 2]) / 255.0f;
                        break;
                
                    case GPU_RG16F:
                        referenceColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 0]));
                        referenceColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 2]));
                        referenceColor[2] = 0.0f;
                        break;
                        
                    case GPU_R32F:
                        referenceColor[0] = *((F32 *) &colorBuffer[address]);
                        referenceColor[1] = 0.0f;
                        referenceColor[2] = 0.0f;
                        break;                        
                                
                    case GPU_RGBA16:
                        referenceColor[0] = (*((U16 *) &colorBuffer[address + 0])) / 65535.0f;
                        referenceColor[1] = (*((U16 *) &colorBuffer[address + 2])) / 65535.0f;
                        referenceColor[2] = (*((U16 *) &colorBuffer[address + 4])) / 65535.0f;
                        break;

                    case GPU_RGBA16F:
                        referenceColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 0]));
                        referenceColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 2]));
                        referenceColor[2] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 4]));
                        break;
                }
                
                bool fullCoverage = true;

                //  Accumulate the color for all the samples in the pixel
                for(U32 i = 0; i < msaaSamples; i++)
                {
                    //  Convert data from the color color buffer 32 bit fp internal format.
                    switch(colorBufferFormat)
                    {
                        case GPU_RGBA8888:
                            currentColor[0] = F32(colorBuffer[address + i * 4 + 0]) / 255.0f;
                            currentColor[1] = F32(colorBuffer[address + i * 4 + 1]) / 255.0f;
                            currentColor[2] = F32(colorBuffer[address + i * 4 + 2]) / 255.0f;
                            break;
                            
                        case GPU_RG16F:
                            currentColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + i * 4 + 0]));
                            currentColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + i * 4 + 2]));
                            currentColor[2] = 0.0f;
                            break;
                            
                        case GPU_R32F:
                            currentColor[0] = *((F32 *) &colorBuffer[address + i * 4]);
                            currentColor[1] = 0.0f;
                            currentColor[2] = 0.0f;
                            break;                        
                                    
                        case GPU_RGBA16:
                            currentColor[0] = (*((U16 *) &colorBuffer[address + i * 8 + 0])) / 65535.0f;
                            currentColor[1] = (*((U16 *) &colorBuffer[address + i * 8 + 2])) / 65535.0f;
                            currentColor[2] = (*((U16 *) &colorBuffer[address + i * 8 + 4])) / 65535.0f;
                            break;

                        case GPU_RGBA16F:
                            currentColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + i * 8 + 0]));
                            currentColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + i * 8 + 2]));
                            currentColor[2] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + i * 8 + 4]));
                            break;
                    }

                    sampleColors[i][0] = currentColor[0];
                    sampleColors[i][1] = currentColor[1];
                    sampleColors[i][2] = currentColor[2];
                    
                    resolvedColor[0] += LINEAR(currentColor[0]);
                    resolvedColor[1] += LINEAR(currentColor[1]);
                    resolvedColor[2] += LINEAR(currentColor[2]);
                    
                    fullCoverage = fullCoverage && (referenceColor[0] == currentColor[0])
                                                && (referenceColor[1] == currentColor[1])
                                                && (referenceColor[2] == currentColor[2]);
                }
                
                //  Check if there is a single sample for the pixel
                if (fullCoverage)
                {
                    //  Convert from RGBA32F (internal format) to RGBA8 (PPM format)
                    red   = U08(GPU_MIN(GPU_MAX(referenceColor[0], 0.0f), 1.0f) * 255.0f);
                    green = U08(GPU_MIN(GPU_MAX(referenceColor[1], 0.0f), 1.0f) * 255.0f);
                    blue  = U08(GPU_MIN(GPU_MAX(referenceColor[2], 0.0f), 1.0f) * 255.0f);
                }
                else
                {
                    //  Resolve color as the average of all the sample colors.
                    resolvedColor[0] = GAMMA(resolvedColor[0] / F32(msaaSamples));
                    resolvedColor[1] = GAMMA(resolvedColor[1] / F32(msaaSamples));
                    resolvedColor[2] = GAMMA(resolvedColor[2] / F32(msaaSamples));

                    //  Convert from RGBA32F (internal format) to RGBA8 (PPM format)
                    red   = U08(GPU_MIN(GPU_MAX(resolvedColor[0], 0.0f), 1.0f) * 255.0f);
                    green = U08(GPU_MIN(GPU_MAX(resolvedColor[1], 0.0f), 1.0f) * 255.0f);
                    blue  = U08(GPU_MIN(GPU_MAX(resolvedColor[2], 0.0f), 1.0f) * 255.0f);
                }

#ifndef SAVE_AS_PNG
                //  Write pixel color into the output file.
                fputc(char(red)  , fout);
                fputc(char(green), fout);
                fputc(char(blue) , fout);
#else   //  SAVE_AS_PNG
                U32 yImage = d3d9PixelCoordinates ? y : (vRes - 1) - y;
                data[(yImage * hRes + x) * 4 + 2] = red;
                data[(yImage * hRes + x) * 4 + 1] = green;
                data[(yImage * hRes + x) * 4 + 0] = blue;
                //data[(yImage * hRes + x) * 4 + 3] = alpha;
                data[(yImage * hRes + x) * 4 + 3] = 255;
#endif  //  SAVE_AS_PNG                    
            }                    
        }                
    }
    
#ifndef SAVE_AS_PNG    
    fclose(fout);
#else    
    ImageSaver::getInstance().savePNG(filename, hRes, vRes, data);
#endif    
}

void cmoDisplayController::writeStencilBuffer()
{
    char filename[128];

#ifdef SAVE_AS_PNG
    static U08 *data = 0;
    static U32 dataSize = 0;
    
    //  Check if the buffer is large enough.
    if (dataSize < (hRes * vRes * 4))
    {
        //  Check if the old buffer must be deleted.
        if (data != NULL)
            delete[] data;
            
        //  Set the new buffer size.            
        dataSize = hRes * vRes * 4;
        
        //  Create the new buffer.
        data = new U08[dataSize];
    }
#endif

    if (lastRSCommand->getCommand() == RSCOM_DUMP_STENCIL)
    {
        //  Create current frame filename.
        sprintf(filename, "stencil%04d-batch%05d.sim", frameCounter, batchCounter);
    }
    else
    {
        //  Create current frame filename.
        sprintf(filename, "stencil%04d.sim", frameCounter);
    }
    
#ifndef SAVE_AS_PNG

    FILE *fout;

    char filenamePPM[128];

    //  Add extension.
    sprintf(filenamePPM, "%s.ppm", filename);
        
    //  Open/Create the file for the current frame.
    fout = fopen(filenamePPM, "wb");

    //  Check if the file was correctly created.
    CG_ASSERT_COND(!(fout == NULL), "Error creating stencil output file.");
    //  Write file header.

    //  Write magic number.
    fprintf(fout, "P6\n");

    //  Write frame size.  */
    fprintf(fout, "%d %d\n", hRes, vRes);

    //  Write color component maximum value.
    fprintf(fout, "255\n");

#endif

    U08 red = 255;
    U08 green = 255;
    U08 blue = 255;

    //  Check if multisampling is enabled.
    if (!multisampling)
    {    
        S32 top = d3d9PixelCoordinates ? 0 : (vRes -1);
        S32 bottom = d3d9PixelCoordinates ? (vRes - 1) : 0;
        S32 nextLine = d3d9PixelCoordinates ? +1 : -1;
        
        //  Do this for the whole picture now.
        for (S32 y = top; y != (bottom + nextLine); y = y + nextLine)
        {
            for (S32 x = 0; x < S32(hRes); x++)
            {
                //  Compute address of the pixel.
                U32 address = zstPixelMapper.computeAddress(x, y);
                
                //  Put some color to make small differences more easy to discover.
                red   = zstBuffer[address + 3];
                green = red & 0xF0;
                blue  = (red & 0x0F) << 4;
                
#ifndef SAVE_AS_PNG                
                fputc((char) red, fout);
                fputc((char) green, fout);
                fputc((char) blue, fout);
#else                
                U32 yImage = d3d9PixelCoordinates ? y : (vRes - 1) - y;
                data[(yImage * hRes + x) * 4 + 2] = red;
                data[(yImage * hRes + x) * 4 + 1] = green;
                data[(yImage * hRes + x) * 4 + 0] = blue;
                //data[(yImage * hRes + x) * 4 + 3] = alpha;
                data[(yImage * hRes + x) * 4 + 3] = 255;
#endif                
            }
        }
    }
    else
    {
    
        CG_ASSERT("Multisampling not supported.");
        
        Vec4FP32 sampleColors[MAX_MSAA_SAMPLES];
        Vec4FP32 resolvedColor;
        Vec4FP32 referenceColor;
        Vec4FP32 currentColor;
        
        S32 top = d3d9PixelCoordinates ? 0 : (vRes - 1);
        S32 bottom = d3d9PixelCoordinates ? (vRes - 1) : 0;
        
        //  Do this for the whole picture now.
        for (S32 y = top; y >= bottom; y--)
        {
            for(S32 x = 0; x < S32(hRes); x++)
            {               
                U32 address = zstPixelMapper.computeAddress(x, y);

                //  Zero resolved color.
                resolvedColor[0] = 0.0f;
                resolvedColor[1] = 0.0f;
                resolvedColor[2] = 0.0f;
                
                //  Convert data from the color color buffer 32 bit fp internal format.
                switch(colorBufferFormat)
                {
                    case GPU_RGBA8888:
                        referenceColor[0] = F32(colorBuffer[address + 0]) / 255.0f;
                        referenceColor[1] = F32(colorBuffer[address + 1]) / 255.0f;
                        referenceColor[2] = F32(colorBuffer[address + 2]) / 255.0f;
                        break;
                
                    case GPU_RG16F:
                        referenceColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 0]));
                        referenceColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 2]));
                        referenceColor[2] = 0.0f;
                        break;
                        
                    case GPU_R32F:
                        referenceColor[0] = *((F32 *) &colorBuffer[address]);
                        referenceColor[1] = 0.0f;
                        referenceColor[2] = 0.0f;
                        break;                        
                                
                    case GPU_RGBA16:
                        referenceColor[0] = (*((U16 *) &colorBuffer[address + 0])) / 65535.0f;
                        referenceColor[1] = (*((U16 *) &colorBuffer[address + 2])) / 65535.0f;
                        referenceColor[2] = (*((U16 *) &colorBuffer[address + 4])) / 65535.0f;
                        break;

                    case GPU_RGBA16F:
                        referenceColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 0]));
                        referenceColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 2]));
                        referenceColor[2] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 4]));
                        break;
                }
                
                bool fullCoverage = true;

                //  Accumulate the color for all the samples in the pixel
                for(U32 i = 0; i < msaaSamples; i++)
                {
                    //  Convert data from the color color buffer 32 bit fp internal format.
                    switch(colorBufferFormat)
                    {
                        case GPU_RGBA8888:
                            currentColor[0] = F32(colorBuffer[address + i * 4 + 0]) / 255.0f;
                            currentColor[1] = F32(colorBuffer[address + i * 4 + 1]) / 255.0f;
                            currentColor[2] = F32(colorBuffer[address + i * 4 + 2]) / 255.0f;
                            break;
                            
                        case GPU_RG16F:
                            currentColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + i * 4 + 0]));
                            currentColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + i * 4 + 2]));
                            currentColor[2] = 0.0f;
                            break;
                            
                        case GPU_R32F:
                            currentColor[0] = *((F32 *) &colorBuffer[address + i * 4]);
                            currentColor[1] = 0.0f;
                            currentColor[2] = 0.0f;
                            break;                        
                                    
                        case GPU_RGBA16:
                            currentColor[0] = (*((U16 *) &colorBuffer[address + i * 8 + 0])) / 65535.0f;
                            currentColor[1] = (*((U16 *) &colorBuffer[address + i * 8 + 2])) / 65535.0f;
                            currentColor[2] = (*((U16 *) &colorBuffer[address + i * 8 + 4])) / 65535.0f;
                            break;

                        case GPU_RGBA16F:
                            currentColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + i * 8 + 0]));
                            currentColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + i * 8 + 2]));
                            currentColor[2] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + i * 8 + 4]));
                            break;
                    }

                    sampleColors[i][0] = currentColor[0];
                    sampleColors[i][1] = currentColor[1];
                    sampleColors[i][2] = currentColor[2];
                    
                    resolvedColor[0] += LINEAR(currentColor[0]);
                    resolvedColor[1] += LINEAR(currentColor[1]);
                    resolvedColor[2] += LINEAR(currentColor[2]);
                    
                    fullCoverage = fullCoverage && (referenceColor[0] == currentColor[0])
                                                && (referenceColor[1] == currentColor[1])
                                                && (referenceColor[2] == currentColor[2]);
                }
                
                //  Check if there is a single sample for the pixel
                if (fullCoverage)
                {
                    //  Convert from RGBA32F (internal format) to RGBA8 (PPM format)
                    red   = U08(GPU_MIN(GPU_MAX(referenceColor[0], 0.0f), 1.0f) * 255.0f);
                    green = U08(GPU_MIN(GPU_MAX(referenceColor[1], 0.0f), 1.0f) * 255.0f);
                    blue  = U08(GPU_MIN(GPU_MAX(referenceColor[2], 0.0f), 1.0f) * 255.0f);
                }
                else
                {
                    //  Resolve color as the average of all the sample colors.
                    resolvedColor[0] = GAMMA(resolvedColor[0] / F32(msaaSamples));
                    resolvedColor[1] = GAMMA(resolvedColor[1] / F32(msaaSamples));
                    resolvedColor[2] = GAMMA(resolvedColor[2] / F32(msaaSamples));

                    //  Convert from RGBA32F (internal format) to RGBA8 (PPM format)
                    red   = U08(GPU_MIN(GPU_MAX(resolvedColor[0], 0.0f), 1.0f) * 255.0f);
                    green = U08(GPU_MIN(GPU_MAX(resolvedColor[1], 0.0f), 1.0f) * 255.0f);
                    blue  = U08(GPU_MIN(GPU_MAX(resolvedColor[2], 0.0f), 1.0f) * 255.0f);
                }

#ifndef SAVE_AS_PNG
                //  Write pixel color into the output file.
                fputc(char(red)  , fout);
                fputc(char(green), fout);
                fputc(char(blue) , fout);
#else   //  SAVE_AS_PNG
                U32 yImage = d3d9PixelCoordinates ? y : (vRes - 1) - y;
                data[(yImage * hRes + x) * 4 + 2] = red;
                data[(yImage * hRes + x) * 4 + 1] = green;
                data[(yImage * hRes + x) * 4 + 0] = blue;
                //data[(yImage * hRes + x) * 4 + 3] = alpha;
                data[(yImage * hRes + x) * 4 + 3] = 255;
#endif  //  SAVE_AS_PNG                    
            }                    
        }                
    }
    
#ifndef SAVE_AS_PNG    
    fclose(fout);
#else    
    ImageSaver::getInstance().savePNG(filename, hRes, vRes, data);
#endif    
}

//  Translates an address to the start of a block into a block number.  
U32 cmoDisplayController::address2block(U32 address)
{
    U32 block;

    //  Translate start block/line address to a block address.  
    block = address >> blockShift;

    return block;
}

//  Calculates the block assigned stamp unit.  
U32 cmoDisplayController::blockUnit(PixelMapper &pixelMapper, U32 block)
{
    return pixelMapper.mapToUnit(block << blockShift);
    
/*
    U32 overTile;
    U32 overLine;
    U32 overX;
    U32 overY;
    U32 scanTile;
    U32 scanX;
    U32 scanY;
    U32 x;
    U32 y;

    //  Assignment of blocks to stamp units is done on a scan tile basis.
    //  The block address (linear memory address) is translated into framebuffer
    //  (x, y) coordinates to get the scan tile coordinates for the block.

    //if (multisampling)
    //    block = block / msaaSamples;

    //if (colorBufferFormat == GPU_RGBA16F)
    //    block = block >> 1;
        
    U32 samples = multisampling ? msaaSamples : 1;
    U32 colorDWords = (colorBufferFormat == GPU_RGBA16F) ? 2 : 1;
            
    //  Calculate the over tile address inside the framebuffer.
    overTile = block / (overW * scanW * overH * scanH * samples * colorDWords);

    //  Calculate size number of horizontal over tiles.
    overLine = U32(ceil(hRes / (F32(overW * scanW * genW * STAMP_WIDTH))));

    //  Calculate the block vertical and horizontal over tile coordinates inside the framebuffer.
    overX = GPU_MOD(overTile, overLine);
    overY = overTile / overLine;

    //  Calculate the scan tile address inside the over scan tile.
    scanTile = GPU_MOD(block, overW * scanW * overH * scanH * samples * colorDWords) / (scanW * scanH);

    //  Calculate the block vertical and horizontal scan tile coordinates inside the over tile (per row).
    //scanX = GPU_MOD(scanTile, overW);
    //scanY = scanTile / overW;

    //  Calculate the block vertical and horizontal scan tile coordinates inside the over tile (morton).
    scanX = 0;
    scanY = 0;
    for(U32 i = 0; i < U32(CG_CEIL2(CG_LOG2(overW))); i++)
    {
        scanX = scanX | ((scanTile & 0x01) << i);
        scanTile = scanTile >> 1;
        scanY = scanY | ((scanTile & 0x01) << i);
        scanTile = scanTile >> 1;
    }

    //  Calculate the scan tile vertical and horizontal coordinates inside the framebuffer.
    x = scanX + overX * overW;
    y = scanY + overY * overH;

    if (numStampUnits == 1)
        return 0;

    if (numStampUnits == 2)
        return ((x + y) & 0x01);

    if (numStampUnits == 4)
        return  GPU_MOD((x & 0x01) + ((y & 0x01) << 1), numStampUnits);
        
    if ((numStampUnits == 8) || (numStampUnits == 16))
        return  GPU_MOD((x & 0x01) + ((y & 0x01) << 1) + ((x & 0x02) << 1) + ((y & 0x02) << 2), numStampUnits);
    
    //  Remove VS2005 warning.
    return 0;
*/   
}

//  Update memory transmission cycles.
void cmoDisplayController::updateMemory(U64 cycle)
{
    //  Check if memory bus is busy.
    if (busCycles > 0)
    {
        //  Update bus use counter.
        busCycles--;

        GPU_DEBUG_BOX(
            printf("cmoDisplayController %lld => Transmission in progress.  Remaining cycles %d\n",
                cycle, busCycles);
        )

        //  Check end of transmission.
        if (busCycles == 0)
        {
            GPU_DEBUG_BOX(
                printf("cmoDisplayController => End of memory read.\n");
            )

            //  Update block queue entry received bytes.
            blockQueue[ticket2queue[lastTicket]].received += lastSize;

            //  Add ticket to the free ticket list.
            ticketList.add(lastTicket);
            
            //  Update free memory tickets counter.
            freeTickets++;
        }
    }
}

//  Update buffer block decompressor state.
void cmoDisplayController::updateDecompressor(U64 cycle, bmoCompressor *compressorEmulator, U08 *dumpBuffer, 
                             U08 *clearValue, U32 bytesPerPixel, U32 baseBufferAddress)
{
    U32 address;
    
    //  Update decompress cycles.
    if (decompressCycles > 0)
    {
        //  Update decompress cycles.
        decompressCycles--;

        GPU_DEBUG_BOX(
            printf("cmoDisplayController %lld => Decompressing block.  Remaining cycles %d\n",
                cycle, decompressCycles);
        )

        //  Check if end of decompression.
        if (decompressCycles == 0)
        {
            //  Update number of blocks to decompress.
            numToDecompress--;

            //  Update pointer to the next block to decompress.
            nextDecompress = GPU_MOD(nextDecompress + 1, blockQueueSize);

            //  Update number of free queue entries.  
            numFree++;
        }
    }

    //  Decompress a block.
    if ((numToDecompress > 0) && (decompressCycles == 0))
    {
        //  Check that the block has been received from memory.
        if (blockQueue[nextDecompress].received == blockQueue[nextDecompress].size)
        {
            GPU_DEBUG_BOX(
                printf("cmoDisplayController %lld => Decompressing block %d at address %x.\n",
                    cycle, blockQueue[nextDecompress].block, blockQueue[nextDecompress].address);
            )

            //  Update decompress cycles.
            decompressCycles = decompressLatency;

            //  Update statistic.
            stateBlocks++;
            
            const ROPBlockState& block = blockQueue[nextDecompress].state;
            
//printf("Compressed data (%d bytes) : \n    ", blockQueue[nextDecompress].size);
//for(U32 d = 0; d < blockQueue[nextDecompress].size; d++)
//printf("%02x ", blockBuffer[nextDecompress][d]);
//printf("\n");

            //  Select compression method.
            switch (block.state)
            {
                case ROPBlockState::CLEAR:

                    GPU_DEBUG_BOX(
                        printf("cmoDisplayController => CLEAR block.\n");
                    )

                    //  Compute address of the block inside the dump buffer.
                    address = blockQueue[nextDecompress].address - baseBufferAddress;
                    
                    //  Fill the block with the clear value.
                    for(U32 i = 0; i < blockSize; i += bytesPerPixel)
                    {
                        for(U32 j = 0; j < (bytesPerPixel >> 2); j++)
                            *((U32 *) &dumpBuffer[(address + i) + j * 4]) = ((U32 *) clearValue)[j];
                    }
                    
                    //  Update statistics.
                    stateBlocksClear++;
            
                    break;

                case ROPBlockState::UNCOMPRESSED:

                    GPU_DEBUG_BOX(
                        printf("cmoDisplayController => UNCOMPRESSED block.\n");
                    )

                    //  Compute address of the block inside the dump buffer.
                    address = blockQueue[nextDecompress].address - baseBufferAddress;
                    
                    //  Fill the block with the read data.
                    for(U32 i = 0; i < blockSize; i += 4)
                        *((U32 *) &dumpBuffer[address + i]) = *((U32 *) &blockBuffer[nextDecompress][i]);

//for(U32 i = 0, address = blockQueue[nextDecompress].address - frontBuffer;
//    i < blockSize; i += 4)
//    *((U32 *) &colorBuffer[address + i]) = 0x000000ff;

                    //  Update statistics.
                    stateBlocksUncompressed++;
                    
                    break;

                case ROPBlockState::COMPRESSED:

                    GPU_DEBUG_BOX(
                        printf("cmoDisplayController => COMPRESSED block with level %i.\n", block.comprLevel);
                    )

                    //  Compute address of the block inside the dump buffer.
                    address = blockQueue[nextDecompress].address - baseBufferAddress;

                    //  Uncompress the block.  
                    compressorEmulator->uncompress(blockBuffer[nextDecompress],  (U32 *) &dumpBuffer[address], 
                                                 blockSize >> 2, block.getComprLevel());

//for(U32 i = 0, address = blockQueue[nextDecompress].address - frontBuffer;
//    i < blockSize; i += 4)
//    *((U32 *) &colorBuffer[address + i]) = 0x00ff00ff;
                                            
                    //  Update stat.
                    switch (block.comprLevel)
                    {
                        case 0: stateBlocksCompressedBest++; break;
                        case 1: stateBlocksCompressedNormal++; break;
                        case 2: stateBlocksCompressedWorst++; break;
                        default:
                            GPU_DEBUG_BOX(
                                printf("cmoDisplayController => Warning: stattistics for compressor level %i not supported.\n", block.comprLevel);
                            )
                            break;                            
                    }
                    
                    break;

                default:
                    CG_ASSERT("Undefined compression method.");
                    break;
            }
//printf("cmoDisplayController => Decompressed block %x state %d at address %08x\n",
//blockQueue[nextDecompress].block, blockQueue[nextDecompress].state, blockQueue[nextDecompress].address);

//printf("Uncompressed data (%d bytes) : \n    ", blockSize);
//for(U32 b = 0; b < blockSize; b++)
//printf("%02x ", colorBuffer[address + b]);
//printf("\n");
        }
    }
}

//  Update the block request stage.
void cmoDisplayController::updateBlockRequest(U64 cycle)
{
    //  Check if memory controller and the bus are ready, there are free memory tickets and there is a block to request.
    if ((numToRequest > 0) && ((memState & MS_READ_ACCEPT) != 0) && (freeTickets > 0))
    {
        //  Check there are enough memory tickets.
        //GPU_ASSERT(
        //    if (freeTickets == 0)
        //        CG_ASSERT("No free memory tickets.");
        //)

        //  Determine if the block has been fully requested (check for clear blocks!!).
        if (blockQueue[nextRequest].size == blockQueue[nextRequest].requested)
        {
            GPU_DEBUG_BOX(
                printf("cmoDisplayController %lld => Block %d fully requested to memory.\n",
                    cycle, blockQueue[nextRequest].block);
            )

            //  Update number of blocks to request.
            numToRequest--;

            //  Update number of blocks to decompress.
            numToDecompress++;

            //  Update pointer to the next block to request.
            nextRequest = GPU_MOD(nextRequest + 1, blockQueueSize);
        }
        else
        {
            //  Request the block to memory.

            //  Calculate transaction size.
            U32 size = GPU_MIN(MAX_TRANSACTION_SIZE, blockQueue[nextRequest].size);

            GPU_DEBUG_BOX(
                printf("cmoDisplayController %lld => Requesting memory at %x %d bytes for block %d.\n",
                    cycle, blockQueue[nextRequest].address + blockQueue[nextRequest].requested,
                    size, blockQueue[nextRequest].block);
            )

            //  Get the next free ticket.
            U32 nextFreeTicket = ticketList.pop();
            
            //  Add queue entry to the translation table.
            ticket2queue[nextFreeTicket] = nextRequest;

            //  Create write transaction.
            MemoryTransaction *memTrans = new MemoryTransaction(
                MT_READ_REQ,
                blockQueue[nextRequest].address + blockQueue[nextRequest].requested,
                size,
                &blockBuffer[nextRequest][blockQueue[nextRequest].requested],
                DISPLAYCONTROLLER,
                nextFreeTicket
            );

            //  Copy cookies from the last command from the Command Processor.
            memTrans->copyParentCookies(*lastRSCommand);

            //  Add a cookie level.
            memTrans->addCookie();

            //  Send transaction to memory controller.
            memoryRequest->write(cycle, memTrans);

            //  Update requested bytes.
            blockQueue[nextRequest].requested += size;

            //  Update free ticket counter.
            freeTickets--;

            //  Determine if the block has been fully requested.
            if (blockQueue[nextRequest].size == blockQueue[nextRequest].requested)
            {
                GPU_DEBUG_BOX(
                    printf("cmoDisplayController %lld => Block %d fully requested to memory.\n",
                        cycle, blockQueue[nextRequest].block);
                )

                //  Update number of blocks to request.
                numToRequest--;

                //  Update number of blocks to decompress.
                numToDecompress++;

                //  Update pointer to the next block to request.
                nextRequest = GPU_MOD(nextRequest + 1, blockQueueSize);
            }
        }
    }
}

//  Update request queue stage.
void cmoDisplayController::updateRequestQueue(U64 cycle, bmoCompressor *compressorEmulator, U32 baseBufferAddress, U32 dumpBufferSize, ROPBlockState *bufferBlockState,
                             U32 bufferBlockSize)
{
    //  Add a block to the request queue.
    if ((stateUpdateCycles == 0) && (numFree > 0) && (requested < dumpBufferSize))
    {
        //  Add next block to the request queue.
        blockQueue[nextFree].address = baseBufferAddress + requested;
        blockQueue[nextFree].block = address2block(requested);
        blockQueue[nextFree].state = bufferBlockState[address2block(requested)];
        blockQueue[nextFree].requested = 0;
        blockQueue[nextFree].received = 0;

        const ROPBlockState& block = blockQueue[nextFree].state;
        
        //  Set block size.
        switch(block.state)
        {
            case ROPBlockState::CLEAR:

                GPU_DEBUG_BOX(
                    printf("cmoDisplayController => Adding request for CLEAR block %d\n",
                        blockQueue[nextFree].block);
                )

                blockQueue[nextFree].size = 0;

                break;

            case ROPBlockState::UNCOMPRESSED:

                GPU_DEBUG_BOX(
                    printf("cmoDisplayController => Adding request for UNCOMPRESSED block %d\n",
                        blockQueue[nextFree].block);
                )

                blockQueue[nextFree].size = bufferBlockSize;

                break;

            case ROPBlockState::COMPRESSED:

                GPU_DEBUG_BOX(
                    printf("cmoDisplayController => Adding request for COMPRESSED block %d with level %d\n",
                        blockQueue[nextFree].block, block.comprLevel);
                )

                blockQueue[nextFree].size = compressorEmulator->getLevelBlockSize(block.comprLevel); 

                break;

            default:
                CG_ASSERT("Undefined block state mode.");
                break;
        }

        //  Update requested bytes (blocks) counter.
        requested += bufferBlockSize;

        //  Update number free entries.
        numFree--;

        //  Update pointer to next free entry.
        nextFree = GPU_MOD(nextFree + 1, blockQueueSize);

        //  Update number of blocks to request.
        numToRequest++;
    }
}

//  Reset the state of the cmoDisplayController unit.
void cmoDisplayController::reset()
{
    //  Reset cmoDisplayController state and registers.
    colorBufferFormat = GPU_RGBA8888;
    bytesPixel = 4;
    hRes = 400;
    vRes = 400;
    d3d9PixelCoordinates = false;
    startX = 0;
    startY = 0;
    width = 400;
    height = 400;
    multisampling = false;
    msaaSamples = 2;
    zStencilCompression = true;
    colorCompression = true;
    
    blt->setMultisampling(multisampling);
    blt->setMSAASamples(msaaSamples);
    
    //  By default set front buffer at 1 MB
    //                 back buffer at 2 MB
    //                 z stencil buffer at 3 MBs
    frontBuffer    = 0x200000;
    backBuffer     = 0x400000;
    zStencilBuffer = 0x600000;
    
    blt->setBackBufferAddress(backBuffer);

    //  Reset clear color value.
    clearColor[0] = 0.0f;
    clearColor[1] = 0.0f;
    clearColor[2] = 0.0f;
    clearColor[3] = 1.0f;
    for(U32 i = 0; i < MAX_BYTES_PER_COLOR; i++)
        clearColorData[0] = 0x00;
    clearColorData[3] = 0xff;
    
    //  Reset depth and stencil clear values.
    clearDepth = 0x00ffffff;
    clearStencil = 0x00;
    
    //  Set to null the current color buffer state memory.
    colorBufferState = NULL;
    
    //  Set to null the current z stencil buffer state memory.
    zStencilBufferState = NULL;

    //  Reset color buffer state memory update cycles.
    stateUpdateCycles = 0;

    //  Fill the ticket list.
    ticketList.clear();
    for(U32 i = 0; i < MAX_MEMORY_TICKETS; i++)
        ticketList.add(i);
    
    //  Reset blitter unit.
    blt->reset();

    //  Reset blit operation counter.
    blitCounter = 0;
}


/*******************************************************************************
 *                      BLITTER TRANSACTION INTERFACE IMPLEMENTATION           *
 *******************************************************************************/

cmoDisplayController::BlitterTransactionInterfaceImpl::BlitterTransactionInterfaceImpl(cmoDisplayController& dac) :
    dac(dac)
{
} 

bool cmoDisplayController::BlitterTransactionInterfaceImpl::sendWriteTransaction(U64 cycle, U32 size, U32 address, U08* data, U32 id)
{
    //  Check if the memory controller accepts write requests.  
    if ((dac.memState & MS_WRITE_ACCEPT) != 0)
    {
        //  Create the new memory transaction.  
        MemoryTransaction* memTrans = new MemoryTransaction(
            MT_WRITE_DATA,
            address,
            size,
            data,
            DISPLAYCONTROLLER,
            id);
            
        //  Send memory transaction to the Memory Controller.  
        dac.memoryRequest->write(cycle, memTrans);

        /*
        //  If there is a Dynamic Object as request source copy its cookies.
        if (writeReq.request->source != NULL)
        {
            memTrans->copyParentCookies(*writeQueue[writeReq].request->source);
            memTrans->addCookie();
        }
        */
                    
        return true;
    }
    else
    {
        //  No memory transaction generated.  
        return false;
    }
}

bool cmoDisplayController::BlitterTransactionInterfaceImpl::sendMaskedWriteTransaction(U64 cycle, U32 size, U32 address, U08* data, U32* mask, U32 id)
{
    //  Check if the memory controller accepts write requests.  
    if ((dac.memState & MS_WRITE_ACCEPT) != 0)
    {
        //  Create the new memory transaction.  
        MemoryTransaction* memTrans = new MemoryTransaction(
            address,
            size,
            data,
            mask,
            DISPLAYCONTROLLER,
            id);
            
        //  Send memory transaction to the Memory Controller.  
        dac.memoryRequest->write(cycle, memTrans);

        /*
        //  If there is a Dynamic Object as request source copy its cookies.
        if (writeReq.request->source != NULL)
        {
            memTrans->copyParentCookies(*writeQueue[writeReq].request->source);
            memTrans->addCookie();
        }
        */
                    
        return true;
    }
    else
    {
        //  No memory transaction generated.  
        return false;
    }
}

bool cmoDisplayController::BlitterTransactionInterfaceImpl::sendReadTransaction(U64 cycle, U32 size, U32 address, U08* data, U32 ticket)
{
    //  Check if the memory controller accepts write requests.  
    if ((dac.memState & MS_READ_ACCEPT) != 0)
    {
        //  Create the new memory transaction.  
        MemoryTransaction* memTrans = new MemoryTransaction(
            MT_READ_REQ,
            address,
            size,
            data,
            DISPLAYCONTROLLER,
            ticket);
            
        //  Send memory transaction to the Memory Controller.  
        dac.memoryRequest->write(cycle, memTrans);

        /*
        //  If there is a Dynamic Object as request source copy its cookies.
        if (writeReq.request->source != NULL)
        {
            memTrans->copyParentCookies(*writeQueue[writeReq].request->source);
            memTrans->addCookie();
        }
        */
                    
        return true;
    }
    else
    {
        //  No memory transaction generated.  
        return false;
    }
}

gpuStatistics::StatisticsManager& cmoDisplayController::BlitterTransactionInterfaceImpl::getSM()
{
    return dac.getSM();
}

string cmoDisplayController::BlitterTransactionInterfaceImpl::getBoxName()
{
    return string("cmoDisplayController");
}
