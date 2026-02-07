/**************************************************************************
 *
 * Recursive Rasterizer mdu class definition file.
 *
 */

/**
 *
 *  @file RecursivecmRasterizer.h
 *
 *  This file defines the Recursive Rasterizer mdu.
 *
 *  The Rasterizer mdu controls all the rasterizer boxes:
 *  Triangle Setup, Recursive Descent, Tile Evaluators,
 *  Interpolator and Fragment FIFO.
 *
 */


#ifndef _RECURSIVERASTERIZER_

#define _RECURSIVERASTERIZER_

#include "GPUType.h"
#include "support.h"
#include "cmMduBase.h"
#include "cmRasterizerCommand.h"
#include "GPUReg.h"
#include "cmRasterizerState.h"
#include "cmTriangleSetup.h"
#include "cmRecursiveDescent.h"
#include "cmTileEvaluator.h"
#include "cmInterpolator.h"

namespace cg1gpu
{

/**
 *
 *  Recursive Rasterizer mdu class definition.
 *
 *  This class implements a Recursive Rasterizer mdu that renders
 *  triangles from the vertex calculated in the vertex
 *  shader.
 *
 */

class RecursiveRasterizer: public cmoMduBase
{
private:

    //  Recursive Rasterizer signals.  
    Signal *rastCommSignal;     //  Command signal from the Command Processor.  
    Signal *rastStateSignal;    //  Recursive Rasterizer state signal to the Command Processor.  
    Signal *triangleSetupComm;  //  Command signal to Triangle Setup.  
    Signal *triangleSetupState; //  State signal from Triangle Setup.  
    Signal *recDescComm;        //  Command signal to Recursive Descent.  
    Signal *recDescState;       //  State signal from Recursive Descent.  
    Signal **tileEvalComm;      //  Command signals to the Tile Evaluators.  
    Signal **tileEvalState;     //  State signals from the Tile Evaluators.  
    Signal *interpolatorComm;   //  Command signal to Interpolator mdu.  
    Signal *interpolatorState;  //  State signal from Interpolator mdu.  

    //  Recursive Rasterizer boxes.  
    TriangleSetup *triangleSetup;   //  Triangle Setup mdu.  
    RecursiveDescent *recDescent;   //  Recursive Descent mdu.  
    TileEvaluator **tileEvaluator;  //  Tile Evaluators boxes.  
    Interpolator *interpolator;     //  Interpolator mdu.  

    //  Recursive Rasterizer registers.  
    S32 viewportIniX;            //  Viewport initial (lower left) X coordinate.  
    S32 viewportIniY;            //  Viewport initial (lower left) Y coordinate.  
    U32 viewportHeight;          //  Viewport height.  
    U32 viewportWidth;           //  Viewport width.  
    F32 nearRange;               //  Near depth range.  
    F32 farRange;                //  Far depth range.  
    U32 zBufferBitPrecission;    //  Z Buffer bit precission.  
    bool frustumClipping;           //  Frustum clipping flag.  
    Vec4FP32 userClip[MAX_USER_CLIP_PLANES];       //  User clip planes.  
    bool userClipPlanes;            //  User clip planes enabled or disabled.  
    CullingMode cullMode;           //  Culling Mode.  
    bool interpolation;             //  Interpolation enabled or disabled (FLAT/SMOTH).  
    bool fragmentAttributes[MAX_FRAGMENT_ATTRIBUTES];   //  Fragment input attributes active flags.  

    //  Recursive Rasterizer parameters.  
    bmoRasterizer &bmRast;    //  Reference to the Rasterizer Behavior Model that is going to be used.  
    U32 triangleSetupFIFOSize;   //  Size of the Triangle Setup FIFO.  
    U32 recDescTrFIFOSize;       //  Recursive Descent triangle FIFO size.  
    U32 recDescTileStackSize;    //  Size of the Tile Stack at Recursive Descent.  
    U32 recDescFrBufferSize;     //  Recursive Descent fragment buffer size.  
    U32 stampFragments;          //  Number of fragments per stamp (lowest level tile).  
    U32 stampsCycle;             //  Number of stamps that can be send to Interpolator per cycle.  
    U32 numTileEvaluators;       //  Number of Tile Evaluators attached to Recursive Descent.  
    U32 tilesCycle;              //  Tiles that can be generated and sent per cycle from the Tile Evaluator to Recursive Descent.  
    U32 teInputBufferSize;       //  Tile Evaluator input tile buffer size.  
    U32 teOutputBufferSize;      //  Tile Evaluator output tile buffer size.  
    U32 tileEvalLatency;         //  Tile Evaluator evaluation latency per tile.  
    char **tileEvalPrefixes;        //  Array with the name prefixes for the differente Tile Evaluator boxes attached to Recursive Descent.  
    U32 interpolators;           //  Number of hardware interpolators.  

    //  Recursive Rasterizer state.  
    RasterizerState state;          //  Current state of the Recursive Rasterizer unit.  
    RasterizerCommand *lastRastComm;//  Last rasterizer command received.  
    RasterizerState *teState;       //  Array for the Tile Evaluators current state.  

    //  Private functions.  

    /**
     *
     *  Processes new rasterizer commands received from the
     *  Command Processor.
     *
     *  @param rastComm The RasterizerCommand to process.
     *  @param cycle The current simulation cycle.
     *
     */

    void processRasterizerCommand(RasterizerCommand *rastComm, U64 cycle);

    /**
     *
     *  Processes a rasterizer register write.
     *
     *  @param reg The rasterizer register to write to.
     *  @param subreg The Rasterizer register subregister to write to.
     *  @param data The data to write to the register.
     *  @param cycle The current simulation cycle.
     *
     */

    void processRegisterWrite(GPURegister reg, U32 subreg, GPURegData data,
        U64 cycle);

public:

    /**
     *
     *  Recursive Rasterizer constructor.
     *
     *  This function creates and initializates the Recursive Rasterizer
     *  mdu state and  structures.
     *
     *  @param bmRast Reference to the Rasterizer Behavior Model that will be
     *  used for emulation.
     *  @param tsFIFOSize Size of the Triangle Setup FIFO.
     *  @param rdTriangleFIFOSize Recursive Descent Triangle FIFO size.
     *  @param rdTileStackSize Recursive Descent Tile Stack size.
     *  @param rdFrBufferSize Recursive Descent Fragment Buffer Size.
     *  @param stampFragments Fragments per stamp (lowest level tile).
     *  @param stampsCycle Stamps sent to Interpolator and Fragment FIFO
     *  boxes per cycle.
     *  @param tileEvals Number of Tile Evaluators attached to the
     *  Recursive Descent mdu.
     *  @param tilesCycle Number of tiles that can be generated and sent
     *  back to Recursive Descent per Tile Evaluator each cycle.
     *  @param teInputBufferSize Tile Evaluator Input Tile Buffer size.
     *  @param teOutputBufferSize Tile Evaluator Output Tile Buffer size.
     *  @param tileEvalLatency Tile Evaluator evaluation latency per tile.
     *  @param tileEvalPrefixes Tile Evaluator name and signal prefixes array.
     *  @param interp  Number of hardware interpolators available.
     *  @param name  Name of the mdu.
     *  @param parent  Pointer to a parent mdu.
     *
     */

    RecursiveRasterizer(bmoRasterizer &bmRast, U32 tsFIFOSize,
        U32 rdTriangleFIFOSize, U32 rdTileStackSize,
        U32 rdFrBufferSize, U32 stampFragments, U32 stampsCycle,
        U32 tileEvals, U32 tilesCycle, U32 teInputBufferSize,
        U32 teOutputBufferSize, U32 tileEvalLatency,
        char **tileEvalPrefixes, U32 interp,
        char *name, cmoMduBase *parent = 0);

    /**
     *
     *  Performs the simulation and emulation of the rasterizer
     *  cycle by cycle.
     *
     *  @param cycle  The cycle to simulate.
     *
     */

    void clock(U64 cycle);
};

} // namespace cg1gpu

#endif
