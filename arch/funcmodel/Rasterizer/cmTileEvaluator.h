/**************************************************************************
 *
 * Tile Evaluator mdu definition file.
 *
 */

 /**
 *
 *  @file cmTileEvaluator.h
 *
 *  This file defines the Tile Evaluator mdu.
 *
 *  This class simulates the Tile Evaluator used for recursive descent
 *  rasterization in a GPU.  The Tile Evaluator evaluates a triangle
 *  (or triangles) against a tiled section of the viewport.
 *
 */

#ifndef _TILEEVALUATOR_

#define _TILEEVALUATOR_

#include "GPUType.h"
#include "cmMduBase.h"
//#include "Signal.h"
#include "GPUReg.h"
#include "support.h"
#include "bmRasterizer.h"
#include "cmRasterizerState.h"
#include "cmRasterizerCommand.h"
#include "cmTileInput.h"

namespace cg1gpu
{

/**
 *
 *  Defines the Tile Evaluator state for Recursive Descent.
 */
enum TileEvaluatorState
{
    TE_READY,       //  The Tile Evaluator can receive new tiles.  
    TE_FULL         //  The Tile Evaluator can not receive new tiles.  
};

/**
 *
 *  This class implements the Tile Evaluator simulation mdu.
 *
 *  The Tile Evaluator mdu simulates the Tile Evaluator used for recursive
 *  descent rasterization in a GPU.
 *  Receives tiles from Recursive Descent mdu and evaluates them against the
 *  triangles being rasterized and the hierarchical Z buffer.
 *
 *  This class inherits from the cmoMduBase class that offers basic simulation
 *  support.
 *
 */

class TileEvaluator : public cmoMduBase
{
private:

    //  Tile Evaluator signals.  
    Signal *rastCommand;        //  Rasterizer command signal from the Rasterizer main mdu.  
    Signal *rastState;          //  Rasterizer state signal to the Rasterizer main mdu.  
    Signal *evalTileStart;      //  Evaluate tile signal to the Tile Evaluator (evaluation latency).  
    Signal *evalTileEnd;        //  Evaluated tile signal from the Tile Evaluator.  (evaluation latency).  
    Signal *newTiles;           //  New tile signals to the Recursive Descent.  
    Signal *inputTileSignal;    //  Input tile signal from the Recursive Descent.  
    Signal *evaluatorState;     //  State signal to the Recursive Descent.  
    Signal *newFragments;       //  Generated fragment signal to the Recursive Descent.  
    Signal *hzRequest;          //  Data signal from the Hierarchical Z Buffer.  

    //  Tile Evaluator state.  
    RasterizerState state;      //  Current state of the Tile Evaluator mdu.  
    RasterizerCommand *lastRSCommand;   //  Stores the last Rasterizer Command received (for signal tracing).  
    U32 tileCounter;         //  Number of processed tiles.  
    bool lastTile;              //  Last batch tile/triangle received flag.  
    U32 fragmentCounter;     //  Number of fragments generated in the current batch/stream.  

    //  Tile Evaluator registers.  

    //  Tile Evaluator parameters.  
    bmoRasterizer &bmRaster;//  Reference to the rasterizer behaviorModel used by the Tiled Evaluator.  
    U32 tilesCycle;          //  Number of generated tiles sent back to Recursive Descent per cycle.  
    U32 stampFragments;      //  Number of fragments per stamp.  
    U32 inputBufferSize;     //  Size of the input tile buffer.  
    U32 outputBufferSize;    //  Size of the output tile buffer.  
    U32 evaluationLatency;   //  Tile evaluation latency.  
    char *prefix;               //  Tile evaluator prefix.  

    //  Tile Evaluator Input buffer.  
    TileInput **inputBuffer;    //  Input tile buffer, stores tiles to be evaluated.  
    U32 firstInput;          //  Pointer to the first tile in the input buffer.  
    U32 nextFreeInput;       //  Pointer to the next free entry in the input buffer.  
    U32 inputTiles;          //  Number of stored tiles in the input buffer.  

    //  Tile Evaluator Output Buffer.  
    TileInput **outputBuffer;   //  Output tile buffer, stores generated tiles to be sent to Recursive descent.  
    U32 firstOutput;         //  Pointer to the first tile in the output buffer.  
    U32 nextFreeOutput;      //  Pointer to the next free entry in the output buffer.  
    U32 outputTiles;         //  Number of tiles stored in the output buffer.  
    U32 reservedOutputs;     //  Number of reserved output buffer entries.  

    //  Private functions.  

    /**
     *
     *  Processes a rasterizer command.
     *
     *  @param command The rasterizer command to process.
     *  @param cycle  Current simulation cycle.
     *
     */

    void processCommand(RasterizerCommand *command, U64 cycle);

    /**
     *
     *  Processes a register write.
     *
     *  @param reg The Triangle Traversal register to write.
     *  @param subreg The register subregister to writo to.
     *  @param data The data to write to the register.
     *
     */

    void processRegisterWrite(GPURegister reg, U32 subreg, GPURegData data);


public:

    /**
     *
     *  Tile Evaluator constructor.
     *
     *  Creates and initializes a new Tile Evaluator mdu.
     *
     *  @param bmRaster Reference to the rasterizer behaviorModel to be used
     *  to emulate rasterization operations.
     *  @param tilesCycle Number of generated tiles that can be sent
     *  back to Recursive Descent per cycle.
     *  @param stampFragments Number of fragments per stamp.
     *  @param inputBufferSize Size of the input tile buffer.
     *  @param outputBufferSize Size of the output tile buffer.
     *  @param evalLatency Tile evaluation latency.
     *  @param prefix Prefix name for the Tile Evaluator.
     *  @param name Name of the mdu.
     *  @param parent Parent mdu of the mdu.
     *
     *  @return A new initialized Tile Evaluator mdu.
     *
     */

    TileEvaluator(bmoRasterizer &bmRaster, U32 tilesCycle,
        U32 stampFragments, U32 inputBufferSize, U32 outputBufferSize,
        U32 evalLatency, char *prefix, char *name, cmoMduBase* parent);

    /**
     *
     *  Tile Evaluator simulation rutine.
     *
     *  Simulates a cycle of the Tile Evaluator mdu.
     *
     *  @param cycle The cycle to simulate of the Tile Evaluator mdu.
     *
     */

    void clock(U64 cycle);

};

} // namespace cg1gpu

#endif
