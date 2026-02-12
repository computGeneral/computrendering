/**************************************************************************
 *
 * Color Write mdu definition file.
 *
 */

/**
 *
 *  @file cmColorWriteV2.h
 *
 *  This file defines the Color Write mdu.
 *
 *  This class implements the final color blend and color write
 *  stages of the GPU pipeline.
 *
 */


#ifndef _COLORWRITEV2_

#define _COLORWRITEV2_

#include "GPUType.h"
#include "support.h"
#include "cmMduBase.h"
#include "cmColorCacheV2.h"
#include "cmRasterizerCommand.h"
#include "cmRasterizerState.h"
#include "GPUReg.h"
#include "cmFragmentInput.h"
#include "bmFragmentOperator.h"
#include "cmGenericROP.h"
#include "ValidationInfo.h"

namespace arch
{

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
    SHADER_LATENCY_MAP = 3,
    DISABLE_MAP = 0xff
};

/**
 *
 *  This class implements the simulation of the Color Blend,
 *  Logical Operation and Color Write stages of a GPU pipeline.
 *
 *  This class inherits from the Generic ROP class that implements a generic
 *  ROP pipeline
 *
 */

class cmoColorWriter : public GenericROP
{
private:

    Signal *blockStateDAC;      //  Signal to send the color buffer block state to the DisplayController.  

    //  Color Write registers.  
    TextureFormat colorBufferFormat;        //  Format of the color buffer.  
    bool colorSRGBWrite;                    //  Flag to enable/disable conversion from linear to sRGB on color write.  
    bool rtEnable[MAX_RENDER_TARGETS];      //  Flag to enable/disable a render target.  
    TextureFormat rtFormat[MAX_RENDER_TARGETS]; //  Format of the render target.  
    U32 rtAddress[MAX_RENDER_TARGETS];       //  Base address of the render target.  
    
    Vec4FP32 clearColor;       //  Current clear color for the framebuffer.  
    bool blend[MAX_RENDER_TARGETS];                 //  Flag storing if color blending is active.  
    BlendEquation equation[MAX_RENDER_TARGETS];     //  Current blending equation mode.  
    BlendFunction srcRGB[MAX_RENDER_TARGETS];       //  Source RGB weight funtion.  
    BlendFunction dstRGB[MAX_RENDER_TARGETS];       //  Destination RGB weight function.  
    BlendFunction srcAlpha[MAX_RENDER_TARGETS];     //  Source alpha weight function.  
    BlendFunction dstAlpha[MAX_RENDER_TARGETS];     //  Destination alpha weight function.  
    Vec4FP32 constantColor[MAX_RENDER_TARGETS];    //  Blend constant color.  
    bool writeR[MAX_RENDER_TARGETS];                //  Write mask for red color component.  
    bool writeG[MAX_RENDER_TARGETS];                //  Write mask for green color component.  
    bool writeB[MAX_RENDER_TARGETS];                //  Write mask for blue color component.  
    bool writeA[MAX_RENDER_TARGETS];                //  Write mask for alpha color channel.  
    bool logicOperation;        //  Flag storing if logical operation is active.  
    LogicOpMode logicOpMode;    //  Current logic operation mode.  
    U32 frontBuffer;         //  Address in the GPU memory of the front (primary) buffer.  
    U32 backBuffer;          //  Address in the GPU memory of the back (secondary) buffer.  

    U08 clearColorData[MAX_BYTES_PER_COLOR];      // Color clear value converted to the color buffer format.  

    //  Color Write parameters.  
    bool disableColorCompr;     //  Disables color compression.  
    U32 colorCacheWays;      //  Color cache set associativity.  
    U32 colorCacheLines;     //  Number of lines in the color cache per way/way.  
    U32 stampsLine;          //  Number of stamps per color cache line.  
    U32 cachePortWidth;      //  Width of the color cache ports in bytes.  
    bool extraReadPort;         //  Add an additional read port to the color cache.  
    bool extraWritePort;        //  Add an additional write port to the color cache.  
    U32 blocksCycle;         //  State of color block copied to the DisplayController per cycle.  
    U32 blockStateLatency;   //  Latency of the block state update signal to the DisplayController.  
    U32 fragmentMapMode;     //  Fragment map mode.  
    U32 numStampUnits;       //  Number of stamp units in the GPU pipeline.  
    
    //  Color Write state.  
    RasterizerCommand *lastRSCommand;   //  Stores the last Rasterizer Command received (for signal tracing).  

    //  Color cache.  
    ColorCacheV2 *colorCache;   //  Pointer to the color cache/write buffer.  
    U32 bytesLine;           //  Bytes per color cache line.  
    U32 lineShift;           //  Logical shift for a color cache line.  
    U32 lineMask;            //  Logical mask for a color cache line.  

    //  Color cache blend output color data array.  
    U08 *outColor;            //  Stores blended color data before logic op & write.  

    //  Color Clear state.  
    U32 copyStateCycles;     //  Number of cycles remaining for the copy of the block state memory.  

    //  Statistics.  
    gpuStatistics::Statistic *inputs;       //  Input fragments.  
    gpuStatistics::Statistic *blended;      //  Blended fragments.  
    gpuStatistics::Statistic *logoped;      //  Logical operation fragments.  

    //  Latency map.  
    U32 *latencyMap;                     //  Stores the fragment latency map for the Color Write unit.  
    bool clearLatencyMap;                   //  Clear the latency map.  

    //  Debug/validation.
    bool validationMode;                //  Flag that stores if the validation mode is enabled in the behaviorModel.  
    FragmentQuadMemoryUpdateMap colorMemoryUpdateMap[MAX_RENDER_TARGETS];   //  Map indexed by fragment identifier storing updates to the color buffer.  

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
     *  Processes a memory transaction.
     *
     *  @param cycle The current simulation cycle.
     *  @param memTrans The memory transaction to process
     *
     */

    void processMemoryTransaction(U64 cycle, MemoryTransaction *memTrans);

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

    /**
     *
     *  Converts an array of color values in RGBA8 format to RGBA32F format.
     *
     *  @param in The input color array in RGBA8 format.
     *  @param out The output color array in RGBA32F format.
     *
     */

    void colorRGBA8ToRGBA32F(U08 *in, Vec4FP32 *out);

    /**
     *
     *  Converts an array of color values in RGBA32F format to RGBA8 format.
     *
     *  @param in The input color array in RGBA32F format.
     *  @param out The output color array in RGBA8 format.
     *
     */

    void colorRGBA32FToRGBA8(Vec4FP32 *in, U08 *out);

    /**
     *
     *  Converts an array of color values in RGBA16F format to RGBA32F format.
     *
     *  @param in The input color array in RGBA16F format.
     *  @param out The output color array in RGBA32F format.
     *
     */

    void colorRGBA16FToRGBA32F(U08 *in, Vec4FP32 *out);

    /**
     *
     *  Converts an array of color values in RGBA32F format to RGBA16F format.
     *
     *  @param in The input color array in RGBA32F format.
     *  @param out The output color array in RGBA16F format.
     *
     */

    void colorRGBA32FToRGBA16F(Vec4FP32 *in, U08 *out);

    /**
     *
     *  Converts an array of color values in RGBA16 format to RGBA32F format.
     *
     *  @param in The input color array in RGBA16 format.
     *  @param out The output color array in RGBA32F format.
     *
     */

    void colorRGBA16ToRGBA32F(U08 *in, Vec4FP32 *out);

    /**
     *
     *  Converts an array of color values in RGBA32F format to RGBA16 format.
     *
     *  @param in The input color array in RGBA32F format.
     *  @param out The output color array in RGBA16 format.
     *
     */

    void colorRGBA32FToRGBA16(Vec4FP32 *in, U08 *out);

    /**
     *
     *  Converts an array of color values in RG16F format to RGBA32F format.
     *
     *  @param in The input color array in RG16F format.
     *  @param out The output color array in RGBA32F format.
     *
     */

    void colorRG16FToRGBA32F(U08 *in, Vec4FP32 *out);

    /**
     *
     *  Converts an array of color values in RGBA32F format to RG16F format.
     *
     *  @param in The input color array in RGBA32F format.
     *  @param out The output color array in RG16F format.
     *
     */

    void colorRGBA32FToRG16F(Vec4FP32 *in, U08 *out);

    /**
     *
     *  Converts an array of color values in R32F format to RGBA32F format.
     *
     *  @param in The input color array in R32F format.
     *  @param out The output color array in RGBA32F format.
     *
     */

    void colorR32FToRGBA32F(U08 *in, Vec4FP32 *out);

    /**
     *
     *  Converts an array of color values in RGBA32F format to R32F format.
     *
     *  @param in The input color array in RGBA32F format.
     *  @param out The output color array in R32F format.
     *
     */

    void colorRGBA32FToR32F(Vec4FP32 *in, U08 *out);

    /**
     *
     *  Converts an array of 32-bit float point color values from linear to sRGB space.
     *
     *  @param in A pointer to the array of color values.
     *
     */
     
    void colorLinearToSRGB(Vec4FP32 *in);

    /**
     *
     *  Converts an array of 32-bit float point color values from sRGB to linear space.
     *
     *  @param in A pointer to the array of color values.
     *
     */
     
    void colorSRGBToLinear(Vec4FP32 *in);

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
     *  @param numStampUnits Number of stamp units in the GPU.
     *  @param maxBlocks Maximum number of sopported color blocks in the olor cache state memory.
     *  @param blocksCycle Number of state block entries that can be modified (cleared), read and sent per cycle.
     *  @param compCycles Color block compression cycles.
     *  @param decompCycles Color block decompression cycles.
     *  @param colorInQSz Color input stamp queue size (entries/stamps)
     *  @param colorFetchQSz Color fetched stamp queue size (entries/stamps)
     *  @param colorReadQSz Color read stamp queue size (entries/stamps)
     *  @param colorOpQSz Color operated stamp queue size (entries/stamps)
     *  @param writeOpQSz Color written stamp queue size (entries/stamps)
     *  @param blendRate Rate at which stamps are blended (cycles between stamps).
     *  @param blendLatency Blending latency.
     *  @param blockStateLatency Latency to send the color block state to the DisplayController.
     *  @param fragMapMode Fragment Map Mode.
     *  @param frEmu Reference to the Fragment Operation Behavior Model object.
     *  @param name The mdu name.
     *  @param prefix String used to prefix the mdu signals names.
     *  @param parent The mdu parent mdu.
     *
     *  @return A new Color Write object.
     *
     */

    cmoColorWriter(U32 stampsCycle, U32 overW, U32 overH, U32 scanW, U32 scanH,
        U32 genW, U32 genH,
        bool disableColorCompr, U32 cacheWays, U32 cacheLines, U32 stampsLine,
        U32 portWidth, bool extraReadPort, bool extraWritePort, U32 cacheReqQueueSize,
        U32 inputRequests, U32 outputRequests, U32 numStampUnits, U32 maxBlocks, U32 blocksCycle,
        U32 compCycles, U32 decompCycles,
        U32 colorInQSz, U32 colorFetchQSz, U32 colorReadQSz, U32 colorOpQSz,
        U32 colorWriteQSz,
        U32 blendRate, U32 blendLatency, U32 blockStateLatency,
        U32 fragMapMode, bmoFragmentOperator &frOp,
        char *name, char *prefix = 0, cmoMduBase* parent = 0);

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
     *  Set Color Write unit validation mode.
     *
     *  @param enable Boolean value that defines if the validation mode is enabled.
     *
     */
     
    void setValidationMode(bool enable);

    /**
     *
     *  Get the color memory update map of the defined render target for the current draw call.
     *
     *  @param rt Idenfier for the render target for which to obtain the color memory update map.
     *
     *  @return A reference to the color memory update map of the defined render target for the current draw call.
     *
     */
  
    FragmentQuadMemoryUpdateMap &getColorUpdateMap(U32 rt);

};

} // namespace arch

#endif

