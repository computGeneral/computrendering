/**************************************************************************
 *
 * Tile Evaluator mdu implementation file.
 *
 */

 /**
 *
 *  @file TileEvaluator.cpp
 *
 *  This file implements the Tile Evaluator mdu.
 *
 *  This class implements the Tile Evaluator used for recursive descent
 *  rasterization in a GPU.
 *
 */

#include "cmTileEvaluator.h"
#include "cmTileEvaluatorStateInfo.h"
#include "cmTileInput.h"
#include "cmRasterizerStateInfo.h"
#include "cmFragmentInput.h"
#include "Tile.h"
#include "cmRecursiveDescent.h"

using namespace cg1gpu;
using namespace std;

//  Tile Evaluator constructor.  
TileEvaluator::TileEvaluator(bmoRasterizer &bm, U32 tilesPerCycle,
    U32 fragmentsPerStamp, U32 inputBufferSz,
    U32 outputBufferSz, U32 evalLatency,
    char *tePrefix,  char *name, cmoMduBase *parent) :

    bmRast(bm), tilesCycle(tilesPerCycle), stampFragments(fragmentsPerStamp),
    inputBufferSize(inputBufferSz), outputBufferSize(outputBufferSz),
    evaluationLatency(evalLatency), prefix(tePrefix), cmoMduBase(name, parent)

{
    DynamicObject *defaultState[1];

    //  Create mdu signals.  

    //  Create signals to the rasterizer main mdu.  

    //  Create command signal from the rasterizer main mdu.  
    rastCommand = newInputSignal("TileEvaluatorCommand", 1, 1, prefix);

    //  Create state signal to the rasterizer main mdu.  
    rastState = newOutputSignal("TileEvaluatorRastState", 1, 1, prefix);

    //  Create default signal value.  
    defaultState[0] = new RasterizerStateInfo(RAST_RESET);

    //  Set default signal value.  
    rastState->setData(defaultState);


    //  Create evaluation latency internal signal.  

    //  Create tile evaluation signal to the Tile Evaluator.  
    evalTileStart = newOutputSignal("EvaluateTile", 1, evaluationLatency, prefix);

    //  Create tile evaluation signal from the Tile Evaluator.  
    evalTileEnd = newInputSignal("EvaluateTile", 1, evaluationLatency, prefix);


    //  Create signals to Recursive Descent.  

    //  Create new tile signal to Recursive Descent.  
    newTiles = newOutputSignal("NewTile", 1, 1, prefix);

    //  Create input tile signal from Recursive Descent.  
    inputTileSignal = newInputSignal("TileEvaluator", 1, 1, prefix);

    //  Create state signal to Recursive Descent.  
    evaluatorState = newOutputSignal("TileEvaluatorState", 1, 1, prefix);

    //  Create default signal value.  
    defaultState[0] = new TileEvaluatorStateInfo(TE_READY);

    //  Set default signal value.  
    evaluatorState->setData(defaultState);


    //  Create fragment signal to Recursive Descent.  
    newFragments = newOutputSignal("NewFragment", stampFragments, 1, prefix);


    //  Create signals with the Hierarchical Z Buffer.  


    //  Allocate memory for the input tile buffer.  
    inputBuffer = new TileInput*[inputBufferSize];

    //  Check allocation.  
    CG_ASSERT_COND(!(inputBuffer == NULL), "Error allocating the input tile buffer.");

    //  Allocate memory for the output tile buffer.  
    outputBuffer = new TileInput*[outputBufferSize];

    //  Check allocation.  
    CG_ASSERT_COND(!(outputBuffer == NULL), "Error allocating the output tile buffer.");
    //  Set initial state to reset.  
    state = RAST_RESET;

    //  Create dummy first last rasterizer command.  
    lastRSCommand = new RasterizerCommand(RSCOM_RESET);
}



//  Tile Evaluator simulation rutine.  
void TileEvaluator::clock(U64 cycle)
{
    TileEvaluatorStateInfo *teStateInfo;
    RasterizerCommand *rastCom;
    TileInput *tileInput, *newTileInput;
    FragmentInput *frInput;
    Tile *tile;
    Tile *subTiles[MAXGENERATEDTILES];
    Fragment *fr;
    Fragment **stamp;
    U32 generatedTiles;
    U32 currentLevel;
    U32 i;

    GPU_DEBUG(
        printf("TileEvaluator-%s => clock %lld.\n", prefix, cycle);
    )

    //  Simulate current cycle.  
    switch(state)
    {
        case RAST_RESET:
            //  Reset state.  

            GPU_DEBUG(
                printf("TileEvaluator-%s => RESET state.\n", prefix);
            )

            //  Reset Tile FIFO state.  

            //  Change to READY state.  
            state = RAST_READY;

            break;

        case RAST_READY:
            //  Ready state.  

            GPU_DEBUG(
                printf("TileEvaluator-%s => READY state.\n", prefix);
            )

            //  Receive and process command from the main Rasterizer mdu.  
            if (rastCommand->read(cycle, (DynamicObject *&) rastCom))
                processCommand(rastCom, cycle);

            break;

        case RAST_DRAWING:
            //  Draw state.  

            GPU_DEBUG(
                printf("TileEvaluator-%s => DRAWING state.\n", prefix);
            )

            //  Start tile evaluation.  
            if (inputTiles > 0)
            {

                //  Get first tile in the input tile buffer.  
                tileInput = inputBuffer[firstInput];

                /*  Reserve output buffer entries if it is a non stamp
                    level tile.  */
                if (tileInput->isLast())
                {
                    //  Reserve space in the output buffer for the last tile.  
                    if ((outputTiles + reservedOutputs) < outputBufferSize)
                    {
                        GPU_DEBUG(
                            printf("TileEvaluator-%s => Starting Last Tile Evaluation.\n",
                                prefix);
                        )

                        //  Start evaluation of the first tile in the input buffer.  
                        evalTileStart->write(cycle, tileInput);

                        //  Update reserved output tile buffer entries counter.  
                        reservedOutputs++;

                        //  Update tiles in input tile buffer counter.  
                        inputTiles--;

                        //  Update pointer to the first tile in the input tile buffer.  
                        firstInput = GPU_MOD(firstInput + 1, inputBufferSize);
                    }
                }
                else if ((tileInput->getTile())->getLevel() == STAMPLEVEL)
                {

                    GPU_DEBUG(
                            printf("TileEvaluator-%s => Starting Stamp Level Tile Evaluation - start point at (%d, %d) level %d.\n",
                                prefix, tileInput->getTile()->getX(),
                                tileInput->getTile()->getY(),
                                tileInput->getTile()->getLevel());
                    )

                    //  Start evaluation of the first tile in the input buffer.  
                    evalTileStart->write(cycle, tileInput);

                    //  Update pointer to the first tile in the input tile buffer.  
                    firstInput = GPU_MOD(firstInput + 1, inputBufferSize);

                    //  Update stored tiles in the tile buffer.  
                    inputTiles--;
                }
                else
                {
                    /*  Reserve space in the output tile buffer before starting
                        the evaluation of the tile.  */
                    if ((outputBufferSize - (outputTiles + reservedOutputs)) >
                        MAXGENERATEDTILES)
                    {
                        GPU_DEBUG(
                            printf("TileEvaluator-%s => Starting Tile Evaluation - start point at (%d, %d) level %d.\n",
                                prefix, tileInput->getTile()->getX(),
                                tileInput->getTile()->getY(),
                                tileInput->getTile()->getLevel());
                        )

                        //  Start evaluation of the first tile in the input buffer.  
                        evalTileStart->write(cycle, tileInput);

                        //  Update pointer to the first tile in the input tile buffer.  
                        firstInput = GPU_MOD(firstInput + 1, inputBufferSize);

                        //  Update stored tiles in the tile buffer.  
                        inputTiles--;

                        //  Update reserved output tiles counter.  
                        reservedOutputs += MAXGENERATEDTILES;
                    }
                }
            }

            //  End tile evaluation.  
            if (evalTileEnd->read(cycle, (DynamicObject *&) tileInput))
            {
                //  Check if it is last tile.  
                if (tileInput->isLast())
                {
                    /*  Store tile in the output buffer to wait
                        for all the previous tiles to be sent to
                        Recursive Descent.  */


                    /*  Check there is enough space in the output buffer
                        for the last tile.  */
                    CG_ASSERT_COND(!(outputTiles >= outputBufferSize), "Not enough space in the output tile buffer for the generated tiles");
                    GPU_DEBUG(
                        printf("TileEvaluator-%s => End of Last Tile Evaluation.\n",
                            prefix);
                    )

                    //  Store last tile in the output buffer.  
                    outputBuffer[nextFreeOutput] = tileInput;

                    //  Update pointer to the next free output tile buffer.  
                    nextFreeOutput = GPU_MOD(nextFreeOutput + 1, outputBufferSize);

                    //  Update number of tiles in the output tile buffer.  
                    outputTiles++;

                    //  Update reserved output tile buffer entries.  
                    reservedOutputs--;
                }
                //  Determine if it is a stamp level fragment.  
                else if (tileInput->getTile()->getLevel() == STAMPLEVEL)
                {
                    /*  Use the rasterizer behaviorModel to generate the stamp
                        fragments.  */
                    stamp = bmRast.generateStamp(tileInput->getTile());

                    //  Send the fragments to Recursive Descent.  
                    for(i = 0; i < stampFragments; i++)
                    {
                        /*  Create Fragment Input containers for the generated
                            fragments.  */
                        frInput = new FragmentInput(tileInput->getTriangleID(),
                            tileInput->getSetupTriangleID(), stamp[i]);

                        GPU_DEBUG(
                            printf("TileEvaluator-%s => Generating stamp fragments.\n",
                                prefix);
                        )

                        //  Send fragment input back to recursive descent.  
                        newFragments->write(cycle, frInput);
                    }

                    //  Update generated fragment counter.  
                    fragmentCounter += stampFragments;

                    //  Delete array of fragment pointers.  
                    delete stamp;

                    //  Delete Tile.  
                    delete tileInput->getTile();

                    //  Delete Tile Input container.  
                    delete tileInput;
                }
                else
                {
                    //  Use the rasterizer behaviorModel to evaluate the tile.  
                    generatedTiles = bmRast.evaluateTile(tileInput->getTile(),
                        subTiles);

                    /*  Check there is enough space in the output buffer
                        for the generated tiles.  */
                    CG_ASSERT_COND(!((outputTiles + generatedTiles) > outputBufferSize), "Not enough space in the output tile buffer for the generated tiles");
                    //  Check if there are no generated tiles for this tile.  
                    if (generatedTiles == 0)
                    {
                        GPU_DEBUG(
                            printf("TileEvaluator-%s => No generated tiles.\n",
                                prefix);
                        )

                        /*  Create Tile Input container without a Tile and
                            end Tile mark.  */
                        newTileInput = new TileInput(tileInput->getTriangleID(),
                            tileInput->getSetupTriangleID(), NULL,
                            TRUE, FALSE);

                        //  Store output tiles in the output buffer.  
                        outputBuffer[nextFreeOutput] = newTileInput;

                        //  Update pointer to the next free output tile buffer.  
                        nextFreeOutput = GPU_MOD(nextFreeOutput + 1, outputBufferSize);

                        //  Update number of tiles in the output tile buffer.  
                        outputTiles++;
                    }

                    //  Add subtiles to the output buffer.  
                    for(i = 0; (generatedTiles > 0) &&
                        (i < (generatedTiles - 1)); i++)
                    {
                        GPU_DEBUG(
                            printf("TileEvaluator-%s => Generating new Tile start point at (%d, %d) level %d.\n",
                                prefix, subTiles[i]->getX(), subTiles[i]->getY(),
                                subTiles[i]->getLevel());
                        )

                        //  Create Tile Input containers for the generated Tiles.  
                        newTileInput = new TileInput(tileInput->getTriangleID(),
                            tileInput->getSetupTriangleID(), subTiles[i],
                            FALSE, FALSE);

                        //  Store output tiles in the output buffer.  
                        outputBuffer[nextFreeOutput] = newTileInput;

                        //  Update pointer to the next free output tile buffer.  
                        nextFreeOutput = GPU_MOD(nextFreeOutput + 1, outputBufferSize);

                        //  Update number of tiles in the output tile buffer.  
                        outputTiles++;
                    }

                    //  Add last generated subtile.  
                    if (generatedTiles > 0)
                    {

                        GPU_DEBUG(
                            printf("TileEvaluator-%s => Generating last new Tile start point at (%d, %d) level %d.\n",
                                prefix, subTiles[i]->getX(), subTiles[i]->getY(),
                                subTiles[i]->getLevel());
                        )

                        /*  Create Tile Input containers for the
                            last generated subtile..  */
                        newTileInput = new TileInput(tileInput->getTriangleID(),
                            tileInput->getSetupTriangleID(),
                            subTiles[generatedTiles - 1], TRUE, FALSE);

                        //  Store output tiles in the output buffer.  
                        outputBuffer[nextFreeOutput] = newTileInput;

                        //  Update pointer to the next free output tile buffer.  
                        nextFreeOutput = GPU_MOD(nextFreeOutput + 1, outputBufferSize);

                        //  Update number of tiles in the output tile buffer.  
                        outputTiles++;
                    }

                    //  Update reserved output tile buffer entries.  
                    reservedOutputs -= MAXGENERATEDTILES;

                    //  Delete Tile.  
                    delete tileInput->getTile();

                    //  Delete Tile Input container.  
                    delete tileInput;
                }

            }

            //  Send stored output tiles to Recursive Descent.  
            for(i = 0;(i < tilesCycle) && (outputTiles > 0); i++)
            {
                /*  Check last tile signal has been received and
                    processed.  */
                if (outputBuffer[firstOutput]->isLast())
                {
                    //  Update output tiles counter.  
                    outputTiles--;

                    GPU_DEBUG(
                        printf("TileEvaluator-%s => Last Tile reached end of Tile Evaluator Pipeline.\n",
                                prefix);
                        )

                    //  Check buffers.  
                    GPU_ASSERT(
                        if ((outputTiles > 0) || (inputTiles > 0) ||
                            (reservedOutputs > 0))
                            CG_ASSERT("Last Tile signal received while there is still tiles in evaluation.");
                    )

                    //  Change to rasterization end state.  
                    state = RAST_END;

                    //  Delete last tile.  
                    delete outputBuffer[firstOutput];
                }
                else
                {
                    GPU_DEBUG(
                        if (outputBuffer[firstOutput]->getTile() == NULL)
                        {
                            printf("TileEvaluator-%s => Sending to Recursive Descent an empty Tile Input.\n",
                                prefix);
                        }
                        else
                        {
                            printf("TileEvaluator-%s => Sending to Recursive Descent Tile start point at (%d, %d) level %d.\n",
                                prefix,
                                outputBuffer[firstOutput]->getTile()->getX(),
                                outputBuffer[firstOutput]->getTile()->getY(),
                                outputBuffer[firstOutput]->getTile()->getLevel());
                        }
                    )

                    //  Send tile to Recursive Descent.  
                    newTiles->write(cycle, outputBuffer[firstOutput]);

                    //  Update pointer to next output tile.  
                    firstOutput = GPU_MOD(firstOutput + 1, outputBufferSize);

                    //  Update output tiles counter.  
                    outputTiles--;
                }

            }

            //  Receive new tiles from Recursive Descent.  
            if (inputTileSignal->read(cycle, (DynamicObject *&) tileInput))
            {
                //  Check there is enough space in the input tile buffer.  
                CG_ASSERT_COND(!(inputTiles >= inputBufferSize), "Input Tile Buffer is full.");
                GPU_DEBUG(
                    if (tileInput->isLast())
                        printf("TileEvaluator-%s => Received Last Tile signal.\n",
                            prefix);
                    else
                        printf("TileEvaluator-%s => Received new Tile with start point at (%d, %d) and level %d\n",
                            prefix, tileInput->getTile()->getX(),
                            tileInput->getTile()->getY(),
                            tileInput->getTile()->getLevel());
                )

                //  Store the tile.  
                inputBuffer[nextFreeInput] = tileInput;

                //  Update pointer to next free entry in the input tile buffer.  
                nextFreeInput = GPU_MOD(nextFreeInput + 1, inputBufferSize);

                //  Update stored input tiles counter.  
                inputTiles++;

                //  Update received tiles counter.  
                tileCounter++;
            }

            break;

        case RAST_END:
            //  Draw end state.  

            GPU_DEBUG(
                printf("TileEvaluator => END state.\n");
            )

            //  Wait for END command from the Rasterizer main mdu.  
            if (rastCommand->read(cycle, (DynamicObject *&) rastCom))
                processCommand(rastCom, cycle);

            break;

        default:
            CG_ASSERT("Unsupported state.");
            break;
    }

    //  Send state to recursive descent.  
    if (inputTiles < (inputBufferSize - 2))
        evaluatorState->write(cycle, new TileEvaluatorStateInfo(TE_READY));
    else
        evaluatorState->write(cycle, new TileEvaluatorStateInfo(TE_FULL));

    //  Send current state to the main Rasterizer mdu.  
    rastState->write(cycle, new RasterizerStateInfo(state));
}


//  Processes a rasterizer command.  
void TileEvaluator::processCommand(RasterizerCommand *command, U64 cycle)
{
    U32 i, j, k;

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
               printf("TileEvaluator => RESET command received.\n");
            )

            //  Change to reset state.  
            state = RAST_RESET;

            break;

        case RSCOM_DRAW:
            //  Draw command from the Rasterizer main mdu.  

            GPU_DEBUG(
                printf("TileEvaluator => DRAW command received.\n");
            )


            //  Check current state.  
            CG_ASSERT_COND(!(state != RAST_READY), "DRAW command can only be received in READY state.");
            //  Initialize the input tile buffer.  
            for(i = 0; i < inputBufferSize; i++)
                inputBuffer[i] = NULL;

            //  Reset pointer to the first tile in the input tile buffer.  
            firstInput = 0;

            //  Reset pointer to the next free entry in the input tile buffer.  
            nextFreeInput = 0;

            //  Reset stored input tiles counter.  
            inputTiles = 0;


            //  Initialize the output tile buffer.  
            for(i = 0; i < outputBufferSize; i++)
                outputBuffer[i] = NULL;

            //  Reset pointer to the first tile in the output tile buffer.  
            firstOutput = 0;

            //  Reset pointer to the next free entry in the output tile buffer.  
            nextFreeOutput = 0;

            //  Reset stored ouput tiles counter.  
            outputTiles = 0;

            //  Reset reserved output buffer entries counter.  
            reservedOutputs = 0;

            //  Reset tile counter.  
            tileCounter = 0;

            //  Reset generated fragment counter.  
            fragmentCounter = 0;

            //  Last batch triangle has not been received yet.  
            lastTile = FALSE;

            //  Change to drawing state.  
            state = RAST_DRAWING;

            break;

        case RSCOM_END:
            //  End command received from Rasterizer main mdu.  

            GPU_DEBUG(
                printf("TileEvaluator => END command received.\n");
            )


            //  Check current state.  
            CG_ASSERT_COND(!(state != RAST_END), "END command can only be received in END state.");
            //  Change to ready state.  
            state = RAST_READY;

            break;

        case RSCOM_REG_WRITE:
            //  Write register command from the Rasterizer main mdu.  

            GPU_DEBUG(
                printf("TileEvaluator => REG_WRITE command received.\n");
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

//  Processes a write request to a GPU register used by the Tile Evaluator.  
void TileEvaluator::processRegisterWrite(GPURegister reg, U32 subreg,
    GPURegData data)
{
    //  Process register write.  
    switch(reg)
    {
	case 0: // avoids warning (case label missing)
        default:
            CG_ASSERT("Unsupported register.");
            break;
    }
}
