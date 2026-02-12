/**************************************************************************
 *
 * Triangle Traversal mdu definition file.
 *
 */


/**
 *
 *  @file cmTriangleTraversal.h
 *
 *  This file defines the Triangle Traversal mdu.
 *
 *  This mdu simulates the traversal and fragment generation
 *  inside a triangle
 *
 */

#include "GPUType.h"
#include "cmMduBase.h"
#include "GPUReg.h"
#include "cmRasterizerCommand.h"
#include "bmRasterizer.h"
#include "cmTriangleSetupOutput.h"
#include "cmRasterizerState.h"
#include "PixelMapper.h"

#ifndef _TRIANGLETRAVERSAL_

#define _TRIANGLETRAVERSAL_

namespace arch
{

//*  Defines the different Triangle Traversal  states.  
enum TriangleTraversaState
{
    TT_READY,
    TT_TRAVERSING,
};

/**
 *
 *  This class implements the Triangle Traversal mdu.
 *
 *  This class implements the Triangle Traversal mdu that
 *  simulates the Triangle Traversal and Fragment Generation
 *  unit of a GPU.
 *
 *  Inherits from the cmoMduBase class that offers basic simulation
 *  support.
 *
 */

class TriangleTraversal : public cmoMduBase
{
private:

    //  Triangle Traversal signals.  
    Signal *traversalCommand;   //  Command signal from the main Rasterizer cmoMduBase.  
    Signal *traversalState;     //  State signal to the main Rasterizer cmoMduBase.  
    Signal *setupTriangle;      //  Setup triangle signal from the Setup mdu.  
    Signal *setupRequest;       //  Triangle Traversal request to the Triangle Setup mdu.  
    Signal *newFragment;        //  New fragment signal to the Hierarchical Z mdu.  
    Signal *hzState;            //  State signal from the Hierarchical Z mdu.  

    //  Triangle Traversal registers.
    U32 hRes;                //  Display horizontal resolution.  
    U32 vRes;                //  Display vertical resolution.  
    S32 startX;              //  Viewport initial x coordinate.  
    S32 startY;              //  Viewport initial y coordinate.  
    U32 width;               //  Viewport width.  
    U32 height;              //  Viewport height.  
    bool multisampling;         //  Flag that stores if MSAA is enabled.  
    U32 msaaSamples;         //  Number of MSAA Z samples generated per fragment when MSAA is enabled.  
    bool perspectTransform;     //  Current vertex position transformation is perspective.  

    //  Triangle Traversal parameters.  
    bmoRasterizer &bmRaster;    //  Reference to the Rasterizer Behavior Model used to generate fragments.  
    U32 trianglesCycle;          //  Numbers of triangles to receive from Triangle Setup per cycle.  
    U32 setupLatency;            //  Latency of the triangle bus with Triangle Setup.  
    U32 stampsCycle;             //  Stamps to generate each cycle.  
    U32 samplesCycle;            //  Number of depth samples per fragment that can be generated per cycle when MSAA is enabled.  
    U32 triangleBatch;           //  Number of triangles to batch for (recursive) traversal.  
    U32 triangleQSize;           //  Size of the triangle queue.  
    bool recursiveMode;             //  Flags that determines the rasterization method (TRUE: recursive, FALSE: scanline based).  
    U32 numStampUnits;           //  Number of stamp units in the GPU.  
    U32 overW;                    //  Over scan tile width in scan tiles.  
    U32 overH;                    //  Over scan tile height in scan tiles.  
    U32 scanW;                    //  Scan tile width in generation tiles.  
    U32 scanH;                    //  Scan tile height in generation tiles.  
    U32 genW;                     //  Generation tile width in stamps.  
    U32 genH;                     //  Generation tile height in stamps.  

    //  Triangle Traversal state.  
    RasterizerState state;                  //  Current triangle traversal state.  
    RasterizerCommand *lastRSCommand;       //  Stores the last Rasterizer Command received (for signal tracing).  
    U32 triangleCounter;                 //  Number of processed triangles.  
    TriangleSetupOutput **triangleQueue;    //  Pointer an array of stored triangles to be traversed.  
    U32 storedTriangles;                 //  Number of stored triangles.  
    U32 nextTriangle;                    //  Pointer to the first triangle in the next batch to traverse.  
    U32 nextStore;                       //  Pointer to the next position where to store a new triangle.  
    U32 batchID;                         //  Identifier of the current batch of triangle being rasterized.  
    U32 batchTriangles;                  //  Number of triangles in the current batch.  
    U32 requestedTriangles;              //  Number of triangles requested to Triangle Setup.  
    U32 fragmentCounter;                 //  Number of fragments generated in the current batch/stream.  
    U32 nextMSAAQuadCycles;              //  Number of cycles until the next group of quads can be processed by the MSAA sample generator.  
    U32 msaaCycles;                      //  Number of cycles that it takes to generate all the MSAA samples for a fragment/quad.  
    bool traversalFinished;                 //  Stores if the traversal of the current triangle has finished.  
    bool lastFragment;                      //  Last batch fragment was generated.  

    //  Pixel Mapper
    PixelMapper pixelMapper;    //  Maps pixels to addresses and processing units.  

    //  Statistics.  
    gpuStatistics::Statistic *inputs;            //  Input triangles.  
    gpuStatistics::Statistic *requests;          //  Triangle requests.  
    gpuStatistics::Statistic *outputs;           //  Output fragments.  
    gpuStatistics::Statistic* utilizationCycles; //  Cycle when rasterizer is doing real work (ie: waiting HZ is not accounted as utilizationCycles) 

    // Stamp coverage related stats 
    gpuStatistics::Statistic *stamps0frag;           //  Empty stamps.                              
    gpuStatistics::Statistic *stamps1frag;           //  Output stamps with 1 covered fragment.     
    gpuStatistics::Statistic *stamps2frag;           //  Output stamps with 2 covered fragments.    
    gpuStatistics::Statistic *stamps3frag;           //  Output stamps with 3 covered fragments.    
    gpuStatistics::Statistic *stamps4frag;           //  Full covered stamps.                       

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
     *  Triangle Traversal constructor.
     *
     *  Creates and initializes a Triangle Traversal mdu.
     *
     *  @param rasEmu Reference to the Rasterizer Behavior Model to be used by Triangle Traversal to generate fragments.
     *  @param trianglesCycle Number of triangle received from Triangle Setup per cycle.
     *  @param setupLat Latency of the triangle bus with Triangle Setup.
     *  @param stampsCycle Number of stamps that Triangle Traversal has to generate per cycle.
     *  @param samplesCycle Number of depth samples per fragment that can be generated per cycle when MSAA is enabled.
     *  @param batchTriangles Number of triangles to batch for triangle traversal.
     *  @param trQueueSize Number of triangles to store in the triangle queue.
     *  @param recursiveMode Flags that determines the rasterization method to use.  If the
     *  flag is true it uses recursive rasterization and if it is false scanline based rasterization.
     *  @param microTrisAsFrag Process micro triangles directly as fragments, skipping setup and traversal operations on the triangle.
     *  @param nStampUnits Number of stamp units in the GPU.
     *  @param overW Over scan tile width in scan tiles (may become a register!).
     *  @param overH Over scan tile height in scan tiles (may become a register!).
     *  @param scanW Scan tile width in fragments.
     *  @param scanH Scan tile height in fragments.
     *  @param genW Generation tile width in fragments.
     *  @param genH Generation tile height in fragments.
     *  @param name The mdu name.
     *  @param parent The mdu parent mdu.
     *
     *  @return A new initialized Triangle Traversal mdu.
     *
     */

    TriangleTraversal(bmoRasterizer &rasEmu, U32 trianglesCycle, U32 setupLat,
        U32 stampsCycle, U32 samplesCycle, U32 batchTriangles, U32 trQueueSize,       
        bool recursiveMode, bool microTrisAsFrag, U32 nStampUnits,
        U32 overW, U32 overH, U32 scanW, U32 scanH, U32 genW, U32 genH,
        char *name, cmoMduBase *parent);

    /**
     *
     *  Triangle Traversal simulation function.
     *
     *  Performs the simulation of a cycle of the Triangle
     *  Traversal mdu.
     *
     *  @param cycle The current simulation cycle.
     *
     */

    void clock(U64 cycle);

    /**
     *
     *  Returns a single line string with state and debug information about the
     *  Triangle Traversal mdu.
     *
     *  @param stateString Reference to a string where to store the state information.
     *
     */

    void getState(std::string &stateString);

};

} // namespace arch

#endif

