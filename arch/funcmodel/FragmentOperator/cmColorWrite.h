/**************************************************************************
 *
 * Color Write mdu definition file.
 *
 */

/**
 *
 *  @file cmColorWrite.h
 *
 *  This file defines the Color Write mdu.
 *
 *  This class implements the final color blend and color write
 *  stages of the GPU pipeline.
 *
 */


#ifndef _COLORWRITE_

#define _COLORWRITE_

#include "GPUType.h"
#include "support.h"
#include "cmMduBase.h"
#include "Signal.h"
#include "cmColorCache.h"
#include "cmRasterizerCommand.h"
#include "cmRasterizerState.h"
#include "GPUReg.h"
#include "cmFragmentInput.h"
#include "bmFragmentOperator.h"

namespace arch
{

/**
 *
 *  Defines a Blend Queue entry.
 *
 *  A blend queue entry stores the colors of an incoming stamp
 *  of fragments (2x2) while waiting for the color of the
 *  pixels to be written (blended) by those fragments to be
 *  read from the color buffer (from the color cache or video
 *  memory).
 *
 *  NOTE:  The number of bytes per color should change
 *  to support other color buffer formats (float point?).
 *
 */


struct BlendQueue
{
    U32 x;           //  First stamp fragment x coordinate.  
    U32 y;           //  First stamp fragment y coordinate.  
    U64 startCycle;  //  Stores the stamp start cycle (same for all fragments).  
    U32 shaderLatency;   //  Latency of the samp in the shader units (same for all fragments).  
    U32 address;     //  Adddres in the color buffer where to store the stamp fragments.  
    U32 way;         //  Way of the color cache where to store the stamp.  
    U32 line;        //  Line of the color cache where to store the stamp.  
    U32 offset;      //  Offset inside the color cache line where to store the stamp.  
    Vec4FP32 *inColor; //  Pointer to a Vec4FP32 array where to store the stamp colors.  
    U08 *outColor;    //  Pointer to a byte array for the final stamp colors.  
    Vec4FP32 *destColorF;  //  Pointer to a Vec4FP32 array with the destination colors.  
    U08 *destColor;   //  Pointer to a byte array where to store the destination color.  
    bool *mask;         //  Stores the write mask for the stamp. 
    DynamicObject *stampCookies;    //  Stores the stamp cookies.  
};

/**
 *
 *  Defines the fragment map modes.
 *
 */
enum FragmentMapMode
{
    COLOR_MAP = 0,
    OVERDRAW_MAP = 1,
    FRAGMENT_LATENCY_MAP = 2,
    SHADER_LATENCY_MAP = 3
};

/**
 *
 *  Defines the state reported by the color write unit to the Z test mdu.
 *
 */

enum ColorWriteState
{
    CWS_READY,      //  Color Write can receive stamps.  
    CWS_BUSY        //  Color Write can not receive stamps.  
};

/**
 *
 *  This class implements the simulation of the Color Blend,
 *  Logical Operation and Color Write stages of a GPU pipeline.
 *
 *  This class inherits from the cmoMduBase class that offers basic
 *  simulation support.
 *
 */

class ColorWrite : public cmoMduBase
{
private:

    //  Defines from where the fragments are received.  
    enum
    {
        Z_STENCIL_TEST = 0,     //  Early Z disabled, fragments come from Z Stencil Test.  
        FRAGMENT_FIFO = 1       //  Early Z enabled, fragments come from Fragment FIFO.  
    };

    //  Color Write signals.  
    Signal *rastCommand;        //  Command signal from the Rasterizer main mdu.  
    Signal *rastState;          //  State signal to the Rasterizer main mdu.  
    Signal *fragments[2];       //  Fragment signal from Z Stencil Test and Fragment FIFO.  
    Signal *colorWriteState[2]; //  State signal to Z Stencil Test and Fragment FIFO.  
    Signal *memRequest;         //  Memory request signal to the Memory Controller.  
    Signal *memData;            //  Memory data signal from the Memory Controller.  
    Signal *blendStart;         //  Signal that simulates the start of the blend operation latency.  
    Signal *blendEnd;           //  Signal that simulates the end of the blend operation.  
    Signal *blockStateDAC;      //  Signal to send the color buffer block state to the DAC.  

    //  Color Write registers.  
    U32 hRes;                //  Display horizontal resolution.  
    U32 vRes;                //  Display vertical resolution.  
    S32 startX;              //  Viewport initial x coordinate.  
    S32 startY;              //  Viewport initial y coordinate.  
    U32 width;               //  Viewport width.  
    U32 height;              //  Viewport height.  
    U32 clearColor;          //  Current clear color for the framebuffer.  
    bool earlyZ;                //  Flag that enables or disables early Z testing (Z Stencil before shading).  
    bool blend;                 //  Flag storing if color blending is active.  
    BlendEquation equation;     //  Current blending equation mode.  
    BlendFunction srcRGB;       //  Source RGB weight funtion.  
    BlendFunction dstRGB;       //  Destination RGB weight function.  
    BlendFunction srcAlpha;     //  Source alpha weight function.  
    BlendFunction dstAlpha;     //  Destination alpha weight function.  
    Vec4FP32 constantColor;    //  Blend constant color.  
    bool writeR;                //  Write mask for red color component.  
    bool writeG;                //  Write mask for green color component.  
    bool writeB;                //  Write mask for blue color component.  
    bool writeA;                //  Write mask for alpha color channel.  
    bool logicOperation;        //  Flag storing if logical operation is active.  
    LogicOpMode logicOpMode;    //  Current logic operation mode.  
    U32 frontBuffer;         //  Address in the GPU memory of the front (primary) buffer.  
    U32 backBuffer;          //  Address in the GPU memory of the back (secondary) buffer.  

    //  Color Write parameters.  
    U32 stampsCycle;         //  Number of stamps that can be received per cycle.  
    U32 overW;               //  Over scan tile width in scan tiles.  
    U32 overH;               //  Over scan tile height in scan tiles.  
    U32 scanW;               //  Scan tile width in generation tiles.  
    U32 scanH;               //  Scan tile height in generation tiles.  
    U32 genW;                //  Generation tile width in stamps.  
    U32 genH;                //  Generation tile height in stamps.  
    U32 bytesPixel;          //  Bytes per pixel.  
    bool disableColorCompr;     //  Disables color compression.  
    U32 colorCacheWays;      //  Color cache set associativity.  
    U32 colorCacheLines;     //  Number of lines in the color cache per way/way.  
    U32 stampsLine;          //  Number of stamps per color cache line.  
    U32 cachePortWidth;      //  Width of the color cache ports in bytes.  
    bool extraReadPort;         //  Add an additional read port to the color cache.  
    bool extraWritePort;        //  Add an additional write port to the color cache.  
    U32 blocksCycle;         //  State of color block copied to the DAC per cycle.  
    U32 blendQueueSize;      //  Blend queue size.  
    U32 blendRate;           //  Rate at which blending is performed on stamps (cycles between stamps).  
    U32 blendLatency;        //  Blending latency.  
    U32 blockStateLatency;   //  Latency of the block state update signal to the DAC.  
    U32 fragmentMapMode;     //  Fragment map mode.  
    bmoFragmentOperator &frEmu;  //  Reference to the fragment operation behaviorModel object.  

    //  Color Write state.  
    RasterizerState state;      //  Current mdu state.  
    RasterizerCommand *lastRSCommand;   //  Stores the last Rasterizer Command received (for signal tracing).  
    U32 currentTriangle;     //  Identifier of the current triangle being processed (used to count triangles).  
    U32 triangleCounter;     //  Number of processed triangles.  
    U32 fragmentCounter;     //  Number of fragments processed in the current batch.  
    U32 frameCounter;        //  Counts the number of rendered frames.  
    bool lastFragment;          //  Last batch fragment flag.  
    bool receivedFragment;      //  If a fragment has been received in the current cycle.  
    MemState memoryState;       //  Current memory controller state.  
    bool endFlush;              //  Flag that signals the end of the flush of the color cache.  
    U32 fragmentSource;      //  Stores from which unit the fragments are received (Z Stencil Test or Fragment FIFO).  
    U32 blendCycles;         /**<  Cycles remaining until the next stamp can be blended.  *&

    //  Color cache.  
    ColorCache *colorCache;     //  Pointer to the color cache/write buffer.  
    U32 bytesLine;           //  Bytes per color cache line.  
    U32 lineShift;           //  Logical shift for a color cache line.  
    U32 lineMask;            //  Logical mask for a color cache line.  
    U32 stampMask;           //  Logical mask for the offset of a stamp inside a color cache line.  

    //  Blend Queue structures.  
    BlendQueue *blendQueue;     //  The blend queue.  Stores stamps to be blended or written.  
    U32 fetchBlend;          //  Entry of the blend queue from which to fetch color data.  
    U32 readBlend;           //  Entry of the blend queue from which to read color data.  
    U32 writeBlend;          //  Entry of the blend queue from which to write color data.  
    U32 freeBlend;           //  Next free entry in the blend queue.  
    U32 blendStamps;         //  Stamps stored in the blend queue.  
    U32 fetchStamps;         //  Stamps waiting in the blend queue to fetch color data.  
    U32 readStamps;          //  Stamps waiting in the blend queue to read color data.  
    U32 writeStamps;         //  Stamps waiting in the blend queue to write color data.  
    U32 freeStamps;          //  Free entries in the blend queue.  

    //  Configurable buffers.  
    FragmentInput **stamp;      //  Stores last receveived stamp.  

    //  Color Clear state.  
    U32 copyStateCycles;     //  Number of cycles remaining for the copy of the block state memory.  
    //U08 clearBuffer[MAX_TRANSACTION_SIZE];    //  Clear buffer, stores the clear values for a full transaction.  
    //U32 clearAddress;        //  Current color clear address.  
    //U32 endClearAddress;     //  End of the memory region to clear.  
    //U32 busCycles;           //  Remaining memory bus cycles.  
    //U32 ticket;              //  Memory ticket.  
    //U32 freeTickets;         //  Number of free memory tickets.  

    //  Statistics.  
    gpuStatistics::Statistic *inputs;       //  Input fragments.  
    gpuStatistics::Statistic *blended;      //  Blended fragments.  
    gpuStatistics::Statistic *logoped;      //  Logical operation fragments.  
    gpuStatistics::Statistic *outside;      //  Outside triangle/viewport fragments.  
    gpuStatistics::Statistic *culled;       //  Culled fragments.  
    gpuStatistics::Statistic *readTrans;    //  Read transactions to memory.  
    gpuStatistics::Statistic *writeTrans;   //  Write transactions to memory.  
    gpuStatistics::Statistic *readBytes;    //  Bytes read from memory.  
    gpuStatistics::Statistic *writeBytes;   //  Bytes written to memory.  
    gpuStatistics::Statistic *fetchOK;      //  Succesful fetch operations.  
    gpuStatistics::Statistic *fetchFail;    //  Failed fetch operations.  
    gpuStatistics::Statistic *allocateOK;   //  Succesful allocate operations.  
    gpuStatistics::Statistic *allocateFail; //  Failed allocate operations.  
    gpuStatistics::Statistic *readOK;       //  Sucessful read operations.  
    gpuStatistics::Statistic *readFail;     //  Failed read operations.  
    gpuStatistics::Statistic *writeOK;      //  Sucessful write operations.  
    gpuStatistics::Statistic *writeFail;    //  Failed write operations.  
    gpuStatistics::Statistic *rawDep;       //  Blocked read accesses because of read after write dependence between stamps.  

    //  Latency map.  
    U32 *latencyMap;                     //  Stores the fragment latency map for the Color Write unit.  
    bool clearLatencyMap;                   //  Clear the latency map.  

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

    /**
     *
     *  Converts an array of color values in integer format to float
     *  point format.
     *
     *  @param in The input color array in integer format.
     *  @param out The output color array in float point format.
     *
     */

    void colorInt2Float(U08 *in, Vec4FP32 *out);

    /**
     *
     *  Converts an array of color values in integer format to float
     *  point format.
     *
     *  @param in The input color array in float point format.
     *  @param out The output color array in integer point format.
     *
     */

    void colorFloat2Int(Vec4FP32 *in, U08 *out);


public:

    /**
     *
     *  Color Write mdu constructor.
     *
     *  Creates and initializes a new Color Write mdu object.
     *
     *  @param stampsCycle Number of stamps per cycle.
     *  @param overW Over scan tile width in scan tiles (may become a register!).
     *  @param overH Over scan tile height in scan tiles (may become a register!).
     *  @param scanW Scan tile width in fragments.
     *  @param scanH Scan tile height in fragments.
     *  @param genW Generation tile width in fragments.
     *  @param genH Generation tile height in fragments.
     *  @param bytesPixel Number of bytes to be used per pixel (should be a register!).
     *  @param disableColorCompr Disables color compression.
     *  @param cacheWays Color cache set associativity.
     *  @param cacheLines Number of lines in the color cache per way/way.
     *  @param stampsLine Numer of stamps per color cache line (less than a tile!).
     *  @param portWidth Width of the color cache ports in bytes.
     *  @param extraReadPort Adds an extra read port to the color cache.
     *  @param extraWritePort Adds an extra write port to the color cache.
     *  @param cacheReqQueueSize Size of the color cache memory request queue.
     *  @param inputRequests Number of read requests and input buffers in the color cache.
     *  @param outputRequests Number of write requests and output buffers in the color cache.
     *  @param maxBlocks Maximum number of sopported color blocks in the
     *  color cache state memory.
     *  @param blocksCycle Number of state block entries that can be
     *  modified (cleared), read and sent per cycle.
     *  @param compCycles Color block compression cycles.
     *  @param decompCycles Color block decompression cycles.
     *  @param blendQueueSize Blend queue size (in entries).
     *  @param blendRate Rate at which stamps are blended (cycles between stamps).
     *  @param blendLatency Blending latency.
     *  @param blockStateLatency Latency to send the color block state to the DAC.
     *  @param fragMapMode Fragment Map Mode.
     *  @param frEmu Reference to the Fragment Operation Behavior Model object.
     *  @param name The mdu name.
     *  @param prefix String used to prefix the mdu signals names.
     *  @param parent The mdu parent mdu.
     *
     *  @return A new Color Write object.
     *
     */

    ColorWrite(U32 stampsCycle, U32 overW, U32 overH, U32 scanW, U32 scanH,
        U32 genW, U32 genH, U32 bytesPixel, bool disableColorCompr,
        U32 cacheWays, U32 cacheLines, U32 stampsLine, U32 portWidth, bool extraReadPort,
        bool extraWritePort, U32 cacheReqQueueSize, U32 inputRequests,
        U32 outputRequests, U32 maxBlocks, U32 blocksCycle,
        U32 compCycles, U32 decompCycles,
        U32 blendQueueSize, U32 blendRate, U32 blendLatency, U32 blockStateLatency,
        U32 fragMapMode, bmoFragmentOperator &frOp, char *name, char *prefix = 0, cmoMduBase* parent = 0);

    /**
     *
     *  Color Write simulation function.
     *
     *  Simulates a cycle of the Color Write mdu.
     *
     *  @param cycle The cycle to simulate of the Color Write mdu.
     *
     */

    void clock(U64 cycle);

    /**
     *
     *  Get pointer to the fragment latency map.
     *
     *  @param width Width of the latency map in stamps.
     *  @param height Height of the latency map in stamps.
     *
     *  @return A pointer to the latency map stored in the Color Write unit.
     *
     */

    U32 *getLatencyMap(U32 &width, U32 &height);

    /**
     *
     *  Returns a single line string with state and debug information about the
     *  Color Write mdu.
     *
     *  @param stateString Reference to a string where to store the state information.
     *
     */

    void getState(string &stateString);

};

} // namespace arch

#endif

