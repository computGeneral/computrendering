/**************************************************************************
 *
 * cmoRasterizer mdu class definition file (unified shaders version).
 *
 */

/**
 *
 *  @file cmRasterizer.h
 *
 *  This file defines the cmoRasterizer mdu (unified shaders version).
 *
 *  The cmoRasterizer mdu controls all the rasterizer boxes:
 *  Triangle Setup, Triangle Traversal, Interpolator and
 *  Fragment FIFO.
 *
 */


#ifndef _RASTERIZER_

#define _RASTERIZER_

#include "GPUType.h"
#include "support.h"
#include "cmMduBase.h"
#include "cmRasterizerCommand.h"
#include "GPUReg.h"
#include "cmRasterizerState.h"
#include "cmTriangleSetup.h"
#include "cmTriangleTraversal.h"
#include "cmHierarchicalZ.h"
#include "cmInterpolator.h"
#include "cmFragmentFIFO.h"

namespace cg1gpu
{

/**
 *
 *  cmoRasterizer mdu class definition.
 *
 *  This class implements a cmoRasterizer mdu that renders
 *  triangles from the vertex calculated in the vertex
 *  shader.
 *
 */

class cmoRasterizer: public cmoMduBase
{
private:

    //  cmoRasterizer signals.  
    Signal *rastCommSignal;     //  Command signal from the Command Processor.  
    Signal *rastStateSignal;    //  cmoRasterizer state signal to the Command Processor.  
    Signal *triangleSetupComm;  //  Command signal to Triangle Setup.  
    Signal *triangleSetupState; //  State signal from Triangle Setup.  
    Signal *triangleTraversalComm;  //  Command signal to Triangle Traversal.  
    Signal *triangleTraversalState; //  State signal from Triangle Traversal.  
    Signal *hzCommand;          //  Command signal to Hierarchical Z.  
    Signal *hzState;            //  State signal from Hierarchical Z.  
    Signal *interpolatorComm;   //  Command signal to Interpolator mdu.  
    Signal *interpolatorState;  //  State signal from Interpolator mdu.  
    Signal *fFIFOCommand;       //  Command signal to the Fragment FIFO mdu.  
    Signal *fFIFOState;         //  State signal from the Fragment FIFO mdu.  

    //  cmoRasterizer boxes.  
    TriangleSetup *triangleSetup;   //  Triangle Setup mdu.  
    TriangleTraversal *triangleTraversal;   //  Triangle Traversal mdu.  
    HierarchicalZ *hierarchicalZ;   //  Hierarchical Z mdu.  
    Interpolator *interpolator;     //  Interpolator mdu.  
    FragmentFIFO *fFIFO;            //  Fragment FIFO mdu.  

    //  cmoRasterizer registers.  
    U32 hRes;                //  Display horizontal resolution.  
    U32 vRes;                //  Display vertical resolution.  
    bool d3d9PixelCoordinates;  //  Use D3D9 pixel coordinates convention -> top left is pixel (0, 0).  
    S32 viewportIniX;            //  Viewport initial (lower left) X coordinate.  
    S32 viewportIniY;            //  Viewport initial (lower left) Y coordinate.  
    U32 viewportHeight;          //  Viewport height.  
    U32 viewportWidth;           //  Viewport width.  
    bool scissorTest;               //  Enables scissor test.  
    S32 scissorIniX;             //  Scissor initial (lower left) X coordinate.  
    S32 scissorIniY;             //  Scissor initial (lower left) Y coordinate.  
    U32 scissorHeight;           //  Scissor height.  
    U32 scissorWidth;            //  Scissor width.  
    F32 nearRange;               //  Near depth range.  
    F32 farRange;                //  Far depth range.  
    bool d3d9DepthRange;            //  Use D3D9 range for depth in clip space [0, 1].  
    F32 slopeFactor;             //  Depth slope factor.  
    F32 unitOffset;              //  Depth unit offset.  
    U32 clearDepth;              //  Clear Z value.  
    U32 zBufferBitPrecission;    //  Z Buffer bit precission.  
    bool frustumClipping;           //  Frustum clipping flag.  
    Vec4FP32 userClip[MAX_USER_CLIP_PLANES];       //  User clip planes.  
    bool userClipPlanes;            //  User clip planes enabled or disabled.  
    FaceMode faceMode;              //  Front face selection mode.  
    CullingMode cullMode;           //  Culling Mode.  
    bool hzEnable;                  //  HZ test enable flag.  
    bool earlyZ;                    //  Early Z test flag (Z/Stencil before shading).  
    bool d3d9RasterizationRules;    //  Use D3D9 rasterization rules.  
    bool twoSidedLighting;          //  Enables two sided lighting color selection.  
    bool multisampling;             //  Flag that stores if MSAA is enabled.  
    U32 msaaSamples;             //  Number of MSAA Z samples generated per fragment when MSAA is enabled.  
    bool interpolation[MAX_FRAGMENT_ATTRIBUTES];        //  Interpolation enabled or disabled (FLAT/SMOTH).  
    bool fragmentAttributes[MAX_FRAGMENT_ATTRIBUTES];   //  Fragment input attributes active flags.  
    bool modifyDepth;               //  Flag storing if the fragment shader modifies the fragment depth component.  
    bool stencilTest;               //  Enable/Disable Stencil test flag.  
    bool depthTest;                 //  Enable/Disable depth test flag.  
    CompareMode depthFunction;      //  Depth test comparation function.  
    bool rtEnable[MAX_RENDER_TARGETS];      //  Render target enable.  
    bool colorMaskR[MAX_RENDER_TARGETS];    //  Update to color buffer Red component enabled.  
    bool colorMaskG[MAX_RENDER_TARGETS];    //  Update to color buffer Green component enabled.  
    bool colorMaskB[MAX_RENDER_TARGETS];    //  Update to color buffer Blue component enabled.  
    bool colorMaskA[MAX_RENDER_TARGETS];    //  Update to color buffer Alpha component enabled.  

    //  cmoRasterizer parameters.  
    bmoRasterizer &bmRaster;    //  Reference to the cmoRasterizer Behavior Model that is going to be used.  
    U32 trianglesCycle;          //  Number of triangles received from Clipper/PA per cycle.  
    U32 triangleSetupFIFOSize;   //  Size of the Triangle Setup FIFO.  
    U32 triangleSetupUnits;      //  Number of triangle setup units.  
    U32 triangleSetupLatency;    //  Latency of the triangle setup units.  
    U32 triangleSetupStartLat;   //  Start latency of the triangle setup units.  
    U32 triangleInputLat;        //  Latency of the triangle bus from PA/Clipper.  
    U32 triangleOutputLat;       //  Latency of the triangle bus between Triangle Setup and Triangle Traversal/Fragment Generation.  
    bool shadedSetup;               //  Triangle setup performed in the unified shader.  
    U32 triangleShQSz;           //  Size of the triangle shade queue in Triangle Setup.  
    U32 stampsCycle;             //  Number of stamps generated/processed per cycle.  
    U32 samplesCycle;            //  Number of depth samples that can be generated per fragment each cycle at Triangle Traversal/Fragment Generation when MSAA is enabled.  
    U32 overW;                   //  Over scan tile width in scan tiles.  
    U32 overH;                   //  Over scan tile height in scan tiles.  
    U32 scanW;                   //  Scan tile width in pixels.  
    U32 scanH;                   //  Scan tile height in pixels.  
    U32 genW;                    //  Generation tile width in pixels.  
    U32 genH;                    //  Generation tile height in pixels.  
    U32 trBatchSize;             //  Size of a rasterization triangle batch (triangle traversal).  
    U32 trBatchQueueSize;        //  Triangles in the batch queue.  
    bool recursiveMode;             //  Determines the rasterization method (T: recursive, F: scanline).  
    bool disableHZ;                 //  Disables Hierarchical Z.  
    U32 blockStamps;             //  Stamps per HZ block.  
    U32 hzBufferSize;            //  Size of the Hierarchical Z Buffer (number of blocks stored).  
    U32 hzCacheLines;            //  Number of lines int the fast access Hierarchical Z Buffer cache.  
    U32 hzCacheLineSize;         //  Blocks per line of the fast access Hierarchical Z Buffer cache.  
    U32 hzQueueSize;             //  Size of the stamp queue for the Early test.  
    U32 hzBufferLatency;         //  Access latency to the HZ Buffer Level 0.  
    U32 hzUpdateLatency;         //  Update signal latency from Z Stencil mdu.  
    U32 clearBlocksCycle;        //  Number of blocks that can be cleared per cycle.  
    U32 interpolators;           //  Number of hardware interpolators.  
    U32 shInputQSz;              //  Shader input queue size (per shader unit).  
    U32 shOutputQSz;             //  Shader output queue size (per shader unit).  
    U32 shInputBatchSz;          //  Number of consecutive shader inputs assigned per shader unit.  
    bool tiledShDistro;             //  Enable/disable distribution of fragment inputs to the shader units on a tile basis.   
    bool unifiedModel;              //  Simulate an unified shader model architecture.  
    U32 numVShaders;             //  Number of fragment shaders attached to the cmoRasterizer.  
    U32 numFShaders;             //  Number of fragment shaders attached to the cmoRasterizer.  
    U32 shInputsCycle;           //  Shader inputs that can be sent to a shader per cycle.  
    U32 shOutputsCycle;          //  Shader outputs that can be received from a shader per cycle.  
    U32 threadGroup;             //  Number of inputs that form a shader thread group.  
    U32 maxFShOutLat;            //  Maximum latency of the shader output signal to the FFragment FIFO.  
    U32 vInputQueueSz;           //  Size of the vertex input queue.  
    U32 vShadedQueueSz;          //  Size of the vertex output queue.  
    U32 trInQueueSz;             //  Triangle input queue size (for triangle setup on shaders). 
    U32 trOutQueueSz;            //  Triangle output queue size (for triangle setup on shaders).  
    U32 rastQueueSz;             //  Size of the generated stamp queue in the Fragment FIFO (per stamp unit).  
    U32 testQueueSz;             //  Size of the early z tested stamp queue in the Fragment FIFO.  
    U32 intQueueSz;              //  Size of the interpolated stamp queue in the Fragment FIFO.  
    U32 shQueueSz;               //  Size of the shaded stamp queue in the Fragment FIFO (per stamp unit).  
    U32 numStampUnits;           //  Number of stamp units attached to the cmoRasterizer.  

    //  cmoRasterizer state.  
    RasterizerState state;      //  Current state of the cmoRasterizer unit.  
    RasterizerCommand *lastRastComm;    //  Last rasterizer command received.  

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
     *  @param subreg The cmoRasterizer register subregister to write to.
     *  @param data The data to write to the register.
     *  @param cycle The current simulation cycle.
     *
     */

    void processRegisterWrite(GPURegister reg, U32 subreg, GPURegData data,
        U64 cycle);

public:

    /**
     *
     *  cmoRasterizer constructor.
     *
     *  This function creates and initializates the cmoRasterizer mdu state and
     *  structures.
     *
     *  @param bmRaster Reference to the cmoRasterizer Behavior Model that will be
     *  used for emulation.
     *  @param trianglesCycle Triangles received from PA/Clipper per cycle.
     *  @param tsFIFOSize Size of the Triangle Setup FIFO.
     *  @param trSetupUnits Number of triangle setup units.
     *  @param trSetupLat Latency of the triangle setup units.
     *  @param trSetupStartLat Start latency of the triangle setup units.
     *  @param trInputLat Latency of the triangle bus from PA/Clipper.
     *  @param trOutputLat Latency of the triangle bus between Triangle Setup and Triangle Traversal/Fragment Generation.
     *  @param shSetup Triangle setup to be performed in the unified shaders.
     *  @param trShQSz Size of the triangle shader queue in Triangle Setup.
     *  @param stampsCycle Stamps generated/processed per cycle.
     *  @param samplesCycle Number of depth samples that can generated per fragment each cycle in Triangle Traversal/Fragment Generation when MSAA is enabled.
     *  @param overW Over scan tile width in scan tiles (may become a register!).
     *  @param overH Over scan tile height in scan tiles (may become a register!).
     *  @param scanW Scan tile width in fragments.
     *  @param scanH Scan tile height in fragments.
     *  @param genW Generation tile width in fragments.
     *  @param genH Generation tile height in fragments.
     *  @param trBatchSize Size of the rasterizer triangle batch.
     *  @param trBatchQueueSize Size of the batch queue in triangles.
     *  @param recursive Flag that determines the rasterization method (T: recursive, F: scanline).
     *  @param disableHZ Disables Hierarchical Z.
     *  @param blockStamps Stamps per HZ block.
     *  @param hzBufferSize Size of the Hierarchical Z Buffer (number of blocks stored).
     *  @param hzCacheLines Lines in the fast access Hierarchical Z Buffer cache.
     *  @param hzCacheLineSize Blocks per line of the fast access Hierarchical Z Buffer cache.
     *  @param hzQueueSize Size of the stamp queue for the Early test.
     *  @param hzBufferLatency Access latency to the HZ Buffer Level 0.
     *  @param hzUpdateLantecy Update signal latency from Z Stencil Test mdu.
     *  @param clearBlocksCycle Number of blocks that can be cleared per cycle.
     *  @param interp  Number of hardware interpolators available.
     *  @param shInQSz Shader Input Queue size (per shader unit).
     *  @param shOutQSz Shader Output Queue size (per shader unit).
     *  @param shInBSz Shader inputs (consecutive) assigned per shader unit.
     *  @param shTileDistro Enable/disable distribution of fragment inputs to the shader units on a tile basis.
     *  @param nVShaders Number of virtual vertex shaders simulated by the rasterizer (unified model only).
     *  @param vshPrefix Array of virtual vertex shader prefixes (unified model only).
     *  @param nFShaders Number of fragment shaders attached to the rasterizer.
     *  @param fshPrefix Array of fragment shader prefixes.
     *  @param shInCycle Shader inputs per cycle and shader.
     *  @param shOutCycle Shader outputs per cycle and shader.
     *  @param thGroup Threads per wave.
     *  @param fshOutLat Latency of the shader output signal.
     *  @param vInQSz Vertex input queue size.
     *  @param vOutQSz Vertex output queue size.
     *  @param trInQSz Triangle input queue size.
     *  @param trOutQSz Triangle output queue size.
     *  @param rastQSz Rasterized stamp queue size (Fragment FIFO) (per stamp unit).
     *  @param testQSz Early Z tested stamp queue size (Fragment FIFO).
     *  @param intQSz Interpolated stamp queue size (Fragment FIFO).
     *  @param shQSz Shaded stamp queue size (Fragment FIFO) (per stamp unit).
     *  @param nStampUnits Number of stamp units attached to the rasterizer.
     *  @param suPrefixes Array of stamp unit prefixes.
     *  @param name  Name of the mdu.
     *  @param parent  Pointer to a parent mdu.
     *
     */

    cmoRasterizer(bmoRasterizer &bmRaster, U32 trianglesCycle, U32 tsFIFOSize,
        U32 trSetupUnits, U32 trSetupLat, U32 trSetupStartLat, U32 trInputLat, U32 trOutputLat,
        bool shSetup, U32 trShQSz, U32 stampsCycle, U32 samplesCycle, U32 overW, U32 overH, U32 scanW, U32 scanH,
        U32 genW, U32 genH, U32 trBatchSize, U32 trBatchQueueSize, bool recursive,
        bool disableHZ, U32 blockStamps, U32 hzBufferSize,
        U32 hzCacheLines, U32 hzCacheLineSize, U32 hzQueueSize, U32 hzBufferLatency,
        U32 hzUpdateLatency, U32 clearBlocksCycle, U32 interp, U32 shInQSz, U32 shOutQSz, U32 shInBSz,
        bool shTileDistro, U32 nVShaders, char **vshPrefix, U32 nFShaders, char **fshPrefix,
        U32 shInCycle, U32 shOutCycle, U32 thGroup, U32 fshOutLat,
        U32 vInQSz, U32 vOutQSz, U32 trInQSz, U32 trOutQSz, U32 rastQSz, U32 testQSz,
        U32 intQSz, U32 shQSz, U32 nStampUnits, char **suPrefixes,
        char *name, cmoMduBase *parent = 0);

    /**
     *
     *  cmoRasterizer destructor.
     *
     */
     
    ~cmoRasterizer();
    
    /**
     *
     *  Performs the simulation and emulation of the rasterizer
     *  cycle by cycle.
     *
     *  @param cycle  The cycle to simulate.
     *
     */

    void clock(U64 cycle);

    /**
     *
     *  Returns a single line string with state and debug information about the
     *  cmoRasterizer mdu.
     *
     *  @param stateString Reference to a string where to store the state information.
     *
     */

    void getState(string &stateString);

    /**
     *
     *  Saves the content of the Hierarchical Z buffer into a file.
     *
     */
     
    void saveHZBuffer();
     
    /**
     *
     *  Loads the content of the Hierarchical Z buffer from a file.
     */
     
    void loadHZBuffer();

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
