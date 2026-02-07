/**************************************************************************
 *
 * Z Stencil test mdu definition file.
 *
 */

/**
 *
 *  @file cmZStencilTestV2.h
 *
 *  This file defines the Z and Stencil Test mdu.
 *
 *  This class implements the Z and stencil tests stages of the GPU pipeline.
 *
 */


#ifndef _ZSTENCILTESTV2_

#define _ZSTENCILTESTV2_

#include "GPUType.h"
#include "support.h"
#include "cmMduBase.h"
#include "cmZCacheV2.h"
#include "cmRasterizerCommand.h"
#include "cmRasterizerState.h"
#include "GPUReg.h"
#include "cmFragmentInput.h"
#include "bmFragmentOperator.h"
#include "bmRasterizer.h"
#include "cmGenericROP.h"
#include "ValidationInfo.h"

namespace cg1gpu
{

/**
 *
 *  This class implements the simulation of the Z and Stencil
 *  test stages of a GPU pipeline.
 *
 *  This class inherits from the Generic ROP class that implements a generic
 *  ROP pipeline
 *
 */

class cmoDepthStencilTester : public GenericROP
{
private:

    //  Z Stencil Test signals.
    Signal *hzUpdate;           //  Update signal to the Hierarchical Z mdu.  
    Signal *blockStateDAC;      //  Signal to send the color buffer block state to the DisplayController.  

    //  Z Stencil test registers.
    U32 clearDepth;                  //  Current clear depth value.  
    U32 depthPrecission;             //  Depth bit precission.  
    U08 clearStencil;                 //  Current clear stencil value.  
    bool modifyDepth;                   //  Flag that stores if the fragment shader has modified the fragment depth component.  
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
    U32 zBuffer;                     //  Address in the GPU memory of the current Z buffer.  

    //  Z Stencil parameters.
    U32 zCacheWays;          //  Z cache set associativity.  
    U32 zCacheLines;         //  Number of lines in the Z cache per way.  
    U32 stampsLine;          //  Number of stamps per Z cache line.  
    U32 cachePortWidth;      //  Width of the Z cache ports in bytes.  
    bool extraReadPort;         //  Add an additional read port to the color cache.  
    bool extraWritePort;        //  Add an additional write port to the color cache.  
    U32 zQueueSize;          //  Z queue size.  
    bool disableZCompression;   //  Disables Z compression (and HZ update!).  
    bool disableHZUpdate;       //  Disables HZ Update.  
    U32 hzUpdateLatency;     //  Latency of the hierarchizal Z update signal.  
    U32 blockStateLatency;   //  Latency of the block state update signal to the DisplayController.  
    U32 numStampUnits;       //  Number of stamp units in the GPU pipeline.  
    U32 blocksCycle;         //  State of z stencil block states copied to the DisplayController per cycle.  

    //  Z Write state.
    RasterizerCommand *lastRSCommand;   //  Stores the last Rasterizer Command received (for signal tracing).  

    //  Z cache.
    ZCacheV2 *zCache;           //  Pointer to the Z cache.  
    U32 bytesLine;           //  Bytes per Z cache line.  
    U32 lineShift;           //  Logical shift for a Z cache line.  
    U32 lineMask;            //  Logical mask for a Z cache line.  

    //  Block state copy state.
    U32 copyStateCycles;     //  Number of cycles remaining for the copy of the block state memory.  

    //  Rasterizer Behavior Model associated with the stage.
    bmoRasterizer &bmRaster;//  Reference to the rasterizer behaviorModel objecter.  

    gpuStatistics::Statistic *outputs;      //  Output fragments.  
    gpuStatistics::Statistic *tested;       //  Tested fragments.  
    gpuStatistics::Statistic *failed;       //  Fragments that failed the tests.  
    gpuStatistics::Statistic *passed;       //  Fragments that passed the tests.  
    gpuStatistics::Statistic *updatesHZ;    //  Updates to Hierarchical Z.  

    //  Debug/validation.
    bool validationMode;                    //  Flag that stores if validation mode is enabled.  
    FragmentQuadMemoryUpdateMap zstencilMemoryUpdateMap;    //  Map indexed by fragment identifier storing updates to the z stencil buffer.  

    //  Debug/Log.
    bool traceFragment;     //  Flag that enables/disables a trace log for the defined fragment.  
    U32 watchFragmentX;  //  Defines the fragment x coordinate to trace log.  
    U32 watchFragmentY;  //  Defines the fragment y coordinate to trace log.  

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
     *  Performs any remaining tasks after the stamp data has been written.
     *  The function should read, process and remove the stamp at the head of the
     *  written stamp queue.
     *
     *  @param cycle Current simulation cycle.
     *  @param stamp Pointer to a stamp that has been written to the ROP data buffer
     *  and needs processing.
     *
     */

    void postWriteProcess(U64 cycle, ROPQueue *stamp);


    /**
     *
     *  To be called after calling the update/clock function of the ROP Cache.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void postCacheUpdate(U64 cycle);

    /**
     *
     *  This function is called when the ROP stage is in the the reset state.
     *
     */

    void reset();

    /**
     *
     *  This function is called when the ROP stage is in the flush state.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void flush(U64 cycle);
    
    /**
     *
     *  This function is called when the ROP stage is in the swap state.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void swap(U64 cycle);

    /**
     *
     *  This function is called when the ROP stage is in the clear state.
     *
     */

    void clear();

    /**
     *
     *  This function is called when a stamp is received at the end of the ROP operation
     *  latency signal and before it is queued in the operated stamp queue.
     *
     *  @param cycle Current simulation cycle.
     *  @param stamp Pointer to the Z queue entry for the stamp that has to be operated.
     *
     */

    void operateStamp(U64 cycle, ROPQueue *stamp);

    /**
     *
     *  This function is called when all the stamps but the last one have been processed.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void endBatch(U64 cycle);

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
     *  @param numStampUnits Number of stamp units in the GPU.
     *  @param maxBlocks Maximum number of sopported Z blocks in the Z cache state memory.
     *  @param blocksCycle Number of state block entries that can be cleared per cycle.
     *  @param compCycles Z block compression cycles.
     *  @param decompCycles Z block decompression cycles.
     *  @param zInQSz Z input stamp queue size (entries/stamps).
     *  @param zFetchQSz Z fetched stamp queue size (entries/stamps).
     *  @param zReadQSz Z read stamp queue size (entries/stamps).
     *  @param zOpQSz Z operated stamp queue size (entries/stamps).
     *  @param writeOpQSz Z written stamp queue size (entries/stamps).
     *  @param zTestRate Rate at which stamp are tested (cycles between two stamps to be tested).
     *  @param zTestLatency Z and stencil test latency.
     *  @param disableHZUpdate Disables HZ update.
     *  @param hzUpdateLatency Latency to send updates to Hierarchical Z buffer.
     *  @param blockStateLatency Latency to send the z stencil block state to the DisplayController.
     *  @param frEmu Reference to the Fragment Operation Behavior Model object.
     *  @param bmRaster Reference to the Rasterizer Behavior Model object to be used by the mdu.
     *  @param name The mdu name.
     *  @param prefix String used to prefix the mdu signals names.
     *  @param parent The mdu parent mdu.
     *
     *  @return A new Z Stencil Test object.
     *
     */

    cmoDepthStencilTester(U32 stampsCycle, U32 overW, U32 overH, U32 scanW, U32 scanH,
        U32 genW, U32 genH,
        bool disableZCompr, U32 cacheWays, U32 cacheLines, U32 stampsLine, U32 portWidth,
        bool extraReadPort, bool extraWritePort, U32 cacheReqQueueSize, U32 inputRequests,
        U32 outputRequests, U32 numStampUnits, U32 maxBlocks, U32 blocksCycle,
        U32 compCycles, U32 decompCycles,
        U32 zInQSz, U32 zFetchQSz, U32 zReadQSz, U32 zOpQSz, U32 zWriteQSz,
        U32 zTestRate, U32 zTestLatency, bool disableHZUpdate, U32 hzUpdateLatency,
        U32 blockStateLatency,
        bmoFragmentOperator &frOp, bmoRasterizer &bmRaster,
        char *name, char *prefix = 0, cmoMduBase* parent = 0);

    /**
     *
     *  Saves the block state memory into a file.
     *
     */
     
    void saveBlockStateMemory();
    
    /**
     *
     *  Loads the block state memory from a file.
     *
     */
     
    void loadBlockStateMemory();

    /**
     *
     *  Get the list of debug commands supported by the Color Write mdu.
     *
     *  @param commandList Reference to a string variable where to store the list debug commands supported
     *  by the Color Write mdu.
     *
     */
     
    void getCommandList(std::string &commandList);

    /**
     *
     *  Executes a debug command on the Color Write mdu.
     *
     *  @param commandStream A reference to a stringstream variable from where to read
     *  the debug command and arguments.
     *
     */    
     
    void execCommand(stringstream &commandStream);

    /** 
     *
     *  Set Z Stencil Test unit validation mode.
     *
     *  @param enable Boolean value that defines if the validation mode is enabled.
     *
     */
     
    void setValidationMode(bool enable);

    /**
     *
     *  Get the z stencil buffer memory update map for the current draw call.
     *
     *  @return A reference to the z stencil buffer memory update map for the current draw call.
     *
     */
     
    FragmentQuadMemoryUpdateMap &getZStencilUpdateMap();
    
};

} // namespace cg1gpu

#endif

