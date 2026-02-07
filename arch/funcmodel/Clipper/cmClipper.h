/**************************************************************************
 * cmoClipper mdu definition file.
 */

/**
 *  @file cmClipper.h
 *
 *  Defines the cmoClipper mdu class.
 *  This class simulates the hardware clipper in a GPU.
 */

#ifndef _CLIPPER_

#define _CLIPPER_

#include "GPUType.h"
#include "cmMduBase.h"
#include "GPUReg.h"
#include "cmTriangleSetupInput.h"
#include "cmClipperCommand.h"


namespace cg1gpu
{


/**
 *
 *  Defines the clipper state.
 *
 */
enum ClipperState
{
    CLP_RESET,  //  Reset state.  
    CLP_READY,  //  Ready state.  
    CLP_DRAW,   //  Draw state.  
    CLP_END     //  End state.  
};


/**
 *
 *  Defines the clipper status values sent to Primitive Assembly
 *
 */
enum ClipperStatus
{
    CLIPPER_READY,  //  cmoClipper is ready to receive a new triangle.  
    CLIPPER_BUSY    //  cmoClipper is busy.  
};


/**
 *
 *  This class implements the simulation mdu for the cmoClipper unit in a GPU.
 *
 *  The cmoClipper unit receives triangles (3 vertices and their
 *  attributes) and clips them against the the frustum clip volume
 *  and/or the user clip planes.  The cmoClipper unit can generate
 *  new vertices and triangles.
 *
 *  The current version of the clipper only performs trivial
 *  triangle reject if the triangle is outside the frustum clip
 *  volume.
 *
 *  The cmoClipper class inherits from the cmoMduBase class that provides
 *  basic simulation support.
 *
 */

class cmoClipper : public cmoMduBase
{

private:

    //  cmoClipper signals.  
    Signal *clipperCommand;          //  cmoClipper command signal from the Command Processor.  
    Signal *clipperCommState;        //  cmoClipper state signal to the Command Processor.  
    Signal *clipperInput;            //  cmoClipper triangle input from the Primitive Assembly unit.  
    Signal *clipperRequest;          //  cmoClipper request signal to the Primitive Assembly unit.  
    Signal *rasterizerNewTriangle;   //  New triangle signal to the Rasterizer unit.  
    Signal *rasterizerRequest;       //  Request signal from the Rasterizer unit.  
    Signal *clipStart;               //  Clipping start signal to the cmoClipper unit.  
    Signal *clipEnd;                 //  Clipping end signal from the cmoClipper unit.  

    //  cmoClipper parameters.  
    U32 trianglesCycle;       //  Number of triangles per cycle received from Primitive Assembly  and sent to Rasterizer.  
    U32 clipperUnits;         //  Number of clipper test units.  
    U32 startLatency;         //  cmoClipper start latency.  
    U32 execLatency;          //  cmoClipper execution latency (triangle reject test).  
    U32 clipBufferSize;       //  Size of the buffer for clipped triangles.  
    U32 rasterizerStartLat;   //  Start latency for rasterizer unit.  
    U32 rasterizerOutputLat;  //  Latency of the triangle bus to Rasterizer.  

    //  cmoClipper registers.  
    bool frustumClip;           //  Frustum clipping enable flag.  
    bool d3d9DepthRange;        //  Defines the range for depth dimension in clip space, [0, 1] for D3D9, [-1, 1] for OpenGL.  

    //  cmoClipper state.  
    ClipperState state;                 //  The current clipper state.  
    ClipperCommand *lastClipperCommand; //  Stores the last clipper command.  
    U32 clipCycles;                  //  Cycles remaining until the next triangle can be clipped.  
    U32 rasterizerCycles;            //  Remaining cycles until a triangle can be sent to rasterizer.  
    U32 triangleCount;               //  Number of triangles processed in the current batch.  
    U32 requestedTriangles;          //  Number of vertices requested by Rasterizer.  
    U32 lastTriangleCycles;          //  Stores the number of cycles remaining until the last triangle signal reaches Rasterizer.  

    //  Clipped triangle buffer.  
    TriangleSetupInput **clipBuffer;    //  Buffer for clipped triangles.  
    U32 nextClipTriangle;            //  Next clipped triangle in the buffer.  
    U32 nextFreeEntry;               //  Next free entry in the buffer.  
    U32 clippedTriangles;            //  Number of clipped triangles in the buffer.  
    U32 reservedEntries;             //  Number of reserved (triangles in the clipper pipeline) clip buffer entries.  

    //  Statistics.  
    gpuStatistics::Statistic *inputs;   //  Number of input triangles.  
    gpuStatistics::Statistic *outputs;  //  Number of output triangles.  
    gpuStatistics::Statistic *clipped;  //  Number of clipped triangles.  

    //  Private functions.  

    /**
     *
     *  Processes a clipper command.
     *
     *  @param clipComm The clipper command to process.
     *
     */

    void processCommand(ClipperCommand *clipComm);

    /**
     *
     *  Process register write.
     *
     *  @param reg cmoClipper register to write.
     *  @param subReg cmoClipper subregister to write.
     *  @param data Data to write to the register.
     *
     */

    void processRegisterWrite(GPURegister reg, U32 subReg, GPURegData data);



public:

    /**
     *
     *  cmoClipper mdu constructor.
     *
     *  Creates and initializes a new cmoClipper mdu.
     *
     *  @param trianglesCycle Number of triangles received from Primitive Assembly per cycle and sent to Rasterizer.
     *  @param clipperUnits Number of clipper test units.
     *  @param startLatency Start cmoClipper latency for input triangles.
     *  @param exeLatency Execution latency for frustum clip reject test.
     *  @param bufferSize Size of the buffer for the clipped triangles.
     *  @param rasterizerStartLat Start latency of the rasterizer unit.
     *  @param rasterizerOutputLat Latency of the triangle bus to Rasterizer.
     *  @param parent The cmoClipper parent mdu.
     *  @param name The cmoClipper mdu name.
     *
     *  @return A new initialize cmoClipper mdu object.
     *
     */

    cmoClipper(U32 trianglesCycle, U32 clipperUnits, U32 startLatency, U32 execLatency,
        U32 bufferSize, U32 rasterizerStartLat, U32 rasterizerOutputLat, char *name, cmoMduBase *parent);

    /**
     *
     *  cmoClipper mdu simulation function.
     *
     *  Performs the simulation of a cycle of the cmoClipper unit.
     *
     *  @param cycle The current simulation cycle.
     *
     */

    void clock(U64 cycle);

    /**
     *
     *  Returns a single line string with state and debug information about the
     *  cmoClipper mdu.
     *
     *  @param stateString Reference to a string where to store the state information.
     *
     */

    void getState(std::string &stateString);

};

} // namespace cg1gpu

#endif


