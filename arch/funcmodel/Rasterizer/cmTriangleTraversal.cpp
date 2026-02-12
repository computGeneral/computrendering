/**************************************************************************
 *
 * Triangle Traversal mdu implementation file.
 *
 */

 /**
 *
 *  @file TriangleTraversal.cpp
 *
 *  This file implements the Triangle Traversal mdu.
 *
 *  This class traverses the triangle generating fragments.
 *
 */

#include "cmTriangleTraversal.h"
#include "cmHZStateInfo.h"
#include "cmTriangleSetupRequest.h"
#include "cmFragmentInput.h"
#include "cmRasterizerStateInfo.h"
#include <stdio.h>
#include <sstream>

using namespace arch;


//  Triangle Traversal mdu constructor.  
TriangleTraversal::TriangleTraversal(bmoRasterizer &rasEmu, U32 cycleTriangles, U32 setupLat,
    U32 stampsPerCycle, U32 zSamplesCycle, U32 triBatch, U32 trQSz,
    bool recursive, bool microTrisAsFrag, U32 nStampUnits,
    U32 overWidth, U32 overHeight, U32 scanWidth, U32 scanHeight,
    U32 genWidth, U32 genHeight, char *name, cmoMduBase *parent) :

    bmRaster(rasEmu), trianglesCycle(cycleTriangles), setupLatency(setupLat), stampsCycle(stampsPerCycle),
    samplesCycle(zSamplesCycle), triangleBatch(triBatch), triangleQSize(trQSz), recursiveMode(recursive),
    numStampUnits(nStampUnits),
    overH(overHeight), overW(overWidth),
    scanH((U32) ceil(scanHeight / (F32) genHeight)), scanW((U32) ceil(scanWidth / (F32) genWidth)),
    genH(genHeight / STAMP_HEIGHT), genW(genWidth / STAMP_WIDTH),
    cmoMduBase(name, parent)

{
    DynamicObject *defaultState[1];

    //  Create statistics.  
    inputs = &getSM().getNumericStatistic("InputTriangles", U32(0), "TriangleTraversal", "TT");
    requests = &getSM().getNumericStatistic("RequestedTriangles", U32(0), "TriangleTraversal", "TT");
    outputs = &getSM().getNumericStatistic("GeneratedFragments", U32(0), "TriangleTraversal", "TT");
    utilizationCycles = &getSM().getNumericStatistic("UtilizationCycles", U32(0), "TriangleTraversal", "TT");
    stamps0frag = &getSM().getNumericStatistic("Stamps0Fragment", U32(0), "TriangleTraversal", "TT");
    stamps1frag = &getSM().getNumericStatistic("Stamps1Fragment", U32(0), "TriangleTraversal", "TT");
    stamps2frag = &getSM().getNumericStatistic("Stamps2Fragment", U32(0), "TriangleTraversal", "TT");
    stamps3frag = &getSM().getNumericStatistic("Stamps3Fragment", U32(0), "TriangleTraversal", "TT");
    stamps4frag = &getSM().getNumericStatistic("Stamps4Fragment", U32(0), "TriangleTraversal", "TT");

    //  Check there are enough entries in the triangle queue.  
    GPU_ASSERT(
        if ((samplesCycle != 2) && (samplesCycle != 4) && (samplesCycle != 8))
            CG_ASSERT("Unsupported number of samples per cycle.  Currently supported values are 2, 4 and 8.");
        
        if (triangleQSize < trianglesCycle)
            CG_ASSERT("Triangle Queue must have at least triangles per cycle entries.");
    )

    //  Create signals.  

    //  Create command signal from the main rasterizer mdu.  
    traversalCommand = newInputSignal("TriangleTraversalCommand", 1, 1, NULL);

    //  Create state signal to the main rasterizer mdu.  
    traversalState = newOutputSignal("TriangleTraversalState", 1, 1, NULL);

    //  Create default signal value.  
    defaultState[0] = new RasterizerStateInfo(RAST_RESET);

    //  Set default signal value.  
    traversalState->setData(defaultState);

    //  Create setup triangle input from the Triangle Setup mdu.  
    setupTriangle = newInputSignal("TriangleSetupOutput", trianglesCycle, setupLatency, NULL);

    //  Create request signal to Triangle Setup.  
    setupRequest = newOutputSignal("TriangleSetupRequest", 1, 1, NULL);

    //  Create new fragment signal to the Hierarchical Z early test.  
    newFragment = newOutputSignal("HZInputFragment", stampsCycle * STAMP_FRAGMENTS, 1, NULL);

    //  Create state signal from the Hierarchical Z early test.  
    hzState = newInputSignal("HZTestState", 1, 1, NULL);

    //  Check that the triangle queue is large enough for the given batch size.  
    CG_ASSERT_COND(!(recursiveMode && (triangleQSize < (triangleBatch * 2))), "Triangle queue too small for given rasterization batch size.");
    //  Allocate space for the triangles to be traversed.  
    triangleQueue = new TriangleSetupOutput*[triangleQSize];

    //  Check allocation.  
    CG_ASSERT_COND(!(triangleQueue == NULL), "Error allocating triangle queue.");
    //  If scanline rasterization mode is used only one triangle supported per batch.  
    if (!recursiveMode)
        triangleBatch = 1;

    //  Set the number of ROP units in the Pixel Mapper.
    pixelMapper.setupUnit(numStampUnits, 0);
    
    //  Reset state.  
    state = RAST_RESET;

    //  Create a fake last rasterizer command.  
    lastRSCommand = new RasterizerCommand(RSCOM_RESET);

    //  Set orthogonal vertex position transformation.  
    perspectTransform = false;
}

//  Triangle Traversal simulation function.  
void TriangleTraversal::clock(U64 cycle)
{
    RasterizerCommand *rastCommand;
    HZStateInfo *hzStateInfo;
    HZState hzCurrentState;
    TriangleSetupOutput *tsOutput;
    TriangleSetupRequest *tsRequest;
    Fragment **stamp;
    DynamicObject *stampCookies;
    FragmentInput *frInput;
    U32 *triangleList;
    U32 triangleID;
    U32 stampUnit;
    TileIdentifier stampTileID;
    U32 i, j;
    bool rasterizationWorking;

    GPU_DEBUG_BOX(
        printf("TriangleTraversal => clock %lld.\n", cycle);
    )

    //  Receive state from the Hierarchical Z unit.  
    if (hzState->read(cycle, (DynamicObject *&) hzStateInfo))
    {
        //  Get HZ state.  
        hzCurrentState = hzStateInfo->getState();

        //  Delete HZ state info object.  
        delete hzStateInfo;
    }
    else
    {
        CG_ASSERT("Missing state signal from the Hierarchical Z mdu.");
    }

/*if ((cycle > 5024880) && (GPU_MOD(cycle, 1000) == 0))
{
printf("TT %lld => state ", cycle);
switch(state)
{
    case RAST_RESET:    printf("RESET\n"); break;
    case RAST_READY:    printf("READY\n"); break;
    case RAST_DRAWING:  printf("DRAWING\n"); break;
    case RAST_END:      printf("END\n"); break;
}
printf("TT => stored triangles %d requested triangles %d last fragment %s\n",
storedTriangles, requestedTriangles, lastFragment?"T":"F");
}*/

    //  Simulate current cycle.  
    switch(state)
    {
        case RAST_RESET:
            //  Reset state.  

            GPU_DEBUG_BOX(
                printf("TriangleTraversal => RESET state.\n");
            )

            //  Reset Triangle Traversal state.  
            
            //  MSAA is disabled by default.
            multisampling = false;
            
            //  The default number of MSAA cycles is 2.
            msaaSamples = 2;
            
            //  Precalculate the number of cycles it takes for all the samples in a fragment to be generated.
            msaaCycles = U32(ceil(F32(msaaSamples) / F32(samplesCycle)));
            
            //  Change to READY state.  
            state = RAST_READY;

            break;

        case RAST_READY:
            //  Ready state.  

            GPU_DEBUG_BOX(
                printf("TriangleTraversal => READY state.\n");
            )

            //  Receive and process command from the main Rasterizer mdu.  
            if (traversalCommand->read(cycle, (DynamicObject *&) rastCommand))
                processCommand(rastCommand, cycle);

            break;

        case RAST_DRAWING:

            // If any bmRaster function is performed this variable will change to true to indicate that the rasterizer is actively working
            rasterizationWorking = false; 


            //  Draw state.  

            GPU_DEBUG_BOX(
                printf("TriangleTraversal => DRAWING state.\n");
            )

            //  Update counter for the cycles until the next group of quads can be processed by the MSAA sample generator.
            if (nextMSAAQuadCycles > 0)
            {
                //  Update counter for next MSAA generation round.
                nextMSAAQuadCycles--;
            }
            
            //  Check if the HZ can receive a new set of fragments.  
            if ((hzCurrentState == HZST_READY) && (!traversalFinished) && (nextMSAAQuadCycles == 0))
            {
                {
                    //  Check if is last triangle.  
                    if (!triangleQueue[nextTriangle]->isLast())
                    {
                        //  Update recursive rasterization of the current triangle batch.  
                        if (recursiveMode)
                        {
                            bmRaster.updateRecursiveMultiv2(batchID);
                            rasterizationWorking = true;
                        }
                    }

                    //  Generate fragments.  
                    for(i = 0; (i < stampsCycle) && (!lastFragment); i++)
                    {
                        //  Check if is the last triangle in the batch.  
                        if (triangleQueue[nextTriangle]->isLast())
                        {
                            //  Allocate a stamp.  
                            stamp = new Fragment*[STAMP_FRAGMENTS];

                            //  Generate an empty stamp for the last triangle.  
                            for(j = 0; j < STAMP_FRAGMENTS; j++)
                                stamp[j] = NULL;

                            GPU_DEBUG_BOX(
                                printf("TriangleTraversal => Generating an empty fragment (last triangle).\n");
                            )

                            //  Set triangle identifier to the relative position of the last triangle in the queue.  
                            triangleID = 0;

                            //  Set last fragment flag.  
                            lastFragment = TRUE;

                            //  The last stamp must be sent to all units, so we don't care about the identifier.  
                            stampUnit = 0;
                            stampTileID = TileIdentifier(0, 0);
                        }
                        else
                        {
                            //  Determine rasterization method.  
                            if (recursiveMode)
                            {
                                //  Generate a new stamp.  
                                stamp = bmRaster.nextStampRecursiveMulti(batchID, triangleID);
                            }
                            else
                            {
                                //  Generate a new stamp.  
                                stamp = bmRaster.nextScanlineStampTiled(triangleQueue[nextTriangle]->getSetupTriangleID());
                            }
                            
                            rasterizationWorking = true;

                            GPU_DEBUG_BOX(
                                printf("TriangleTraversal => Generating new stamp.\n");
                            )

                            //  Check if a stamp was received.  
                            if (stamp != NULL)
                            {
                                //  Calculate the tile identifier for the first fragment (should be the same for the whole stamp).  
                                stampTileID = bmRaster.calculateTileID(stamp[0]);

                                //  Calculate the stamp unit assigned to the stamp  (use first fragment in the stamp).  
                                //stampUnit = stampTileID.assignOnX(numStampUnits);
                                //stampUnit = stampTileID.assignMorton(numStampUnits);
                                stampUnit = pixelMapper.mapToUnit(stamp[0]->getX(), stamp[0]->getY());

                                //  Set if last fragment was received.  
                                lastFragment = stamp[STAMP_FRAGMENTS - 1]->isLastFragment();
                            }
                        }

                        //  Check if a stamp was generated.  
                        if (stamp != NULL)
                        {
                            //  Allocate the container for the stamp cookies.  
                            stampCookies = new DynamicObject();

                            //  Generate stamp cookies from triangle cookies.  
                            if (recursiveMode)
                            {
                                //  Copy cookies from parent Setup Triangle.  
                                stampCookies->copyParentCookies(*triangleQueue[GPU_MOD(nextTriangle + triangleID,triangleQSize)]);
                            }
                            else
                            {
                                //  Copy cookies from parent Setup Triangle.  
                                stampCookies->copyParentCookies(*triangleQueue[nextTriangle]);
                            }

                            //  Add a new cookie for the whole stamp.  
                            stampCookies->addCookie();

                            // Update the number of generated fragments.
                            fragmentCounter += STAMP_FRAGMENTS;

                            // Compute stamp coverage and update related stats.
                            U32 count = 0;
                            U32 f;

                            for (f = 0; f < STAMP_FRAGMENTS; f++)
                            {
                                    if (stamp[f] != NULL)
                                            count += (stamp[f]->isInsideTriangle()? 1 : 0);
                            }
                            switch(count)
                            {
                                    case 0: stamps0frag->inc(); break;
                                    case 1: stamps1frag->inc(); break;
                                    case 2: stamps2frag->inc(); break;
                                    case 3: stamps3frag->inc(); break;
                                    case 4: stamps4frag->inc(); break;
                            }
                        }

                        //  Send the fragments in the stamp down the pipeline.  
                        for(j = 0; (j < STAMP_FRAGMENTS) && (stamp != NULL); j++)
                        {
                            //  Compute MSAA samples if multisampling is enabled.
                            if (multisampling)
                            {
                                //  Check it isn't a NULL fragment from the last quad
                                if (stamp[j] != NULL)
                                {
                                    //  Compute the requested depth samples for the fragment.
                                    bmRaster.computeMSAASamples(stamp[j], msaaSamples);
                                }
                            }
                            
                            //  Determine rasterization method.  
                            if (recursiveMode)
                            {
                                //  Create fragment input for the Hierarchical Z cmoMduBase.  
                                frInput = new FragmentInput(
                                    triangleQueue[GPU_MOD(nextTriangle + triangleID, triangleQSize)]->getTriangleID(),
                                    triangleQueue[GPU_MOD(nextTriangle + triangleID, triangleQSize)]->getSetupTriangleID(),
                                    stamp[j],
                                    stampTileID,
                                    stampUnit);

                                //  Set fragment input start cycle.  
                                frInput->setStartCycle(cycle);

                                //  Copy cookies from parent Setup Triangle.  
                                //frInput->copyParentCookies(*triangleQueue[GPU_MOD(nextTriangle + triangleID,triangleQSize)]);
                            }
                            else
                            {
                                //  Create fragment input for the Hierarchical Z cmoMduBase.  
                                frInput = new FragmentInput(
                                    triangleQueue[nextTriangle]->getTriangleID(),
                                    triangleQueue[nextTriangle]->getSetupTriangleID(),
                                    stamp[j],
                                    stampTileID,
                                    stampUnit);

                                //  Set fragment input start cycle.  
                                frInput->setStartCycle(cycle);

                                //  Copy cookies from parent Setup Triangle.  
                                //frInput->copyParentCookies(*triangleQueue[nextTriangle]);
                            }

                            //  Keep a common cookie for the whole stamp.  
                            frInput->copyParentCookies(*stampCookies);

                            //  Add a new cookie for each fragment.  
                            frInput->addCookie();

                            //  Update statistics.  
                            outputs->inc();

                            //  Send fragment to Interpolator.  
                            newFragment->write(cycle, frInput);

                            GPU_DEBUG_BOX(
                                printf("TriangleTraversal => Sending Fragment to Hierarchical Z.\n");
                            )
                        }

                        //  Check if a stamp was generated.  
                        if (stamp != NULL)
                        {
                            //  Delete stamp container.  
                            delete stampCookies;
                        }

                        //  Delete the stamp (array of pointers to fragment).  
                        delete[] stamp;
                    }
                }
            }
//if (lastFragment)
//printf("E");

            //  Check if the last fragment of the triangle was generated.  
            if (!traversalFinished && !triangleQueue[nextTriangle]->isLast() && 
                bmRaster.lastFragment(triangleQueue[nextTriangle]->getSetupTriangleID()))
            {
                //  Set current triangle as finished.  
                traversalFinished = TRUE;

                //  Free the setup triangles in the batch from the rasterizer behaviorModel.  
                for(i = 0; i < batchTriangles; i++)
                {
//printf("T");
                    //  Free the setup triangle from the rasterizer behaviorModel.  
                    bmRaster.destroyTriangle(triangleQueue[GPU_MOD(nextTriangle + i, triangleQSize)]->getSetupTriangleID());

                    //  Delete triangle output objects for the finished batch.  
                    delete triangleQueue[GPU_MOD(nextTriangle + i, triangleQSize)];

                    //  Set current triangle pointer to NULL.  
                    triangleQueue[GPU_MOD(nextTriangle + i, triangleQSize)] = NULL;
                }

                //  Update pointer to the next triangle to process.  
                nextTriangle = GPU_MOD(nextTriangle + batchTriangles, triangleQSize);

                //  Update number of stored triangles.  
                storedTriangles -= batchTriangles;

                //  Update processed triangles counter.  
                triangleCounter += batchTriangles;

                //  Reset last fragment flag.  
                lastFragment = FALSE;
            }
            else if (!traversalFinished && lastFragment)
            {
printf("B");
                //  Set current triangle as finished.  
                traversalFinished = TRUE;

                //  Delete current triangle.  
                delete triangleQueue[nextTriangle];

                //  Set current triangle pointer to NULL.  
                triangleQueue[nextTriangle] = NULL;

                //  Update processed triangles counter.  
                triangleCounter++;

                //  Update number of stored triangles.  
                storedTriangles--;

                //  End of batch, change to END state.  
                state = RAST_END;
            }

            //  Check if start rasterization of a new triangle batch.  
            if (traversalFinished &&
                ((storedTriangles >= triangleBatch) ||
                ((storedTriangles > 0) && (triangleQueue[GPU_PMOD(nextStore - 1, triangleQSize)]->isLast()))))
            {
                //  Set current triangle traversal as not finished.  
                traversalFinished = FALSE;
 
                //  Calculates the number of triangles to batch into the rasterizer.  
                batchTriangles = GPU_MIN(storedTriangles, triangleBatch);
 
                //  Check if the last in the batch is the last triangle.  
                if (triangleQueue[GPU_MOD(nextTriangle + batchTriangles - 1, triangleQSize)]->isLast())
                {
                    //  Leave the last triangle for the end.  
                    batchTriangles--;
                }
 
                if (!triangleQueue[nextTriangle]->isLast()) //  Check if it is the last triangle.  
                {
                    //  Initialize the rasterization of the current batch.  
 
                    //  Determine rasterization method.  
                    if (recursiveMode)
                    {
                        //  Create a list with the triangles in batch.  
                        triangleList = new U32[batchTriangles];
                        for(i = 0; i < batchTriangles; i++)
                            triangleList[i] = triangleQueue[GPU_MOD(nextTriangle + i, triangleQSize)]->getSetupTriangleID();
 
                        //  Start the recursive rasterization of the batch.  
                        batchID = bmRaster.startRecursiveMulti(triangleList, batchTriangles, multisampling);
 
                        //  Delete triangle list.  
                        delete[] triangleList;
                    }
                    else
                    {
                        //  Calculate start position for scanline rasterization.  
                        bmRaster.startPosition(triangleQueue[nextTriangle]->getSetupTriangleID(), multisampling);
                    }
                    
                    rasterizationWorking = true;
                }
                else
                {
                    GPU_DEBUG_BOX(
                        printf("TriangleTraversal => Last triangle (ID %d) received.\n",
                            triangleQueue[nextTriangle]->getTriangleID());
                    )
                }
            }

            //  Receive setup triangles from Triangle Setup.  
            while (setupTriangle->read(cycle, (DynamicObject *&) tsOutput))
            {
                GPU_DEBUG_BOX(
                    printf("TriangleTraversal => Received new setup triangle ID %d from Triangle Setup.\n",
                        tsOutput->getTriangleID());
                )
 
                CG_ASSERT_COND(!(storedTriangles == triangleQSize), "Triangle buffer full."); 
                //  Store as current triangle to traverse.  
                triangleQueue[nextStore] = tsOutput;
 
                //  Update pointer to next store position.  
                nextStore = GPU_MOD(nextStore + 1, triangleQSize);
 
                //  Update number of stored triangles.  
                storedTriangles++;
 
                //  Update requested triangles counter.  
                requestedTriangles--;
 
                //  Update statistics.  
                inputs->inc();
            }

            //  Check if triangles can be requested to setup.  
            if ((storedTriangles + requestedTriangles + trianglesCycle) <= triangleQSize)
            {
                GPU_DEBUG_BOX(
                    printf("TriangleTraversal => Requested triangle to Triangle Setup.\n");
                )
 
                //  Create a request to Triangle Setup.  
                tsRequest = new TriangleSetupRequest(trianglesCycle);
 
                //  Copy cookies from the last DRAW command.  
                tsRequest->copyParentCookies(*lastRSCommand);
 
                //  Add a new cookie.  
                tsRequest->addCookie();
 
                //  Send request to Triangle Setup.  
                setupRequest->write(cycle, tsRequest);
 
                //  Update number of requested triangles.  
                requestedTriangles += trianglesCycle;
 
                //  Update statistics.  
                requests->inc();
            }

            // Update utilization statistic
            if ( rasterizationWorking )
                utilizationCycles->inc();

            break;

        case RAST_END:
            //  Draw end state.  

            GPU_DEBUG_BOX(
                printf("TriangleTraversal => END state.\n");
            )

            //  Wait for END command from the Rasterizer main mdu.  
            if (traversalCommand->read(cycle, (DynamicObject *&) rastCommand))
                processCommand(rastCommand, cycle);

            break;

        default:
            CG_ASSERT("Unsupported state.");
            break;
    }

    //  Send current state to the main Rasterizer mdu.  
    traversalState->write(cycle, new RasterizerStateInfo(state));
}


//  Processes a rasterizer command.  
void TriangleTraversal::processCommand(RasterizerCommand *command, U64 cycle)
{
    U32 samples;

    //  Delete last command.  
    delete lastRSCommand;

    //  Store current command as last received command.  
    lastRSCommand = command;

    //  Process command.  
    switch(command->getCommand())
    {
        case RSCOM_RESET:
            //  Reset command from the Rasterizer main mdu.  

            GPU_DEBUG_BOX(
                printf("TriangleTraversal => RESET command received.\n");
            )

            //  Change to reset state.  
            state = RAST_RESET;

            break;

        case RSCOM_DRAW:
            //  Draw command from the Rasterizer main mdu.  

            GPU_DEBUG_BOX(
                printf("TriangleTraversal => DRAW command received.\n");
            )


            //  Check current state.  
            CG_ASSERT_COND(!(state != RAST_READY), "DRAW command can only be received in READY state.");
            //  Reset number of stored triangles.  
            storedTriangles = 0;

            //  Reset pointer to the next triangle to process.  
            nextTriangle = 0;

            //  Reset pointer to the next store position in the triangle queue.  
            nextStore = 0;

            //  Reset number of requested triangles.  
            requestedTriangles = 0;

            //  Reset triangle counter.  
            triangleCounter = 0;

            //  Reset batch fragment counter.  
            fragmentCounter = 0;

            //  Reset the number of cycles remaining until the next group of quads can be processed by the MSAA sample generator.
            nextMSAAQuadCycles = 0;
            
            //  No triangle to traverse yet.  
            traversalFinished = TRUE;

            //  Reset last fragment flag.  
            lastFragment = FALSE;

            //  Set display in the pixel mapper.
            samples = multisampling ? msaaSamples : 1;
            pixelMapper.setupDisplay(hRes, vRes, STAMP_WIDTH, STAMP_WIDTH, genW, genH, scanW, scanH, overW, overH, samples, 1);

            //  Change to drawing state.  
            state = RAST_DRAWING;

            break;

        case RSCOM_END:
            //  End command received from Rasterizer main mdu.  

            GPU_DEBUG_BOX(
                printf("TriangleTraversal => END command received.\n");
            )


            //  Check current state.  
            CG_ASSERT_COND(!(state != RAST_END), "END command can only be received in END state.");
            //  Change to ready state.  
            state = RAST_READY;

            break;

        case RSCOM_REG_WRITE:
            //  Write register command from the Rasterizer main mdu.  

            GPU_DEBUG_BOX(
                printf("TriangleTraversal => REG_WRITE command received.\n");
            )

            //  Check current state.  
            CG_ASSERT_COND(!(state != RAST_READY), "REGISTER WRITE command can only be received in READY state.");
            //  Process register write.  
            processRegisterWrite(command->getRegister(),
                command->getSubRegister(), command->getRegisterData());

            break;

        default:
            CG_ASSERT("Unsupported command.");
            break;
    }
}

void TriangleTraversal::processRegisterWrite(GPURegister reg, U32 subreg,
    GPURegData data)
{
    //  Process register write.  
    switch(reg)
    {
        case GPU_DISPLAY_X_RES:
            //  Write display horizontal resolution register.  
            hRes = data.uintVal;

            GPU_DEBUG_BOX(
                printf("HierarchicalZ => Write GPU_DISPLAY_X_RES = %d.\n", hRes);
            )

            break;

        case GPU_DISPLAY_Y_RES:
            //  Write display vertical resolution register.  
            vRes = data.uintVal;

            GPU_DEBUG_BOX(
                printf("HierarchicalZ => Write GPU_DISPLAY_Y_RES = %d.\n", vRes);
            )

            break;

        case GPU_VIEWPORT_INI_X:
            //  Write viewport initial x coordinate register.  
            startX = data.intVal;

            GPU_DEBUG_BOX(
                printf("HierarchicalZ => Write GPU_VIEWPORT_INI_X = %d.\n", startX);
            )

            break;

        case GPU_VIEWPORT_INI_Y:
            //  Write viewport initial y coordinate register.  
            startY = data.intVal;

            GPU_DEBUG_BOX(
                printf("HierarchicalZ => Write GPU_VIEWPORT_INI_Y = %d.\n", startY);
            )

            break;

        case GPU_VIEWPORT_WIDTH:
            //  Write viewport width register.  
            width = data.uintVal;

            GPU_DEBUG_BOX(
                printf("HierarchicalZ => Write GPU_VIEWPORT_WIDTH = %d.\n", width);
            )

            break;

        case GPU_VIEWPORT_HEIGHT:
            //  Write viewport height register.  
            height = data.uintVal;

            GPU_DEBUG_BOX(
                printf("HierarchicalZ => Write GPU_VIEWPORT_HEIGHT = %d.\n", height);
            )

            break;
    
        case GPU_MULTISAMPLING:
        
            //  Write Multisampling enable flag.
            multisampling = data.booleanVal;
            
            GPU_DEBUG_BOX(
                printf("TriangleTraversal => Write GPU_MULTISAMPLING = %s\n", multisampling?"TRUE":"FALSE");
            )
                       
            break;
            
        case GPU_MSAA_SAMPLES:
        
            //  Write MSAA z samples per fragment register.
            msaaSamples = data.uintVal;
            
            //  Precalculate the number of cycles it takes for all the samples in a fragment to be generated.
            msaaCycles = U32(ceil(F32(msaaSamples) / F32(samplesCycle)));

            GPU_DEBUG_BOX(
                printf("TriangleTraversal => Write GPU_MSAA_SAMPLES = %d\n", msaaSamples);
            )

            break;            

        default:
            CG_ASSERT("Unsupported register.");
            break;
    }

}

void TriangleTraversal::getState(string &stateString)
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
        default:
            stateStream << "undefined";
            break;
    }

    stateStream << " | Stored Triangles = " << storedTriangles;
    stateStream << " | Requested Triangles = " << requestedTriangles;
    stateStream << " | Triangle Counter = " << triangleCounter;
    stateStream << " | Traversal Finished = " << traversalFinished;
    stateStream << " | Fragment Counter = " << fragmentCounter;

    stateString.assign(stateStream.str());
}
