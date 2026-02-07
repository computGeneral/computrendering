/**************************************************************************
 *
 * Fragment FIFO (unified shaders version) class definition file.
 *
 */

/**
 *
 *  @file cmFragmentFIFO.h
 *
 *  This file defines the Fragment FIFO class.
 *
 *  This class implements a Fragment FIFO simulation mdu.  The Fragment FIFO stores
 *  and directs stamps of fragments from the rasterizer to the Interpolator and Shader
 *  units.  It also stores shaded fragments and sends them Z Stencil Test.
 *
 */


#ifndef _FRAGMENTFIFO_

#define _FRAGMENTFIFO_



#include "cmMduBase.h"
#include "GPUType.h"
#include "support.h"
#include "GPUReg.h"
#include "cmRasterizerState.h"
#include "cmRasterizerCommand.h"
#include "cmInterpolator.h"
#include "cmZStencilTestV2.h"
#include "cmColorWriteV2.h"
#include "cmShaderFetch.h"
#include "cmConsumerStateInfo.h"
#include "cmFragmentInput.h"
#include "cmFragmentFIFOState.h"
//#include <fstream>
#include "zfstream.h"

using namespace std;

namespace cg1gpu
{

/**
 *
 *  Defines a number of cycles as threshold after a group of vertex inputs
 *  smaller than STAMP_FRAGMENTS is sent to the shaders.
 *
 */
static const U32 VERTEX_CYCLE_THRESHOLD = 1000;

/**
 *
 *  This class implements the Fragment FIFO mdu (unified shaders version).
 *
 *  The Fragment FIFO mdu simulates a storage unit that redirects fragments produced in
 *  the rasterizer to the Interpolator and Shader units for shading, receives shaded
 *  fragments from the Shader and stores them until they can be processed in the
 *  Z and Stencil Test unit.
 *
 *  Inherits from the cmoMduBase class that offers basic simulation support.
 *
 */

class FragmentFIFO: public cmoMduBase
{

private:

#ifdef GPU_TEX_TRACE_ENABLE
    //ofstream fragTrace;
    gzofstream fragTrace;
#endif

    //  Fragment FIFO Signals.  
    Signal *hzInput;            //  Stamp signal from Hierarchical/Early Z mdu.  
    Signal *ffStateHZ;          //  Fragment FIFO tate signal to the Hierarchical/Early Z mdu.  
    Signal *interpolatorInput;  //  Input stamp signal to the Interpolator unit.  
    Signal *interpolatorOutput; //  Output stamp signal from the Interpolator unit.  
    Signal **shaderInput;       //  Array of stamp input signal to the Shader unit.  
    Signal **shaderOutput;      //  Array of stamp output signal from the Shader unit.  
    Signal **shaderState;       //  Array of state signal from the Shader unit.  
    Signal **ffStateShader;     //  Array of Fragment FIFO state signals to the Shader unit.  
    Signal **vertexInput;       //  Array of vertex input signals from cmoStreamController Loader.  
    Signal **vertexOutput;      //  Array of vertex output signals to cmoStreamController Commit.  
    Signal **vertexState;       //  Array of vertex state signals to cmoStreamController Loader.  
    Signal **vertexConsumer;    //  Array of vertex consumer state signals from cmoStreamController Commit.  
    Signal *triangleInput;      //  Triangle shader input signal from Triangle Setup.  
    Signal *triangleOutput;     //  Triangle shader output signal to Triangle Setup.  
    Signal *triangleState;      //  Triangle shader state signal to Triangle Setup.  
    Signal **zStencilInput;     //  Array of input stamp signal to the Z Stencil Test unit.  
    Signal **zStencilOutput;    //  Array of stamp signal from the Z Stencil Test unit.  
    Signal **zStencilState;     //  Array of state signal from the Z Stencil Test unit.  
    Signal **fFIFOZSTState;     //  Array of state signal to the Z Stencil Test unit.  
    Signal **cWriteInput;       //  Array of input stamp signal to the Color Write unit.  
    Signal **cWriteState;       //  Array of state signal from the Color Write unit.  
    Signal *fFIFOCommand;       //  Command signal from the main rasterizer mdu.  
    Signal *fFIFOState;         //  Command signal to the main rasterizer mdu.  

    //  Fragment FIFO parameters.  
    U32 hzStampsCycle;       //  Number of stamps received from Hierarchical Z per cycle.  
    U32 stampsCycle;         //  Number of stamps per cycle send down the pipeline.  
    bool unifiedModel;          //  Simulate an unified shader architecture.  
    bool shadedSetup;           //  Triangle setup performed in the shader.  
    U32 trianglesCycle;      //  Triangles received/sent from/to Triangle Setup per cycle.  
    U32 triangleLat;         //  Latency of the triangle input/output signals with Triangle Setup.  
    U32 numVShaders;         //  Number of virtual vertex shader visible to Stremer units.  
    U32 numFShaders;         //  Number of Fragment Shaders attached to Fragment FIFO.  
    U32 threadGroup;         //  Size of a thread group (packet of inputs to send to the shader).  
    U32 shInputsCycle;       //  Shader inputs per cycle that can be sent to a shader.  
    U32 shOutputsCycle;      //  Shader outputs per cycle that can be received from a shader.  
    U32 fshOutLatency;       //  Maximum latency of the shader stamp output signal.  
    U32 interpolators;       //  Number of attribute interpolator ALUs in the Interpolator unit.  
    U32 shInputQSize;        //  Size of the shader input queue.  
    U32 shOutputQSize;       //  Size of the shader output queue.  
    U32 shInputBatchSize;    //  Consecutive shader inputs assigned per shader unit work batch.  
    bool tiledShDistro;         //  Enable/disable distribution of fragment inputs to the shader units on a tile basis.  
    U32 vInputQueueSz;       //  Vertex input queue size (in vertices).  
    U32 vShadedQueueSz;      //  Vertex output/shaded queue size (in vertices).  
    U32 triangleInQueueSz;   //  Size of the triangle input queue (in triangles).  
    U32 triangleOutQueueSz;  //  Size of the triangle output queue (in triangles).  
    U32 rastQueueSz;         //  Rasterizer (Generated fragments) queue size (per stamp unit).  
    U32 testQueueSz;         //  Early Z Test (fragments after Z Stencil) queue size.  
    U32 intQueueSz;          //  Interpolator (interpolated fragments) queue size.  
    U32 shQueueSz;           //  Shader (shaded fragments) queue size (per stamp unit).  
    U32 numStampUnits;       //  Number of stamp units attached to the Rasterizer.  

    U32 stampsPerUnit;       //  Precalculated number of stamps to be processed per cycle and stamp/quad unit (ZST or CW).  
    U32 stampsPerGroup;      //  Precalculated number of stamps per thread group.  

    //  Fragment FIFO structures.  
    FragmentInput ****rastQueue;//  Queue for the stamps received from Fragment Generation per stamp unit.  
    U32 *nextRast;           //  Next stamp in the rasterizer queue to be sent to the Interpolator.  
    U32 *nextFreeRast;       //  Next free entry in the rasterizer queue.  
    U32 *rastStamps;         //  Stamps in the rasterizer stamp queue.  
    U32 *freeRast;           //  Number of free entries in the rasterizer stamp queue.  
    U32 allRastStamps;       //  Number of stamps in all the rasterizer stamp queues.  
    U32 nextRastUnit;        //  Pointer to the next queue from where to read rasterizer stamps.  

    FragmentInput ****testQueue;//  Queue for the stamps after early Z/Stencil test per stamp unit.  
    U32 *nextTest;           //  Next stamp in the post early z test queue.  
    U32 *nextFreeTest;       //  Next free entry in the post early z test queue.  
    U32 *testStamps;         //  Stamps in the post early z stamp queue.  
    U32 *freeTest;           //  Number of free entries in the post early z test stamp queue.  
    U32 allTestStamps;       //  Number of stamps in all the after early Z stamp queues.  
    U32 nextTestUnit;        //  Pointer to the next queue from where to read tested stamps.  

    FragmentInput ****intQueue; //  Queue for the stamps interpolated per stamp unit.  
    U32 *nextInt;            //  Next stamp in the interpolated queue to be sent to the shader.  
    U32 *nextFreeInt;        //  Next free entry in the interpolator queue.  
    U32 *intStamps;          //  Stamps in the interpolated stamp queue.  
    U32 *freeInt;            //  Number of free entries in the interpolated stamp queue.  
    U32 allIntStamps;        //  Number of interpolated stamps in all the queues.  
    U32 nextIntUnit;         //  Pointer to the next queue from which to read interpolated stamps.  

    FragmentInput ****shQueue;  //  Queue for the stamps shaded per stamp unit.  
    bool **shaded;              //  Stores if a stamp in the shaded queue has been really shaded per stamp unit (to keep order even with Shader out ordering the fragments).  
    U32 *nextShaded;         //  Next stamp in the shaded queue to be sent to Z Stencil Test per stamp unit.  
    U32 *nextFreeSh;         //  Next free enry in the shaded queue per stamp unit.  
    U32 *shadedStamps;       //  Stamps in the shaded stamp queue per stamp unit.  
    U32 *freeShaded;         //  Number of free entries in the shaded stamp queue per stamp unit.  

    //  Vertex queues.  
    ShaderInput **inputVertexQueue; //  Queue storing vertex inputs (unshaded).  
    U32 nextInputVertex;         //  Pointer to the next vertex in the vertex input queue.  
    U32 nextFreeInput;           //  Pointer to the next free entry in the vertex input queue.  
    U32 vertexInputs;            //  Number of vertices in the input queue.  
    U32 freeInputs;              //  Number of free entries in the vertex input queue.  

    ShaderInput **shadedVertexQueue;//  Queue storing vertex output (shaded).  
    U32 nextShadedVertex;        //  Pointer to the next vertex in the shaded vertex queue.  
    U32 nextFreeShVertex;        //  Pointer to the next free entry in the shaded vertext queue.  
    U32 shadedVertices;          //  Number of vertices in the shaded vertex queue.  
    U32 freeShVertices;          //  Number of free entries in the shaded vertex queue.  

    //  Triangle queue.  
    ShaderInput **triangleInQueue;  //  Queue storing triangle shader inputs.  
    U32 nextInputTriangle;       //  Pointer to the next triangle to send to the shader.  
    U32 nextFreeInTriangle;      //  Pointer to the next free entry in the queue.  
    U32 inputTriangles;          //  Number of triangles waiting to be sent to the shader.  
    U32 freeInputTriangles;      //  Number of free triangle input queue entries.  

    ShaderInput **triangleOutQueue; //  Queue storing triangle shader outputs.  
    U32 nextOutputTriangle;      //  Pointer to the next triangle to send back to Triangle Setup.  
    U32 nextFreeOutTriangle;     //  Pointer to the next free entry in the queue.  
    U32 outputTriangles;         //  Number of triangles waiting to be sent back to Triangle Setup.  
    U32 freeOutputTriangles;     //  Number of free triangle output queue entries.  

    //  Shader batch queues.  
    ShaderInput ***shaderInputQ;    //  Queue storing shader inputs to be sent to the shader (per shader unit).  
    U32 *nextShaderInput;        //  Next input to send to a shader in the shader input queue.  
    U32 *nextFreeShInput;        //  Next free entry in the shader input queue.  
    U32 *numShaderInputs;        //  Number of inputs waiting to be sent in the shader input queue.  
    U32 *numFreeShInputs;        //  Number of free entries in the shader input queue.  
    U32 nextFreeShInQ;           //  Next queue where to store shader inputs.  
    U32 nextSendShInQ;           //  Next queue from which to send inputs to the shader.  
    U64 inputFragments;          //  Number of inputs fragments sent to the shaders.  

    ShaderInput ***shaderOutputQ;   //  Queue storing shader outputs received from the shader (per shader unit).  
    U32 *nextShaderOutput;       //  Next output received from a shader to process in the shader output queue.  
    U32 *nextFreeShOutput;       //  Next free entry in the shader output queue.  
    U32 *numShaderOutputs;       //  Number of outputs waiting to be processed in the shader output queue.  
    U32 *numFreeShOutputs;       //  Number of free entries in the shader output queue.  
    U32 nextProcessShOutQ;       //  Next queue from which to process shader outputs.  
    U64 outputFragments;         //  Number of output fragments sent to the shaders.  

    U32 *batchedShInputs;        //  Number of consecutive shader inputs sent to a shader unit.  

    //  Stall detection state.
    U64 *lastCycleShInput;       //  Stores per shader unit the last cycle a shader input was sent to the shader unit.  
    U64 *lastCycleShOutput;      //  Stores per shader unit the last cycle a shader output was received from the shader unit.  
    U32 *shaderElements;         //  Stores per shader unit the number of elements being processed in the shader unit.  
    U64 *lastCycleZSTIn;         //  Stores per ROP unit the last cycle an input was sent to the ZStencilTest unit.  
    U64 *lastCycleCWIn;          //  Stores per ROP unit the last cycle an input was sent to the Color Write unit.  
    
    //  Fragment FIFO registers.  
    bool earlyZ;                //  Flag that enables or disables early Z testing (Z Stencil before shading).  
    bool fragmentAttributes[MAX_FRAGMENT_ATTRIBUTES];   //  Stores the fragment input attributes that are enabled and must be calculated.  
    bool stencilTest;               //  Enable/Disable Stencil test flag.  
    bool depthTest;                 //  Enable/Disable depth test flag.  
    bool rtEnable[MAX_RENDER_TARGETS];      //  Enable/Disable render target.  
    bool colorMaskR[MAX_RENDER_TARGETS];    //  Update to color buffer Red component enabled.  
    bool colorMaskG[MAX_RENDER_TARGETS];    //  Update to color buffer Green component enabled.  
    bool colorMaskB[MAX_RENDER_TARGETS];    //  Update to color buffer Blue component enabled.  
    bool colorMaskA[MAX_RENDER_TARGETS];    //  Update to color buffer Alpha component enabled.  


    //  Fragment FIFO state.  
    RasterizerState state;      //  Current Fragment FIFO mdu state.  
    RasterizerCommand *lastRSCommand;   //  Last rasterizer command received.  
    ConsumerState *consumerState;       //  Array for storing the state from cmoStreamController Commit.  
    ShaderState *shState;       //  Array for storing the state from the Fragment shaders.  
    ROPState *zstState;         //  Array for storing the state from the Z Stencil Units.  
    ROPState *cwState;          //  Array for storing the state from the Color Write Units.  
    U32 fragmentCounter;     //  Number of received fragments.  
    bool lastFragment;          //  Stores if the last fragment has been received.  
    U32 interpCycles;        //  Cycles until the next stamp can be sent to the Interpolator (depends on the number of attributes to interpolate).  
    U32 waitIntCycles;       //  Cycles remaining until the interpolator is ready again to receive a stamp.  
    bool *lastStampInterpolated;//  Flag storing if the last stamp passed through the interpolator.  
    bool *lastShaded;           //  Stores if the last stamp has been queued in the stamp unit shaded queues.  
    U32 notLastSent;         //  Number of Z Stencil Test/Color Writes units waiting for last fragment.  
    bool lastVertexInput;       //  Flag storing if the last vertex of a batch was issued by any cmoStreamController Loader Unit.  
    bool lastVertexCommit;      //  Flag storing if the last vertex of a batch was send to PA by the cmoStreamController Commit unit.  
    bool lastTriangle;          //  Last triangle input in the batch received.  
    bool firstVertex;           //  Stores if the first vertex of the current batch has been received.  
    U64 lastVertexGroupCycle;//  Stores the cycle that the last vertex group was issued the shader units (used to prevent deadlocks).  

    //  Statistics.  
    gpuStatistics::Statistic *inputs;       //  Input fragments from HZ.  
    gpuStatistics::Statistic *interpolated; //  Interpolated fragments.  
    gpuStatistics::Statistic *shadedFrag;   //  Shaded fragments.  
    gpuStatistics::Statistic *outputs;      //  Output fragments to Z Stencil.  
    gpuStatistics::Statistic *inVerts;      //  Input vertices.  
    gpuStatistics::Statistic *shVerts;      //  Shaded vertices.  
    gpuStatistics::Statistic *outVerts;     //  Output vertices.  
    gpuStatistics::Statistic *inTriangles;  //  Input triangles.  
    gpuStatistics::Statistic *outTriangles; //  Output triangles.  
    gpuStatistics::Statistic *shTriangles;  //  Shaded triangles.  
    gpuStatistics::Statistic *rastGLevel;   //  Occupation of the rasterized stamp queues.  
    gpuStatistics::Statistic *testGLevel;   //  Occupation of the early z tested stamp queues.  
    gpuStatistics::Statistic *intGLevel;    //  Occupation of the interpolated stamp queues.  
    gpuStatistics::Statistic *shadedGLevel; //  Occupation of the shader stamp queues.  
    gpuStatistics::Statistic *vertInLevel;  //  Occupation of the vertex input queue.  
    gpuStatistics::Statistic *vertOutLevel; //  Occupation of the vertex output queue.  
    gpuStatistics::Statistic *trInLevel;    //  Occupation of the triangle input queue.  
    gpuStatistics::Statistic *trOutLevel;   //  Occupation of the triangle output queue.  

    //  Private functions.  

    /**
     *
     *  Receives new stamps from Fragment Generation.
     *
     *  @param cycle The current simulation cycle.
     *
     */

    void receiveStamps(U64 cycle);

    /**
     *
     *  Receives triangles from Triangle Setup.
     *
     *  @param cycle The current simulation cycle.
     *
     */
    void receiveTriangles(U64 cycle);

    /**
     *
     *  Receives vertices from cmoStreamController Loader.
     *
     *  @param cycle The current simulation cycle.
     *
     */
    void receiveVertices(U64 cycle);

    /**
     *
     *  Sends shaded stamps to the Z Stencil unit.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void sendStampsZStencil(U64 cycle);

    /**
     *
     *  Sends shaded stamps to the Color Write unit.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void sendStampsColorWrite(U64 cycle);

    /**
     *
     *  Sends z tested stamps to the Color Write unit.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void sendTestedStampsColorWrite(U64 cycle);

    /**
     *
     *  Sends shaded triangles to Triangle Setup.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void sendTriangles(U64 cycle);

    /**
     *
     *  Sends shaded vertices to cmoStreamController Commit.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void sendVertices(U64 cycle);

    /**
     *
     *  Shades stamps.  Sends interpolated stamps to the Fragment Shader units.
     *
     *  @param cycle Current simulation cycle.
     */

    void shadeStamps(U64 cycle);

    /**
     *
     *  Shades triangles.  Sends input triangles to the Shader units.
     *
     *  @param cycle Current simulation cycle.
     */

    void shadeTriangles(U64 cycle);

    /**
     *
     *  Shades vertices.  Sends input vertices to the Shader units.
     *
     *  @param cycle Current simulation cycle.
     */

    void shadeVertices(U64 cycle);

    /**
     *
     *  Receives shaded vertices or stamps from the Fragment Shader units.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void receiveShaderOutput(U64 cycle);

    /**
     *
     *  Distributes the shader outputs to the propper shaded (vertex, triangle or fragment) queues.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void distributeShaderOutput(U64 cycle);

    /**
     *
     *  Sends shader inputs to the shader units.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void sendShaderInputs(U64 cycle);

    /**
     *
     *  Interpolates stamps.  Sends stamps to the Interpolator unit from
     *  rasterized stamp queue.
     *
     *  @param cycle Current simulation cycle.
     */

    void startInterpolation(U64 cycle);

    /**
     *
     *  Interpolates stamps.  Sends stamps to the Interpolator unit from
     *  early Z tested stamp queue.
     *
     *  @param cycle Current simulation cycle.
     */

    void startInterpolationEarlyZ(U64 cycle);

    /**
     *
     *  Interpolates stamps.   Receives interpolated stamps from the Interpolator unit.
     *
     *  @param cycle Current simulation cycle.
     */

    void endInterpolation(U64 cycle);

    /**
     *
     *  Send stamps to Z Stencil Test (early Z) from rasterized queue.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void startEarlyZ(U64 cycle);

    /**
     *
     *  Receive stamps from early z test and store them in the early z tested stamp queue.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void endEarlyZ(U64 cycle);

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
     *  Fragment FIFO class constructor.
     *
    *  Creates and initializes a Fragment FIFO mdu.
     *
     *  @param hzStampsCycle Stamps received from Hierarchical Z per cycle.
     *  @param stampsCycle Stamps issued down the pipeline per cycle.
     *  @param unified Simulate an unified shader model.
     *  @param shadedSetup Triangle setup on the shader.
     *  @param trCycle Triangles to be shaded received/sent per cycle from Triangle Setup.
     *  @param trLat Triangle signal from/to Triangle Setup latency.
     *  @param numVShaders Number of Virtual Vertex Shaders supported in the Fragment FIFO (only for unified model).
     *  @param numFShaders Number of Fragment Shaders attached to Fragment FIFO.
     *  @param shInCycle Shader inputs that can be sent per cycle to a shader.
     *  @param shOutCycle Shader outputs that can be received per cycle from a shader.
     *  @param thGroup Size of a shader thread group (inputs grouped to be sent to the shader).
     *  @param fshOutLat Fragment Shader output signal maximum latency.
     *  @param interp Number of attribute interpolators in the Interpolator unit.
     *  @param shInQSz Shader Input queue size (per shader unit).
     *  @param shOutQSz Shader Output queue size (per shader unit).
     *  @param shInBSz Consecutive shader inputs assigned per shader unit.
     *  @param shTileDistro Enable/disable distribution of fragment inputs to the shader units on a tile basis.
     *  @param vInQSz Vertex input queue size (only for unified model).
     *  @param vOutQSz Vertex output queue size (only for unified model).
     *  @param trInQSz Triangle input queue size (for setup on shader).
     *  @param trOutQSz Triangle output queue size (for setup on shader).
     *  @param rastQSz Size of the rasterizer stamp queue (per stamp unit).
     *  @param testQSz Size of the early z stamp queue.
     *  @param intQSz Size of the interpolator stamp queue.
     *  @param shaderQSz Size of the shader stamp queue.
     *  @param vshPrefixes Array of shader prefixes for the virtual vertex shaders (only for unified model).
     *  @param fshPrefixes Array of shader prefixes for the Fragment Shaders attached to Fragment FIFO.
     *  @param nStampUnits Number of stamp units attached to Fragment FIFO.
     *  @param suPrefixes Array of stamp unit prefixes.
     *  @param name Name of the mdu.
     *  @param parent Pointer to the mdu parent mdu.
     *
     *  @return A new initialized Fragment FIFO mdu.
     *
     */

    FragmentFIFO(U32 hzStampsCycle, U32 stampsCycle, bool unified, bool shadedSetup,
        U32 trCycle, U32 trLat, U32 numVShaders, U32 numFShaders, U32 shInCycle, U32 shOutCycle,
        U32 thGroup, U32 fshOutLat, U32 interp, U32 shInQSz, U32 shOutQSz, U32 shInBSz,
        bool shTileDistro, U32 vInQSz, U32 vOutQSz, U32 trInQSz, U32 trOutQSz,
        U32 rastQSz, U32 testQSz, U32 intQSz, U32 shaderQSz,
        char **vshPrefixes, char **fshPrefixes, U32 nStampUnits, char **suPrefixes, char *name, cmoMduBase* parent);

    /**
     *
     *  FragmentFIFO destructor.
     *
     *  Closes files and such.
     *
     */
     
    ~FragmentFIFO();
    
    /**
     *
     *  Fragment FIFO class simulation function.
     *
     *  Simulates a cycle of the Fragment FIFO mdu.
     *
     *  @param cycle The cycle to simulate in the Fragment FIFO mdu.
     *
     */

    void clock(U64 cycle);

    /**
     *
     *  Returns a single line string with state and debug information about the
     *  Fragment FIFO mdu.
     *
     *  @param stateString Reference to a string where to store the state information.
     *
     */

    void getState(string &stateString);
    
    /**
     *
     *  Detects stall conditions in the Fragment FIFO mdu.
     *
     *  @param cycle Current simulation cycle.
     *  @param active Reference to a boolean variable where to return if the stall detection logic is enabled/implemented.
     *  @param stalled Reference to a boolean variable where to return if the cmFragmentFIFO.has been detected to be stalled.
     *
     */
     
    void detectStall(U64 cycle, bool &active, bool &stalled);

    /**
     *
     *  Writes into a string a report about the stall condition of the mdu.
     *
     *  @param cycle Current simulation cycle.
     *  @param stallReport Reference to a string where to store the stall state report for the mdu.
     *
     */
     
    void stallReport(U64 cycle, string &stallReport);
    
};

} // namespace cg1gpu

#endif
