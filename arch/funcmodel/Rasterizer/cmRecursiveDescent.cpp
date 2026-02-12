/**************************************************************************
 *
 * Recursive Descent mdu implementation file.
 *
 */

 /**
 *
 *  @file RecursiveDescent.cpp
 *
 *  This file implements the Recursive Descent mdu.
 *
 *  This class implements the Recursive used for recursive descent
 *  rasterization in a GPU.
 *
 */

#include "cmRecursiveDescent.h"
#include "cmRasterizerStateInfo.h"
#include "cmTriangleSetupRequest.h"
#include "InterpolatorStateInfo.h"
#include "cmTileEvaluatorStateInfo.h"
#include "Tile.h"

using namespace arch;

//  Recursive Descent constructor.  
RecursiveDescent::RecursiveDescent(bmoRasterizer &bm, U32 trFIFOSize,
    U32 tileEvaluators, U32 tilesCycleEvaluator, U32 stackSize,
    U32 stampFr, U32 staCycle, U32 frBufferSize, char **tileEvalPref,
    char *name, cmoMduBase *parent) :

    bmRast(bm), triangleFIFOSize(trFIFOSize),
    numTileEvaluators(tileEvaluators), tilesCycle(tilesCycleEvaluator),
    tileStackSize(stackSize), stampFragments(stampFr), stampsCycle(staCycle),
    fragmentBufferSize(frBufferSize), tileEvalPrefixes(tileEvalPref),
    cmoMduBase(name, parent)

{
    int i, j;
    DynamicObject *defaultState[1];

    //  Create mdu signals.  

    //  Create signals to the rasterizer main mdu.  

    //  Create command signal from the rasterizer main mdu.  
    rastCommand = newInputSignal("RecursiveDescentCommand", 1, 1, NULL);

    //  Create state signal to the rasterizer main mdu.  
    rastState = newOutputSignal("RecursiveDescentState", 1, 1, NULL);

    //  Create default signal value.  
    defaultState[0] = new RasterizerStateInfo(RAST_RESET);

    //  Set default signal value.  
    rastState->setData(defaultState);

    //  Create signals to Triangle Setup.  

    //  Create setup triangle input from the Triangle Setup mdu.  
    newTriangle = newInputSignal("TriangleSetupOutput", 1, 2, NULL);

    //  Create request signal to Triangle Setup.  
    triangleRequest = newOutputSignal("TriangleSetupRequest", 1, 1, NULL);

    //  Allocate the signals for the tile evaluators.  

    //  Allocate input signals to the Tile Evaluators.  
    evaluateTile = new (Signal *)[numTileEvaluators];

    //  Check allocation.  
    CG_ASSERT_COND(!(evaluateTile == NULL), "Error allocating input signals to the Tile Evaluators.");
    //  Allocate new tiles signals from the Tile Evaluators.  
    newTiles = new (Signal *)[numTileEvaluators];

    //  Check Allocation.  
    CG_ASSERT_COND(!(newTiles == NULL), "Error allocating new tile signal from the Tile Evaluators.");
    //  Allocate tile evaluator state signals.  
    evaluatorState = new (Signal *)[numTileEvaluators];

    //  Check allocation.  
    CG_ASSERT_COND(!(evaluatorState == NULL), "Error allocating tile evaluator state signals.");
    //  Allocate new fragment signal from the Tile Evaluators.  
    newFragment = new (Signal *)[numTileEvaluators];

    //  Check allocation.  
    CG_ASSERT_COND(!(newFragment == NULL), "Error allocating tile evaluators new fragment signals.");
    //  Create signals to the Tile Evaluators.  
    for(i = 0; i < numTileEvaluators; i++)
    {
        //  Create tile input signal to a Tile Evaluator.  
        evaluateTile[i] = newOutputSignal("TileEvaluator", 1, 1, tileEvalPrefixes[i]);

        //  Create new tile signal from a Tile Evaluator.  
        newTiles[i] = newInputSignal("NewTile", 1, 1, tileEvalPrefixes[i]);

        //  Create state signal from a Tile Evaluator.  
        evaluatorState[i] = newInputSignal("TileEvaluatorState", 1, 1, tileEvalPrefixes[i]);

        //  Create new fragment signal from a Tile Evaluator.  
        newFragment[i] = newInputSignal("NewFragment", stampFragments, 1, tileEvalPrefixes[i]);
    }


    //  Create signals with the Interpolator unit.  

    //  Create fragment output signal to the interpolator.  
    interpolator = newOutputSignal("InterpolatorNewFragment", stampFragments * stampsCycle, 1, NULL);

    //  Create state signal from the Interpolator.  
    interpolatorState = newInputSignal("InterpolatorState", 1, 1, NULL);


    //  Allocate memory for the Setup Triangle Queue.  
    triangleFIFO = new (TriangleSetupOutput *)[triangleFIFOSize];

    //  Check allocation.  
    CG_ASSERT_COND(!(triangleFIFO == NULL), "Error allocating setup triangle FIFO.\n");
    //  Allocate memory for the Tile Stack.  
    tileStack = new (TileInput **)[numTileEvaluators];

    //  Check allocation.  
    CG_ASSERT_COND(!(tileStack == NULL), "Error allocating Tile stack.");
    //  Allocate memory for each Tile Evaluator Tile Stack.  
    for(i = 0; i < numTileEvaluators; i++)
    {
        //  Allocate the Tile Evaluator Tile Stack.  
        tileStack[i] = new (TileInput *)[tileStackSize];

        //  Check allocation.  
        CG_ASSERT_COND(!(tileStack[i] == NULL), "Error allocating Tile Stack for Tile Evaluator.");    }

    //  Allocate next tile stack pointers.  
    nextTile = new U32[numTileEvaluators];

    //  Check allocation.  
    CG_ASSERT_COND(!(nextTile == NULL), "Error allocating next tile in the stack pointers.");
    //  Allocate stores tiles counters.  
    storedTiles = new U32[numTileEvaluators];

    //  Check allocation.  
    CG_ASSERT_COND(!(storedTiles == NULL), "Error allocating stored tiles counters.");
    //  Allocate reserved tiles counters.  
    reservedTiles = new U32[numTileEvaluators];

    //  Check allocation.  
    CG_ASSERT_COND(!(reservedTiles == NULL), "Error allocating reserved tiles counters.");
    //  Allocate memory for the fragment buffer.  
    fragmentBuffer = new (FragmentInput *)[fragmentBufferSize];

    //  Check memory allocation.  
    CG_ASSERT_COND(!(fragmentBuffer == NULL), "Error allocating fragment buffer.");
    //  Allocate memory for the tile evaluators current state.  
    tileEvalStates = new TileEvaluatorState[numTileEvaluators];

    //  Check memory allocation.  
    CG_ASSERT_COND(!(tileEvalStates == NULL), "Error allocating Tile Evaluator current state array.");
    //  Set initial state to reset.  
    state = RAST_RESET;

    //  Create dummy first last rasterizer command.  
    lastRSCommand = new RasterizerCommand(RSCOM_RESET);
}



//  Recursive Descent simulation rutine.  
void RecursiveDescent::clock(U64 cycle)
{
    InterpolatorStateInfo *intStateInfo;
    InterpolatorState intState;
    TriangleSetupOutput *tsOutput;
    TriangleSetupRequest *tsRequest;
    TileEvaluatorStateInfo *teStateInfo;
    RasterizerCommand *rsCommand;
    TileInput *tileInput;
    FragmentInput *frInput;
    Tile *tile;
    U32 receivedTiles;
    U32 receivedFragments;
    bool allReady;
    bool sendTile;
    int i, j;

    GPU_DEBUG(
        printf("RecursiveDescent => clock %lld.\n", cycle);
    )

    //  Receive state from the Tile Evaluators.  
    for (i = 0; i < numTileEvaluators; i++)
    {
        //  Receive state signal from a Tile Evaluator.  
        if (evaluatorState[i]->read(cycle, (DynamicObject *&) teStateInfo))
        {
            //  Get interpolator state.  
            tileEvalStates[i] = teStateInfo->getState();

            GPU_DEBUG(
                if (tileEvalStates[i] == TE_READY)
                    printf("RecursiveDescent => TileEvaluator %d READY.\n", i);
                else
                    printf("RecursiveDescent => TileEvaluator %d NOT READY.\n", i);
            )

            //  Delete tile evaluator state info object.  
            delete teStateInfo;
        }
        else
        {
            CG_ASSERT("Missing state signal from a Tile Evaluator.");
        }
    }

    //  Receive interpolator state.  
    if (interpolatorState->read(cycle, (DynamicObject *&) intStateInfo))
    {
        //  Get interpolator state.  
        intState = intStateInfo->getState();

        //  Delete interpolator state info object.  
        delete intStateInfo;
    }
    else
    {
        CG_ASSERT("Missing state signal from the Interpolator.");
    }

    //  Simulate current cycle.  
    switch(state)
    {
        case RAST_RESET:
            //  Reset state.  

            GPU_DEBUG(
                printf("RecursiveDescent => RESET state.\n");
            )

            //  Reset Recursive Descent state.  

            //  Change to READY state.  
            state = RAST_READY;

            break;

        case RAST_READY:
            //  Ready state.  

            GPU_DEBUG(
                printf("RecursiveDescent => READY state.\n");
            )

            //  Receive and process command from the main Rasterizer mdu.  
            if (rastCommand->read(cycle, (DynamicObject *&) rsCommand))
                processCommand(rsCommand, cycle);

            break;

        case RAST_DRAWING:
            //  Draw state.  

            GPU_DEBUG(
                printf("RecursiveDescent => DRAWING state.\n");
            )

            /*  Send last tile and last fragment signals after all the
                triangles and tiles have been processed.  */
            if (lastTriangle && (storedFragments == 0) && (totalReserved == 0) &&
                (totalStored == 0))
            {
                //  Check that all the tile evaluators are ready.  
                GPU_ASSERT(
                    //  Check all the Tile Evaluators are ready.  
                     for(i = 0, allReady = TRUE; i < numTileEvaluators; i++)
                        allReady = allReady && (tileEvalStates[i] == TE_READY) &&
                            (storedTiles[i] == 0) && (reservedTiles[i] == 0);

                    if (!allReady)
                        CG_ASSERT("Tile Evaluators not ready for Last Tile signal.");
                )

                //  Create and send last tile signal to all Tile Evaluators.  
                for (i = 0; i < numTileEvaluators; i++)
                {
                    //  Create tile input for last tile (NULL).  
                    tileInput = new TileInput(0, 0, NULL, FALSE, TRUE);

                    //  Copy cookies from the last DRAW command.  
                    tileInput->copyParentCookies(*lastRSCommand);

                    //  Add a new cookie.  
                    tileInput->addCookie();

                    GPU_DEBUG(
                        printf("RecursiveDescent => Last tile sent to Tile Evaluator %d\n",
                            i);
                    )

                    //  Send tile to the tile evaluator.  
                    evaluateTile[i]->write(cycle, tileInput);
                }

                //  Send last fragment signal to Interpolator.  
                for(i = 0; i < (stampsCycle * stampFragments); i++)
                    interpolator->write(cycle,  new FragmentInput(0, 0, NULL));

                //  Change state to rasterization end.  
                state = RAST_END;
            }


            //  Send tiles to Tile Evaluators.  

            /*  Try to send a tile to each Tile Evaluator if there are
                tiles and the Tile Evalutor is ready.  */
            for(i = 0; i < numTileEvaluators; i++)
            {
                /*  Low level tiles have priority over top level
                    tiles.  */

                GPU_DEBUG(
                    printf("RecursiveDescent => Tiles in stack %d = %d\n",
                        i, storedTiles[i]);
                    printf("RecursiveDescent => Reserved tiles in stack %d = %d\n",
                        i, reservedTiles[i]);
                )

                //  Check tile state.  
                if ((storedTiles[i] > 0) &&(tileEvalStates[i] == TE_READY))
                {
                    //  Get last tile from the Tile Stack.  
                    tileInput = tileStack[i][nextTile[i]];

                    /*  Reserve space in the queue/buffers for new
                        tiles/fragments.  */

                    //  Set send tile to Evaluator to true.  
                    sendTile = TRUE;

                    //  Check if it is a stamp level tile.  
                    if (tileInput->getTile()->getLevel() == STAMPLEVEL)
                    {
                        //  Reserve space in the fragment file.  
                        if ((storedFragments + stampFragments) >=
                            fragmentBufferSize)
                        {
                            /*  No space left.  Do not send the tile to the
                                tile evaluator.  */
                            sendTile = FALSE;
                        }
                        else
                        {
                            //  Reserve space for the new fragments.  
                            storedFragments += stampFragments;
                        }
                    }
                    else
                    {
                        //  Reserve space in the tile queue.  
                        if ((storedTiles[i] + reservedTiles[i] + MAXGENERATEDTILES) >
                            tileStackSize)
                        {
                            //  Not enough space.  Do not send the tile.  
                            sendTile = FALSE;
                        }
                        else
                        {
                            //  Reserve space for the new tiles.  
                            reservedTiles[i] += MAXGENERATEDTILES;

                            //  Update total reserved entries counter.  
                            totalReserved += MAXGENERATEDTILES;
                        }
                    }

                    //  Check if the tile can be sent to the Evaluator.  
                    if (sendTile)
                    {
                        GPU_DEBUG(
                            printf("RecursiveDescent => Tile at (%d, %d) level %d sent to Tile Evaluator %d\n",
                                tileInput->getTile()->getX(), tileInput->getTile()->getY(),
                                tileInput->getTile()->getLevel(), i);
                        )

                        //  Send tile to the tile evaluator.  
                        evaluateTile[i]->write(cycle, tileInput);

                        //  Update last tile pointer.  
                        nextTile[i]--;

                        //  Update stored tiles counter.  
                        storedTiles[i]--;

                        //  Update total stored tiles counter.  
                        totalStored--;
                    }
                }
            }

            GPU_DEBUG(
                printf("RecursiveDescent => Fragments in buffer %d\n",
                    storedFragments);
            )

            //  Send fragments to the Interpolator.  
            if ((storedFragments > 0) && (intState == INT_READY))
            {
                /*  Send a stamp to the interpolator if it has been
                    already received from the tile evaluators.  */
                for(i = 0; (i < stampsCycle) &&
                    (fragmentBuffer[firstFragment] != NULL); i++)
                {
                    //  Send fragment stamp to the interpolator.  
                    for(j = 0; j < stampFragments; j++)
                    {
                        //  Get next fragment from the fragment buffer.  
                        frInput = fragmentBuffer[firstFragment];

                        //  Check that the next stamp fragment is ready.  
                        CG_ASSERT_COND(!(frInput == NULL), "Missing fragment from stamp.");
                        GPU_DEBUG(
                            printf("RecursiveDescent => Sending Fragment from triangle ID %d to Interpolator.\n",
                                frInput->getTriangleID());
                        )

                        //  Send the fragment to the intepolator.  
                        interpolator->write(cycle, frInput);

                        //  Clean fragment buffer entry.  
                        fragmentBuffer[firstFragment] = NULL;

                        //  Update pointer to the next fragment.  
                        firstFragment = GPU_MOD(firstFragment + 1, fragmentBufferSize);

                    }

                    //  Update stored fragments counter.  
                    storedFragments -= stampFragments * stampsCycle;
                }
            }

            //  Receive new tiles from the Tile Evaluators.  
            for(i = 0; i < numTileEvaluators; i++)
            {
                //  Receive a tile from a Tile Evaluator.  
                for(receivedTiles = 0;
                    (newTiles[i]->read(cycle, (DynamicObject *&) tileInput));
                    receivedTiles++)
                {
                    GPU_DEBUG(
                        if (tileInput->getTile() != NULL)
                            printf("RecursiveDescent => Received Tile at (%d, %d) level %d from Tile Evaluator %d\n",
                                tileInput->getTile()->getX(),
                                tileInput->getTile()->getY(),
                                tileInput->getTile()->getLevel(), i);
                        else
                            printf("RecursiveDescent => Received Empty Tile from Tile Evaluator %d.\n",
                                i);
                    )

                    //  Check if received an empty tile.  
                    if (tileInput->getTile() != NULL)
                    {
                        //  Check if there is space in the tile stack.  
                        CG_ASSERT_COND(!(storedTiles[i] >= tileStackSize), "No available space at the current tile level.");
                        /*  Perform some load balancing between the
                            tile evaluators.  If any of the Tile Evaluators
                            has no work write the received tile into its
                            tile stack.  */

                        //  Search for a Tile Evaluator without work.  
                        for(j = 0; (storedTiles[j] > 0) &&
                            (reservedTiles[j] > 0) && (j < numTileEvaluators);
                            j++);

                        //  Check if there is Tile Evaluator without work.  
                        if ((j < numTileEvaluators) && (storedTiles[j] == 0)
                            && (reservedTiles[j] == 0))
                        {
                            /*  Store the received tile at the free Tile
                                Evaluator stack.  */

                            //  Update last tile pointer.  
                            nextTile[j]++;

                            //  Store tile at the queue.  
                            tileStack[j][nextTile[j]] = tileInput;

                            //  Update stored tiles for current level counter.  
                            storedTiles[j]++;
                        }
                        else
                        {
                            /*  Store the received tile at the source Tile
                                Evaluator stack.  */

                            //  Update last tile pointer.  
                            nextTile[i]++;

                            //  Store tile at the queue.  
                            tileStack[i][nextTile[i]] = tileInput;

                            //  Update stored tiles for current level counter.  
                            storedTiles[i]++;
                        }

                        //  Update total stored tiles.  
                        totalStored++;
                    }
                    else
                    {
                        //  Delete the empty tile.  
                        delete tileInput;
                    }

                    /*  Check if it is the last tile from the input parent
                        tile.  */
                    if (tileInput->isEndTile())
                    {
                        //  Update reserved tiles counter.  
                        reservedTiles[i] -= MAXGENERATEDTILES;

                        //  Update total reserved tile entries counter.  
                        totalReserved -= MAXGENERATEDTILES;
                    }
                }

                /*  Check if number of tiles per cycle and Tile Evaluator was
                    exceded.  */
                CG_ASSERT_COND(!(receivedTiles > MAXGENERATEDTILES), "Received more tiles from the Tile Evaluator than expected.");
            }


            //  Receive fragments from the Tile Evaluators.  
            for(i = 0; i < numTileEvaluators; i++)
            {
                //  Receive fragments from a Tile Evaluator.  
                for(receivedFragments = 0;
                    newFragment[i]->read(cycle, (DynamicObject *&) frInput);
                    receivedFragments++)
                {
                    //  Add fragment to the fragment buffer.  
                    fragmentBuffer[nextFreeFBEntry] = frInput;

                    //  Update pointer to the next free framebuffer entry.  
                    nextFreeFBEntry = GPU_MOD(nextFreeFBEntry + 1, fragmentBufferSize);

                    //  STORED FRAGMENT COUNTER ALREADY UPDATED AT RESERVE.  
               }

                /*  Check if number of fragments per cycle and tile
                    evaluator was exceded.  */
                CG_ASSERT_COND(!(receivedFragments > stampFragments), "Received more fragments than expected from the Tile Evaluators.");            }

            //  Add new top level tiles.  

            /*  Refill the tile evaluator tile stacks with top level tiles
                generated from the stored setup triangles.  */
            for(i = 0; i < numTileEvaluators; i++)
            {
                if ((storedTiles[i] == 0) && (reservedTiles[i] < tileStackSize) &&
                    (storedTriangles > 0))
                {
                    //  Get next triangle from the triangle FIFO.  
                    tsOutput = triangleFIFO[firstTriangle];

                    //  Check if it is the last triangle.  
                    if (!tsOutput->isLast())
                    {
                        //  Create new start tile for the triangle.  
                        tile = bmRast.topLevelTile(tsOutput->getSetupTriangleID());

                        //  Create tile input container for the tile.  
                        tileInput = new TileInput(tsOutput->getTriangleID(),
                            tsOutput->getSetupTriangleID(), tile, FALSE, FALSE);

                        //  Copy cookies from the last DRAW command.  
                        tileInput->copyParentCookies(*lastRSCommand);

                        //  Add a new cookie.  
                        tileInput->addCookie();

                        //  Update last tile pointer.  
                        nextTile[i]++;

                        //  Add tile to the tile stack.  
                        tileStack[i][nextTile[i]] = tileInput;

                        //  Update stored tiles for current level counter.  
                        storedTiles[i]++;

                        //  Update total stored tiles counter.  
                        totalStored++;

                        //  Ask to delete the setup triangle in the rasterizer behaviorModel.  
                        bmRast.destroyTriangle(tsOutput->getSetupTriangleID());
                    }

                    //  Update pointer to the first triangle.  
                    firstTriangle = GPU_MOD(firstTriangle + 1, triangleFIFOSize);

                    //  Update stored triangles counter.  
                    storedTriangles--;

                    //  Delete Triangle Setup Output object.  
                    delete tsOutput;
                }
            }

            //  Receive setup triangles from Triangle Setup.  
            if (newTriangle->read(cycle, (DynamicObject *&) tsOutput))
            {
                GPU_DEBUG(
                    printf("RecursiveDescent => Received new setup triangle ID %d from Triangle Setup.\n",
                        tsOutput->getTriangleID());
                )

                //  Check there is enough space in the setup triangle FIFO.  
                CG_ASSERT_COND(!(storedTriangles >= triangleFIFOSize), "Not enough space available in the Triangle FIFO.");
                //  Check last triangle hasn't been already received.  
                CG_ASSERT_COND(!(lastTriangle), "Triangle received after last triangle mark.");
                //  Store setup triangle in the triangle FIFO.  
                triangleFIFO[nextFreeSTEntry] = tsOutput;

                //  Check if it is the last triangle.  
                if (tsOutput->isLast())
                {
                    //  Last triangle received.  
                    lastTriangle = TRUE;
                }

                //  Update stored setup triangles counter.  
                storedTriangles++;

                //  Update pointer to the next free entry in the triangle FIFO.  
                nextFreeSTEntry = GPU_MOD(nextFreeSTEntry + 1, triangleFIFOSize);

                //  Update received triangles counter.  
                triangleCounter++;
            }


            //  Request new triangles to Triangle Setup.  

            /*  Check if there is space in the setup triangle FIFO and
                request a new triangle to Triangle Setup.  */
            if (!lastTriangle && (storedTriangles < (triangleFIFOSize - 2)))
            {
                GPU_DEBUG(
                    printf("RecursiveDescent => Requested triangle to Triangle Setup.\n");
                )

                //  Create a request to Triangle Setup.  
                tsRequest = new TriangleSetupRequest();

                //  Copy cookies from the last DRAW command.  
                tsRequest->copyParentCookies(*lastRSCommand);

                //  Add a new cookie.  
                tsRequest->addCookie();

                //  Send request to Triangle Setup.  
                triangleRequest->write(cycle, tsRequest);
            }

            break;

        case RAST_END:
            //  Draw end state.  

            GPU_DEBUG(
                printf("RecursiveDescent => END state.\n");
            )

            //  Wait for END command from the Rasterizer main mdu.  
            if (rastCommand->read(cycle, (DynamicObject *&) rsCommand))
                processCommand(rsCommand, cycle);

            break;

        default:
            CG_ASSERT("Unsupported state.");
            break;
    }

    //  Send current state to the main Rasterizer mdu.  
    rastState->write(cycle, new RasterizerStateInfo(state));
}


//  Processes a rasterizer command.  
void RecursiveDescent::processCommand(RasterizerCommand *command, U64 cycle)
{
    TriangleSetupRequest *tsRequest;
    int i, j;

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
                printf("RecursiveDescent => RESET command received.\n");
            )

            //  Change to reset state.  
            state = RAST_RESET;

            break;

        case RSCOM_DRAW:
            //  Draw command from the Rasterizer main mdu.  

            GPU_DEBUG(
                printf("RecursiveDescent => DRAW command received.\n");
            )


            //  Check current state.  
            CG_ASSERT_COND(!(state != RAST_READY), "DRAW command can only be received in READY state.");
            //  Reset triangle counter.  
            triangleCounter = 0;

            //  Last batch triangle has not been received yet.  
            lastTriangle = FALSE;

            //  Initialize setup triangle queue entries.  
            for(i = 0; i < triangleFIFOSize; i++)
                triangleFIFO[i] = NULL;

            //  Reset stored triangles counter.  
            storedTriangles = 0;

            //  Reset pointer to the first triangle in the FIFO.  
            firstTriangle = 0;

            //  Reset pointer to the next free entry in the triangle FIFO.  
            nextFreeSTEntry = 0;

            //  Initialize tile stack.  
            for(i = 0; i < numTileEvaluators; i++)
            {
                for(j = 0; j < tileStackSize; j++)
                    tileStack[i][j] = NULL;

                //  Reset stored tiles counter.  
                storedTiles[i] = 0;

                //  Reset reserved tiles counter.  
                reservedTiles[i] = 0;

                //  Reset pointer to the next (last) tile in the stack.  
                nextTile[i] = 0;
            }

            //  Initialize fragment buffer.  
            for(i = 0; i < fragmentBufferSize; i++)
                fragmentBuffer[i] = NULL;

            //  Reset pointer to first fragment in the buffer.  
            firstFragment = 0;

            //  Reset pointer to next free entry in the buffer.  
            nextFreeFBEntry = 0;

            //  Reset fragments in the buffer counter.  
            storedFragments = 0;

            //  Reset total stored tiles counter.  
            totalStored = 0;

            //  Reset total reserved tile entries counter.  
            totalReserved = 0;

            //  Create a request to Triangle Setup.  
            tsRequest = new TriangleSetupRequest();

            //  Copy cookies from the last DRAW command.  
            tsRequest->copyParentCookies(*lastRSCommand);

            //  Add a new cookie.  
            tsRequest->addCookie();

            //  Send request to Triangle Setup.  
            triangleRequest->write(cycle, tsRequest);

            //  Change to drawing state.  
            state = RAST_DRAWING;

            break;

        case RSCOM_END:
            //  End command received from Rasterizer main mdu.  

            GPU_DEBUG(
                printf("RecursiveDescent => END command received.\n");
            )


            //  Check current state.  
            CG_ASSERT_COND(!(state != RAST_END), "END command can only be received in END state.");
            //  Change to ready state.  
            state = RAST_READY;

            break;

        case RSCOM_REG_WRITE:
            //  Write register command from the Rasterizer main mdu.  

            GPU_DEBUG(
                printf("RecursiveDescent => REG_WRITE command received.\n");
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

//  Process register write.  
void RecursiveDescent::processRegisterWrite(GPURegister reg, U32 subreg,
    GPURegData data)
{
    //  Process register write.  
    switch(reg)
    {
        default:
            CG_ASSERT("Unsupported register.");
            break;
    }

}
