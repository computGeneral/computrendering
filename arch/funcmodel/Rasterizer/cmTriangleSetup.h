/**************************************************************************
 *
 * Triangle Setup mdu definition file.
 *
 */


/**
 *
 *  @file cmTriangleSetup.h
 *
 *  This file defines the Triangle Setup mdu.
 *
 *
 */

#include "GPUType.h"
#include "cmMduBase.h"
#include "GPUReg.h"
#include "cmRasterizer.h"
#include "cmRasterizerCommand.h"
#include "bmRasterizer.h"
#include "cmTriangleSetupInput.h"
#include "cmTriangleSetupOutput.h"
#include <string>

#ifndef _TRIANGLESETUP_

#define _TRIANGLESETUP_

namespace arch
{

//*  Defines the different Triangle Setup states.  
enum TriangleSetupState
{
    TS_READY,
    TS_FULL,
};

/**
 *
 *  This class implements the Triangle Setup mdu.
 *
 *  This class implements the Triangle Setup mdu that
 *  simulates the Triangle Setup unit of a GPU.
 *
 *  Inherits from the cmoMduBase class that offers basic simulation
 *  support.
 *
 */

class TriangleSetup : public cmoMduBase
{

private:

    //  Triangle Setup parameters.  
    bmoRasterizer &bmRaster;       //  Reference to the rasterizer behaviorModel used by Triangle Setup.  
    U32 trianglesCycle;             //  Number of triangles per cycle received and sent.  
    U32 setupFIFOSize;              //  Size (triangles) of the setup FIFO.  
    U32 numSetupUnits;              //  Number of setup unit.  
    U32 setupLatency;               //  Latency in cycles of a setup unit.  
    U32 setupStartLatency;          //  Start latency in cycles of a setup unit (inverse of througput).  
    U32 paLatency;                  //  Latency of the triangle bus from primite assembly/clipper.  
    U32 tbLatency;                  //  Latency of the triangle bus from Triangle Bound unit.  
    U32 fgLatency;                  //  Latency of the triangle bus to fragment generation.  
    bool   shaderSetup;                //  Perform triangle setup in the unified shader.  
    bool   preTriangleBound;           //  A previous Triangle Bound Unit early computes incoming triangles BBs.  
    U32 triangleShaderQSz;          //  Size of the reorder queue for the triangles being shaded.  

    //  Triangle Setup signals.  
    Signal *setupCommand;       //  Command signal from the Rasterizer main mdu.  
    Signal *rastSetupState;     //  State signal to the Rasterizer main mdu.  
    Signal *inputTriangle;      //  New triangle signal from Primitive Assembly.  
    Signal *triangleRequest;    //  Request signal to the Clipper/Triangle Bound unit.  
    Signal *setupOutput;        //  Setup output signal to the Triangle Traversal/Fragment Generator.  
    Signal *setupRequest;       //  Request signal from the Triangle Traversal/Fragment Generator.  
    Signal *setupStart;         //  Setup Triangle start signal.  Simulates the cost setting up a triangle.  
    Signal *setupEnd;           //  Setup Triangle end signal.  End of the setup of a triangle.  
    Signal *setupShaderInput;   //  Triangle input signal to the unified shaders (FragmentFIFO).  
    Signal *setupShaderOutput;  //  Triangle output signal from the unified shaders (FragmentFIFO).  
    Signal *setupShaderState;   //  Unified shader (FragmentFIFO) state signal.  

    //  Triangle Setup registers.  
    U32 hRes;                //  Display horizontal resolution.  
    U32 vRes;                //  Display vertical resolution.  
    bool d3d9PixelCoordinates;  //  Use D3D9 pixel coordinates convention -> top left is pixel (0, 0).  
    U32 iniX;                //  Viewport initial x coordinate.  
    U32 iniY;                //  Viewport initial y coordinate.  
    U32 width;               //  Viewport width.  
    U32 height;              //  Viewport height.  
    bool scissorTest;           //  Enables scissor test.  
    S32 scissorIniX;         //  Scissor initial x coordinate.  
    S32 scissorIniY;         //  Scissor initial y coordinate.  
    U32 scissorWidth;        //  Scissor width.  
    U32 scissorHeight;       //  Scissor height.  
    F32 nearDepth;                //  Near depth range.  
    F32 farDepth;                 //  Far depth range.  
    bool d3d9DepthRange;        //  Use D3D9 range for depth in clip space [0, 1].  
    F32 slopeFactor;         //  Depth slope factor.  
    F32 unitOffset;          //  Depth unit offset.  
    U32 zBitPrecission;      //  Z buffer bit precission.  
    FaceMode faceMode;          //  Front face selection mode.  
    bool d3d9RasterizationRules;//  Use D3D9 rasterization rules.  
    bool twoSidedLighting;      //  Enables two sided lighting color selection.  
    CullingMode culling;        //  The current face culling mode.  

    F64 minTriangleArea;     //  The minimum area that a triangle must have to be rasterized (pixel area?).  

    //  Triangle Setup state.  
    RasterizerState state;              //  Current triangle setup working state.  
    RasterizerCommand *lastRSCommand;   //  Stores the last Rasterizer Command received (for signal tracing).  
    U32 setupWait;                   //  Number of cycles remaining until the next triangle can start setup.  
    U32 triangleCounter;             //  Number of processed triangles.  
    TriangleSetupOutput **setupFIFO;    //  Setup triangles FIFO.  Used to keep setup triangles before triangle traversal.  
    U32 setupTriangles;              //  Number of triangle stored in the setup FIFO.  
    U32 setupReserves;               //  Number of setup FIFO reserved entries.  
    U32 firstSetupTriangle;          //  First setup triangle stored in the setup FIFO.  
    U32 nextFreeSTFIFO;              //  Next free entry in the setup FIFO.  
    U32 trianglesRequested;          //  Number of setup triangle that have been requested by Triangle Traversal.  
    bool lastTriangle;                  //  Last triangle received from the Clipper/Triangle Bound unit.  

    //  Triangle queue for setup in the unified shaders.  

    /**
     *
     *  Defines an entry for the reorder queue for the triangles sent to the shaders.
     *
     */

    struct TriangleShaderInput
    {
        TriangleSetupInput *tsInput;    //  Triangle setup input waiting to be shaded.  
        bool shaded;                    //  Flag storing the the triangle setup result has been received.  
        Vec4FP32 A;                    //  Stores the triangle A equation coefficient attribute.  
        Vec4FP32 B;                    //  Stores the triangle B equation coefficient attribute.  
        Vec4FP32 C;                    //  Stores the triangle C equation coefficient attribute.  
        F32  tArea;                  //  Stores the triangle estimated 'area' attribute.  
    };

    TriangleShaderInput *triangleShaderQueue;   //  Stores triangles waiting to be processed in the shader.  
    U32 nextSendShTriangle;                  //  Next triangle to be sent to the shaders.  
    U32 nextShaderTriangle;                  //  Next shaded triangle to commit.  
    U32 nextFreeShaderTriangle;              //  Next free entry in the triangle shader queue.  
    U32 sendShTriangles;                     //  Triangles waiting to be sent to the shaders.  
    U32 shaderTriangles;                     //  Triangles waiting for shader result.  
    U32 freeShTriangles;                     //  Free entries in the triangle shader queue.  
    U32 shTriangleReserves;                  //  Number of reserved entries in the triangle shader queue.  

    //  Statistics.  
    gpuStatistics::Statistic *inputs;   //  Number of input triangles.  
    gpuStatistics::Statistic *outputs;  //  Number of outputs triangles.  
    gpuStatistics::Statistic *requests; //  Triangle requests from Fragment Generation.  
    gpuStatistics::Statistic *culled;   //  Number of culled triangles.  
    gpuStatistics::Statistic *front;    //  Number of front facing triangles.  
    gpuStatistics::Statistic *back;     //  Number of back facing triangles.  

    //   Triangle size related statistics.  
    gpuStatistics::Statistic *count_less_1;         //  Non-culled triangles of less than 1 pixel size.   
    gpuStatistics::Statistic *count_1_to_4;         //  Non-culled triangles between 1 and less than 4 pixels size.  
    gpuStatistics::Statistic *count_4_to_16;        //  Non-culled triangles between 4 and less than 16 pixels size.  
    gpuStatistics::Statistic *count_16_to_100;      //  Non-culled triangles between 16 and less than 100 pixels size.  
    gpuStatistics::Statistic *count_100_to_1000;    //  Non-culled triangles between 100 and less than 1000  pixels size.  
    gpuStatistics::Statistic *count_1000_to_screen; //  Non-culled triangles between 1000 and less than screen pixels size.  
    gpuStatistics::Statistic *count_greater_screen; //  Non-culled triangles greater than screen pixels size.  
    gpuStatistics::Statistic *count_overlap_1x1;    //  Non-culled triangles overlapping just one screen pixel.  
    gpuStatistics::Statistic *count_overlap_2x2;    //  Non-culled triangles overlapping just 2x2 screen pixels.   

    //  Private functions.  

    /**
     *
     *  Processes a rasterizer command.
     *
     *  @param command The rasterizer command to process.
     *
     */

    void processCommand(RasterizerCommand *command);

    /**
     *
     *  Processes a register write.
     *
     *  @param reg The Triangle Setup register to write.
     *  @param subreg The register subregister to writo to.
     *  @param data The data to write to the register.
     *
     */

    void processRegisterWrite(GPURegister reg, U32 subreg, GPURegData data);

    /**
     *
     *  Performs the cull face test for a triangle.
     *
     *  @param triangleID Identifier of the the triangle (index?).
     *  @param setupID Identifier of the setup triangle.
     *  @param tArea Reference to a 64 bit float point variable where
     *  to store the calculated are approximation of the triangle.
     *
     *  @return If the triangle must be dropped after the cull face test.
     *  TRUE means that it must be dropped, FALSE that it must not be dropped.
     *
     */

    bool cullFaceTest(U32 triangleID, U32 setupID, F64 &tArea);

    /**
     *
     *  Sends triangles to triangle traversal/fragment generation.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void sendTriangles(U64 cycle);

    /**
     *
     *  Processes a setup triangle.
     *
     *  Performes face and area culling.  Inverts triangle edge and z equations as
     *  required.  Selects color attribute for two faced lighting.
     *
     *  @param tsInput Pointer to the triangle that has been setup.
     *  @param triangleID Identifier of the setup triangle in the Rasterizer Behavior Model.
     *
     */

    void processSetupTriangle(TriangleSetupInput *tsInput, U32 triangleID);

public:

    /**
     *
     *  Triangle Setup constructor.
     *
     *  Creates and initializes a new Triangle Setup mdu.
     *
     *  @param bmRaster Reference to the rasterizer behaviorModel to be used by the Triangle Setup mdu.
     *  @param trianglesCycle Number of triangles recieved or sent to Fragment Generation per
     *  cycle.
     *  @param setupFIFOSize Size of the setup FIFO.
     *  @param setupUnits Number of setup units in the mdu.  Each setup unit can start the setup
     *  of a triangle in paralel.
     *  @param latency Latency of a setup unit.
     *  @param startLatency Start latency of a setup unit.
     *  @param paLat Latency of the bus from Primitive Assembly/Clipper.
     *  @param tbLat Latency of the bus from Triangle Bound unit.
     *  @param fgLat Latency of the bus to Primitive Assembly.
     *  @param shaderSetup Perform triangle setup in the unified shaders.
     *  @pamra preTriangleBound A previous Triangle Bound Unit early computes incoming triangles BBs.
     *  front of Triangle Setup.
     *  @param microTriBypass Bypass of microtriangles is enabled (MicroPolygon rasterizer only).
     *  @param triShQSz Size of the triangle shader result reorder queue.
     *  @param name cmoMduBase name.
     *  @param parent cmoMduBase parent mdu.
     *
     *  @return A new initialized Triangle Setup mdu.
     *
     */

    TriangleSetup(bmoRasterizer &bmRaster, U32 trianglesCycle, U32 setupFIFOSize,
        U32 setupUnits, U32 latency, U32 startLatency, U32 paLat, U32 tbLat, 
        U32 fgLat, bool shaderSetup, bool preTriangleBound, bool microTriBypass, 
        U32 triShQSz, char *name, cmoMduBase *parent);

    /**
     *
     *  Triangle Setup simulation function.
     *
     *  Simulates one cycle of the Triangle Setup GPU unit.
     *
     *  @param cycle The cycle to simulate.
     *
     */

    void clock(U64 cycle);

    /**
     *
     *  Returns a single line string with state and debug information about the
     *  Triangle Setup mdu.
     *
     *  @param stateString Reference to a string where to store the state information.
     *
     */

    void getState(std::string &stateString);

};

} // namespace arch

#endif
