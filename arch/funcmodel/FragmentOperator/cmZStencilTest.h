/**************************************************************************
 *
 * Z Stencil test mdu definition file.
 *
 */

/**
 *
 *  @file cmZStencilTest.h
 *
 *  This file defines the Z and Stencil Test mdu.
 *
 *  This class implements the Z and stencil tests
 *  stages of the GPU pipeline.
 *
 */


#ifndef _ZSTENCILTEST_

#define _ZSTENCILTEST_

#include "GPUType.h"
#include "support.h"
#include "cmMduBase.h"
//#include "Signal.h"
#include "cmZCache.h"
#include "cmRasterizerCommand.h"
#include "cmRasterizerState.h"
#include "GPUReg.h"
#include "cmFragmentInput.h"
#include "bmFragmentOperator.h"
#include "bmRasterizer.h"

namespace cg1gpu
{

/**
 *
 *  Defines a Z Queue entry.
 *
 *  A Z queue entry stores fragments and z values
 *  for a stamp being tested.
 *
 */


struct ZQueue
{
    U32 address;     //  Adddres in the Z buffer where to store the stamp fragments.  
    U32 way;         //  Way of the color cache where to store the stamp.  
    U32 line;        //  Line of the color cache where to store the stamp.  
    U32 offset;      //  Offset inside the color cache line where to store the stamp.  
    bool lastStamp;     //  Stores if the stored stamp is marked as last stamp.  
    U32 *inDepth;    //  Pointer to an unsigned integer array where to store the stamp Z values.  
    U32 *zData;      //  Z and stencil data for the stamp for/from the Z buffer.  
    bool *culled;       //  Pointer to a boolean array where to store the stamp culled bits.  
    FragmentInput **stamp;      //  Pointer to the fragments in the stamp.  
};


/**
 *
 *  Defines the state reported by the Z Stencil Test unit to the Fragment FIFO.
 *
 */

enum ZStencilTestState
{
    ZST_READY,      //  Z Stencil test can receive stamps.  
    ZST_BUSY        //  Z Stencil test can not receive stamps.  
};

/**
 *
 *  This class implements the simulation of the Z and Stencil
 *  test stages of a GPU pipeline.
 *
 *  This class inherits from the cmoMduBase class that offers basic
 *  simulation support.
 *
 */

class ZStencilTest : public cmoMduBase
{
private:

    //  Defines where the tested fragments must be sent.  
    enum
    {
        COLOR_WRITE = 0,        //  Early Z disabled, fragments are sent to Color Write.  
        FRAGMENT_FIFO = 1       //  Early Z enabled, fragments are sent to Fragment FIFO.  
    };

    //  Z Stencil Test signals.  
    Signal *rastCommand;        //  Command signal from the Rasterizer main mdu.  
    Signal *rastState;          //  State signal to the Rasterizer main mdu.  
    Signal *fragments;          //  Fragment signal from the Fragment FIFO mdu.  
    Signal *zStencilState;      //  State signal to the Fragment FIFO mdu.  
    Signal *colorWrite[2];      //  Stamp output signal to the Color Write or Fragment FIFO mdu.  
    Signal *colorWriteState[2]; //  State signal from the Color Write or Fragment FIFO mdu.  
    Signal *memRequest;         //  Memory request signal to the Memory Controller.  
    Signal *memData;            //  Memory data signal from the Memory Controller.  
    Signal *testStart;          //  Signal that simulates the start of the test operation latency.  
    Signal *testEnd;            //  Signal that simulates the end of the test operation.  
    Signal *hzUpdate;           //  Update signal to the Hierarchical Z mdu.  

    //  Z Stencil test registers.  
    U32 hRes;                //  Display horizontal resolution.  
    U32 vRes;                //  Display vertical resolution.  
    S32 startX;              //  Viewport initial x coordinate.  
    S32 startY;              //  Viewport initial y coordinate.  
    U32 width;               //  Viewport width.  
    U32 height;              //  Viewport height.  
    U32 clearDepth;          //  Current clear depth value.  
    U32 depthPrecission;     //  Depth bit precission.  
    U08 clearStencil;         //  Current clear stencil value.  
    bool earlyZ;                //  Flag that enables or disables early Z testing (Z Stencil before shading).  
    bool modifyDepth;           //  Flag that stores if the fragment shader has modified the fragment depth component.  
    bool zTest;                         //  Flag storing if Z test is enabled.  
    CompareMode depthFunction;          //  Depth test comparation function.  
    bool depthMask;                     //  If depth buffer update is enabled or disabled.  
    bool stencilTest;                   //  Flag storing if Stencil test is enabled.  
    CompareMode stencilFunction;        //  Stencil test comparation function.  
    U08 stencilReference;             //  Stencil reference value for the test.  
    U08 stencilTestMask;              //  Stencil mask for the stencil operands test.  
    U08 stencilUpdateMask;            //  Stencil mask for the stencil update.  
    StencilUpdateFunction stencilFail;  //  Update function when stencil test fails.  
    StencilUpdateFunction depthFail;    //  Update function when depth test fails.  
    StencilUpdateFunction depthPass;    //  Update function when depth test passes.  
    U32 zBuffer;             //  Address in the GPU memory of the current Z buffer.  

    //  Color Write parameters.  
    U32 stampsCycle;         //  Number of stamps that can be received per cycle.  
    U32 overW;               //  Over scan tile width in scan tiles.  
    U32 overH;               //  Over scan tile height in scan tiles.  
    U32 scanW;               //  Scan tile width in generation tiles.  
    U32 scanH;               //  Scan tile height in generation tiles.  
    U32 genW;                //  Generation tile width in stamps.  
    U32 genH;                //  Generation tile height in stamps.  
    U32 bytesPixel;          //  Depth bytes per pixel.  
    U32 zCacheWays;          //  Z cache set associativity.  
    U32 zCacheLines;         //  Number of lines in the Z cache per way/way.  
    U32 stampsLine;          //  Number of stamps per Z cache line.  
    U32 cachePortWidth;      //  Width of the Z cache ports in bytes.  
    bool extraReadPort;         //  Add an additional read port to the color cache.  
    bool extraWritePort;        //  Add an additional write port to the color cache.  
    U32 zQueueSize;          //  Z queue size.  
    bool disableZCompression;   //  Disables Z compression (and HZ update!).  
    U32 testRate;            //  Rate at which stamp are tested (cycles between two stamp to be tested).  
    U32 testLatency;         //  Z and Stencil test latency.  
    bool disableHZUpdate;       //  Disables HZ Update.  
    U32 hzUpdateLatency;     //  Latency of the hierarchizal Z update signal.  
    bmoFragmentOperator &frEmu;  //  Reference to the fragment operation behaviorModel object.  
    bmoRasterizer &bmRaster;//  Reference to the rasterizer behaviorModel objecter.  

    //  Z Write state.  
    RasterizerState state;      //  Current mdu state.  
    RasterizerCommand *lastRSCommand;   //  Stores the last Rasterizer Command received (for signal tracing).  
    U32 currentTriangle;     //  Identifier of the current triangle being processed (used to count triangles).  
    U32 triangleCounter;     //  Number of processed triangles.  
    U32 fragmentCounter;     //  Number of fragments processed in the current batch.  
    U32 frameCounter;        //  Counts the number of rendered frames.  
    bool lastFragment;          //  Last batch fragment flag.  
    bool receivedFragment;      //  If a fragment has been received in the current cycle.  
    MemState memoryState;       //  Current memory controller state.  
    bool endFlush;              //  Flag that signals the end of the flush of the Z cache.  
    U32 fragmentDestination; //  Stores which is the current destination of stamps (FragmentFIFO or Color Write).  
    U32 testCycles;          //  Cycles until the next stamp can be tested.  

    //  Z cache.  
    ZCache *zCache;             //  Pointer to the Z cache.  
    U32 bytesLine;           //  Bytes per Z cache line.  
    U32 lineShift;           //  Logical shift for a Z cache line.  
    U32 lineMask;            //  Logical mask for a Z cache line.  
    U32 stampMask;           //  Logical mask for the offset of a stamp inside a Z cache line.  

    //  Z Queue structures.  
    ZQueue *zQueue;         //  The blend queue.  Stores stamps to be blended or written.  
    U32 fetchZ;          //  Entry of the Z queue for which to fetch Z and stencil data.  
    U32 readZ;           //  Entry of the Z queue for which to read Z data.  
    U32 writeZ;          //  Entry of the Z queue for which to write Z and stencil data.  
    U32 colorZ;          //  Entry of the Z queue to be sent next to Color Write.  
    U32 freeZ;           //  Next free entry in the Z queue.  
    U32 testStamps;      //  Stamps stored in the Z queue.  
    U32 fetchStamps;     //  Stamps waiting in the Z queue to fetch Z data.  
    U32 readStamps;      //  Stamps waiting in the Z queue to read Z data.  
    U32 writeStamps;     //  Stamps waiting in the Z queue to write Z data.  
    U32 colorStamps;     //  Stamps waiting in the Z queue to be sent to Color Write.  
    U32 freeStamps;      //  Free entries in the Z queue.  

    //  Configurable buffers.  
    FragmentInput **stamp;      //  Stores last receveived stamp.  

    //  Statistics.  
    gpuStatistics::Statistic *inputs;       //  Input fragments.  
    gpuStatistics::Statistic *outputs;      //  Output fragments.  
    gpuStatistics::Statistic *tested;       //  Tested fragments.  
    gpuStatistics::Statistic *failed;       //  Fragments that failed the tests.  
    gpuStatistics::Statistic *passed;       //  Fragments that passed the tests.  
    gpuStatistics::Statistic *outside;      //  Outside triangle/viewport fragments.  
    gpuStatistics::Statistic *culled;       //  Culled for non test related reasons.  
    gpuStatistics::Statistic *readTrans;    //  Read transactions to memory.  
    gpuStatistics::Statistic *writeTrans;   //  Write transactions to memory.  
    gpuStatistics::Statistic *readBytes;    //  Bytes read from memory.  
    gpuStatistics::Statistic *writeBytes;   //  Bytes written to memory.  
    gpuStatistics::Statistic *fetchOK;      //  Succesful fetch operations.  
    gpuStatistics::Statistic *fetchFail;    //  Failed fetch operations.  
    gpuStatistics::Statistic *readOK;       //  Sucessful read operations.  
    gpuStatistics::Statistic *readFail;     //  Failed read operations.  
    gpuStatistics::Statistic *writeOK;      //  Sucessful write operations.  
    gpuStatistics::Statistic *writeFail;    //  Failed write operations.  
    gpuStatistics::Statistic *updatesHZ;    //  Updates to Hierarchical Z.  
    gpuStatistics::Statistic *rawDep;       //  Blocked read accesses because of read after write dependence between stamps.  

    //  Z and stencil Clear state.  
    //U08 clearBuffer[MAX_TRANSACTION_SIZE];    //  Clear buffer, stores the clear values for a full transaction.  
    //U32 clearAddress;        //  Current color clear address.  
    //U32 endClearAddress;     //  End of the memory region to clear.  
    //U32 busCycles;           //  Remaining memory bus cycles.  
    //U32 ticket;              //  Memory ticket.  
    //U32 freeTickets;         //  Number of free memory tickets.  

    //  Private functions.  

    /**
     *
     *  Processes a rasterizer command.
     *
     *  @param command The rasterizer command to process.
     *  @param cycle Current simulation cycle.
     *
     */

    void processCommand(RasterizerCommand *command, U64 cycle);

    /**
     *
     *  Processes a register write.
     *
     *  @param reg The Interpolator register to write.
     *  @param subreg The register subregister to writo to.
     *  @param data The data to write to the register.
     *
     */

    void processRegisterWrite(GPURegister reg, U32 subreg, GPURegData data);

    /**
     *
     *  Processes a received stamp.
     *
     */

    void processStamp();

    /**
     *
     *  Processes a memory transaction.
     *
     *  @param cycle The current simulation cycle.
     *  @param memTrans The memory transaction to process
     *
     */

    void processMemoryTransaction(U64 cycle, MemoryTransaction *memTrans);


public:

    /**
     *
     *  Z Stencil Test mdu constructor.
     *
     *  Creates and initializes a new Z Stencil Test mdu object.
     *
     *  @param stampsCycle Number of stamps per cycle.
     *  @param overW Over scan tile width in scan tiles (may become a register!).
     *  @param overH Over scan tile height in scan tiles (may become a register!).
     *  @param scanW Scan tile width in fragments.
     *  @param scanH Scan tile height in fragments.
     *  @param genW Generation tile width in fragments.
     *  @param genH Generation tile height in fragments.
     *  @param bytesPixel Number of bytes to be used per pixel (should be a register!).
     *  @param disableZCompr Disables Z compression (and HZ update!).
     *  @param cacheWays Z cache set associativity.
     *  @param cacheLines Number of lines in the Z cache per way/way.
     *  @param stampsLine Numer of stamps per Z cache line (less than a tile!).
     *  @param portWidth Width of the Z cache ports in bytes.
     *  @param extraReadPort Adds an extra read port to the color cache.
     *  @param extraWritePort Adds an extra write port to the color cache.
     *  @param cacheReqQueueSize Size of the Z cache memory request queue.
     *  @param inputRequests Number of read requests and input buffers in the Z cache.
     *  @param outputRequests Number of write requests and output buffers in the Z cache.
     *  @param maxBlocks Maximum number of sopported Z blocks in the Z cache state memory.
     *  @param blocksCycle Number of state block entries that can be cleared per cycle.
     *  @param compCycles Z block compression cycles.
     *  @param decompCycles Z block decompression cycles.
     *  @param zQueueSize Z queue size (in entries/stamps).
     *  @param zTestRate Rate at which stamp are tested (cycles between two stamps to be tested).
     *  @param zTestLatency Z and stencil test latency.
     *  @param disableHZUpdate Disables HZ update.
     *  @param hzUpdateLatency Latency to send updates to Hierarchical Z buffer.
     *  @param frEmu Reference to the Fragment Operation Behavior Model object.
     *  @param bmRaster Reference to the Rasterizer Behavior Model object to be used by the mdu.
     *  @param name The mdu name.
     *  @param prefix String used to prefix the mdu signals names.
     *  @param parent The mdu parent mdu.
     *
     *  @return A new Z Stencil Test object.
     *
     */

    ZStencilTest(U32 stampsCycle, U32 overW, U32 overH, U32 scanW, U32 scanH,
        U32 genW, U32 genH, U32 bytesPixel,
        bool disableZCompr, U32 cacheWays, U32 cacheLines, U32 stampsLine, U32 portWidth,
        bool extraReadPort, bool extraWritePort, U32 cacheReqQueueSize, U32 inputRequests,
        U32 outputRequests, U32 maxBlocks, U32 blocksCycle,
        U32 compCycles, U32 decompCycles,
        U32 zQueueSize, U32 zTestRate, U32 zTestLatency, bool disableHZUpdate, U32 hzUpdateLatency,
        bmoFragmentOperator &frOp, bmoRasterizer &bmRaster,
        char *name, char *prefix = 0, cmoMduBase* parent = 0);

    /**
     *
     *  Z Stencil Test simulation function.
     *
     *  Simulates a cycle of the Z Stencil Test mdu.
     *
     *  @param cycle The cycle to simulate of the Z Stencil Test mdu.
     *
     */

    void clock(U64 cycle);

    /**
     *
     *  Returns a single line string with state and debug information about the
     *  Z Stencil Test mdu.
     *
     *  @param stateString Reference to a string where to store the state information.
     *
     */

    void getState(string &stateString);
};

} // namespace cg1gpu

#endif

