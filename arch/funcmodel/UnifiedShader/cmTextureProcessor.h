/**************************************************************************
 *
 * Shader Texture Unit cmoMduBase.
 *  This mdu implements the simulation a Texture Unit for shader unit.
 *
 */

#ifndef _TEXTUREUNIT_

#define _TEXTUREUNIT_

#include "support.h"
#include "GPUType.h"
#include "cmMduBase.h"
#include "GPUReg.h"
#include "bmTextureProcessor.h"
#include "cmShaderCommand.h"
#include "cmShaderFetch.h"
#include "cmTextureCacheGen.h"
#include "cmTextureCache.h"
#include "cmTextureCacheL2.h"
#include "cmTextureRequest.h"
#include "cmTextureResult.h"
#include "ArchConfig.h"

//#include <fstream>
#include "zfstream.h"

using namespace std;

namespace cg1gpu
{

/**
 *
 *  Texture Unit State.
 *
 */

enum TextureUnitState
{
    TU_READY,   //  The Shader accepts new texture read requests.  
    TU_BUSY     //  The Shader can not accept new texture requests.  
};

/**
 *  Defines the maximum number of loops that can be executed in the filter  pipeline.
 */
static const U32 MAX_FILTER_LOOPS = 4;

/**
 *  Defines the maximum number of bytes to be read per texel.
 */
static const U32 MAX_TEXEL_DATA = 16;

/**
 *
 *  cmoTextureProcessor implements the texture unit attached to a shader.
 *  Inherits from cmoMduBase class.
 *
 */

class cmoTextureProcessor : public cmoMduBase
{
private:
#ifdef GPU_TEX_TRACE_ENABLE
    //ofstream texTrace;
    //ofstream texAddrTrace;
    gzofstream texTrace;
    gzofstream texAddrTrace;
#endif

    //  Texture Unit signals.  
    Signal *commProcSignal;     //  Command signal from the Command Processor.  
    Signal *readySignal;        //  Ready signal to the ShaderDecodeExecute mdu.  
    Signal *textDataSignal;     //  Texture data signal to the Decode/Execute mdu.  
    Signal *textRequestSignal;  //  Texture request signal from Decode/Execute.  
    Signal *filterInput;        //  Input signal for the filter unit (simulates filter latency).  
    Signal *filterOutput;       //  Output signal for the filter unit (simulates filter latency).  
    Signal *addressALUInput;    //  Input signal for the address ALU (simulates address calculation latency).  
    Signal *addressALUOutput;   //  Output signal for the address ALU (simulates address calculation latency).  
    Signal *memDataSignal;      //  Data signal from the Memory Controller.  
    Signal *memRequestSignal;   //  Request signal to the Memory Controller.  

    //  Texture Unit registers.  
    bool textureEnabled[MAX_TEXTURES];      //  Texture unit enable flag.  
    TextureMode textureMode[MAX_TEXTURES];  //  Current texture mode active in the texture unit.  
    U32 textureAddress[MAX_TEXTURES][MAX_TEXTURE_SIZE][CUBEMAP_IMAGES];  //  Address in GPU memory of the active texture mipmaps.  
    U32 textureWidth[MAX_TEXTURES];      //  Active texture width in texels.  
    U32 textureHeight[MAX_TEXTURES];     //  Active texture height in texels.  
    U32 textureDepth[MAX_TEXTURES];      //  Active texture depth in texels.  
    U32 textureWidth2[MAX_TEXTURES];     //  Log2 of the texture width (base mipmap size).  
    U32 textureHeight2[MAX_TEXTURES];    //  Log2 of the texture height (base mipmap size).  
    U32 textureDepth2[MAX_TEXTURES];     //  Log2 of the texture depth (base mipmap size).  
    U32 textureBorder[MAX_TEXTURES];     //  Texture border in texels.  
    TextureFormat textureFormat[MAX_TEXTURES];  //  Texture format of the active texture.  
    bool textureReverse[MAX_TEXTURES];          //  Reverses texture data (little to big endian).  
    TextureCompression textureCompr[MAX_TEXTURES];  //  Texture compression mode of the active texture.  
    TextureBlocking textureBlocking[MAX_TEXTURES];  //  Texture blocking mode for the texture.  
    bool textD3D9ColorConv[MAX_TEXTURES];           //  Defines if the texture color components have to read in the order defined by D3D9.  
    bool textD3D9VInvert[MAX_TEXTURES];             //  Defines if the texture u coordinate has to be inverted as defined by D3D9.  
    Vec4FP32 textBorderColor[MAX_TEXTURES];    //  Texture border color.  
    ClampMode textureWrapS[MAX_TEXTURES];       //  Texture wrap mode for s coordinate.  
    ClampMode textureWrapT[MAX_TEXTURES];       //  Texture wrap mode for t coordinate.  
    ClampMode textureWrapR[MAX_TEXTURES];       //  Texture wrap mode for r coordinate.  
    bool textureNonNormalized[MAX_TEXTURES];    //  Texture coordinates are non-normalized.  
    FilterMode textureMinFilter[MAX_TEXTURES];  //  Texture minification filter.  
    FilterMode textureMagFilter[MAX_TEXTURES];  //  Texture Magnification filter.  
    bool textureEnableComparison[MAX_TEXTURES]; //  Texture Enable Comparison Filter (PCF).  
    CompareMode textureComparisonFunction[MAX_TEXTURES];   //  Texture Comparison Function (PCF).  
    bool textureSRGB[MAX_TEXTURES];         //  Texture sRGB space to linear space conversion.  
    F32 textureMinLOD[MAX_TEXTURES];     //  Texture minimum lod.  
    F32 textureMaxLOD[MAX_TEXTURES];     //  Texture maximum lod.  
    F32 textureLODWays[MAX_TEXTURES];    //  Texture lod ways.  
    U32 textureMinLevel[MAX_TEXTURES];   //  Texture minimum mipmap level.  
    U32 textureMaxLevel[MAX_TEXTURES];   //  Texture maximum mipmap level.  
    F32 textureUnitLODWays[MAX_TEXTURES];    //  Texture unit lod ways (not texture lod!!).  
    U32 maxAnisotropy[MAX_TEXTURES];     //  Maximum anisotropy allowed for the texture unit.  

    //  Stream input registers.
    U32 attributeMap[MAX_VERTEX_ATTRIBUTES];     //  Mapping from vertex input attributes and vertex streams.  
    Vec4FP32 attrDefValue[MAX_VERTEX_ATTRIBUTES];  //  Defines the vertex attribute default values.  
    U32 streamAddress[MAX_STREAM_BUFFERS];       //  Address in GPU memory for the vertex stream buffers.  
    U32 streamStride[MAX_STREAM_BUFFERS];        //  Stride for the stream buffers.  
    StreamData streamData[MAX_STREAM_BUFFERS];      //  Data type for the stream buffer.  
    U32 streamElements[MAX_STREAM_BUFFERS];      //  Number of stream data elements (vectors) per stream buffer entry.  
    bool d3d9ColorStream[MAX_STREAM_BUFFERS];       //  Read components of the color attributes in the order defined by D3D9.  

    
    //  Texture unit queues.  

    TextureRequest **textRequestsQ;         //  Queue for the texture requests.  
    U32 nextRequest;                     //  Next texture request to be processed.  
    U32 nextFreeRequest;                 //  Next empty entry in the texture request queue.  
    U32 requests;                        //  Number of texture requests in the request queue.  
    U32 freeRequests;                    //  Number of free entries in the the request queue.  

    TextureAccess **texAccessQ;             //  Queue for the texture accesses being processed.  
    TextureRequest **texAccessReqQ;         //  Stores the requests that generated the texture accesses to keep the original cookies.  
    U32 numCalcTexAcc;                   //  Number of texture accesses for which to calculate the texel addresses.  
    U32 numFetchTexAcc;                  //  Number of texture accesses waiting to be fetched.  
    U32 numFreeTexAcc;                   //  Number of free entries in the texture access queue.  
    U32 *freeTexAccessQ;                 //  Stores the free entries in the texture access queue.  
    U32 nextFreeTexAcc;                  //  Pointer to the next entry in the free texture access queue.  
    U32 lastFreeTexAcc;                  //  Pointer to the last entry in the free texture access queue.  
    U32 *calcTexAccessQ;                 //  Stores the pointers to the texture access queue entries waiting for address calculation.  
    U32 nextCalcTexAcc;                  //  Next entry in the address calculation queue.  
    U32 lastCalcTexAcc;                  //  Last entry in the address calculation queue.  
    U32 *fetchTexAccessQ;                //  Stores the pointers to the texture access queue entries waiting to be fetched.  
    U32 nextFetchTexAcc;                 //  Next entry in the fetch queue.  
    U32 lastFetchTexAcc;                 //  Last entry in the fetch queue.  

    TextureResult **textResultQ;            //  Queue for the texture results being waiting to be sent back to the shader.  
    U32 nextResult;                      //  Next texture result to be sent back to the Shader.  
    U32 nextFreeResult;                  //  Next free entry in the texture result queue.  
    U32 numResults;                      //  Number of texture results waiting to be sent back to the Shader.  
    U32 freeResults;                     //  Number of free entries in the texture result queue.  

    /**
     *  Defines a trilinear processing element from a texture access
     *  awaiting processing (fetch, read or filter).
     */
    struct TrilinearElement
    {
        U32 access;          //  Pointer to the texture access for the trilinear processing element.  
        U32 trilinear;       //  Pointer to the trilinear processing element inside the texture access.  
    };

    TrilinearElement *readyReadQ;   //  Stores the trilinear processing elements that can be read from the texture cache.  
    U32 nextFreeReadyRead;       //  Next free entry in the trilinear ready read queue.  
    U32 nextReadyRead;           //  Next trilinear element to read in the trilinear ready read queue.  
    U32 numFreeReadyReads;       //  Number of free entries in the trilinear ready read queue.  
    U32 numReadyReads;           //  Number of triliner elements in the trilinear ready read queue.  

    TrilinearElement *waitReadW;    //  Stores trilinear elements that are waiting for a line in the texture cache.  
    U32 *freeWaitReadQ;          //  Stores the free entries in the trilinear wait read window.  
    U32 *waitReadQ;              //  Stores the waiting entries in the trilinear wait read window.  
    U32 nextFreeWaitRead;        //  Pointer to the next entry in the free wait read entry queue.  
    U32 lastFreeWaitRead;        //  Pointer to the last entry in the free wait read entry queue.  
    U32 numFreeWaitReads;        //  Number of free trilinear wait read entries.  
    U32 numWaitReads;            //  Number of trilinear elements waiting in the trilinear wait read queue.  
    U32 pendingMoveToReady;      //  Number of trilinear elements pending to be moved to the ready read queue.  

    TrilinearElement *filterQ;      //  Stores the trilinear processing elements waiting to be filtered.  
    U32 nextFreeFilter;          //  Pointer to the next free entry in the trilinear filter queue.  
    U32 nextToFilter;            //  Pointer to the next trilinear element to be filtered in the trilinear filter queue.  
    U32 nextFiltered;            //  Pointer to the next trilinear element already filtered in the filter queue.  
    U32 numFreeFilters;          //  Number of free entries in the trilinear filter queue.  
    U32 numToFilter;             //  Number of trilinear elements to be filtered in the trilinear filter queue.  
    U32 numFiltered;             //  Number of trilinear elements filtered in the trilinear filter queue.  

    //  Texture Cache.  
    TextureCacheGen* textCache;     //  Pointer to the generic texture cache interface that will be used by Texture Unit implementation.  
    TextureCache*    textCacheL1;   //  Pointer to the single level texture cache.  
    TextureCacheL2*  textCacheL2;   //  Pointer to the two level texture cache object.  

    //  Address ALU state.  
    U32 addressALUCycles;    //  Cycles remaining until the address ALU can receive a new input.  

    //  Filter unit state.  
    U32 filterCycles;        //  Cycles remaining until the filter unit can receive a new input.  

    //  Texture Unit state.  
    TextureUnitState txState;   //  State of the Texture Unit.  
    ShaderState state;          //  Current state of the Shader Unit.  
    MemState memoryState;       //  State of the Memory Controller.  
    U32 textureTickets;      //  Number of texture tickets offered to the Shader.  

    //  Texture Unit parameters.  
    bmoTextureProcessor* bmTexProc;    //  Reference to the Texture Behavior Model object attached to the Texture Unit.  
    U32 stampFragments;      //  Number of fragments in a stamp.  
    U32 requestsCycle;       //  Texture requests that can be received per cycle.  
    U32 requestQueueSize;    //  Size of the texture request queue.  
    U32 accessQueueSize;     //  Size of the texture access queue.  
    U32 resultQueueSize;     //  Size of the texture result queue.  
    U32 waitReadWindowSize;  //  Size of the read wait window.  
    bool useTwoLevelCache;      //  Enables the two level texture cache or the single level texture cache.  
    U32 textCacheLineSizeL0; //  Size in bytes of the texture cache lines for L0 cache or single level.  
    U32 textCacheWaysL0;     //  Number of ways in the texture cache for L0 cache or single level.  
    U32 textCacheLinesL0;    //  Number of lines per way in the texture cache for L0 cache or single level.  
    U32 textCachePortWidth;  //  Width in bytes of the texture cace ports for L0 cache or single level.  
    U32 textCacheReqQSize;   //  Size of the texture cache request queue for L0 cache or single level.  
    U32 textCacheInReqSizeL0;//  Size of the texture cache input request queue for L0 cache or single level.  
    U32 textCacheMaxMiss;    //  Misses supported per cycle in the texture cache for L0 cache or single level.  
    U32 textCacheDecomprLatency; //  Decompression latency for compressed texture cache lines for L0 cache or single level.  
    U32 textCacheLineSizeL1; //  Size in bytes of the texture cache lines for L1 cache (two level version).  
    U32 textCacheWaysL1;     //  Number of ways in the texture cache for L1 cache (two level version).  
    U32 textCacheLinesL1;    //  Number of lines per way in the texture cache for L1 cache (two level version).  
    U32 textCacheInReqSizeL1;//  Size of the texture cache input request queue for L1 cache (two level version).  

    U32 addressALULatency;   //  Latency of calculating the address of a bilinear sample.  
    U32 filterLatency;       //  Latency of filtering a bilinear sample.  
    
    //  Statistics.  
    gpuStatistics::Statistic *txRequests;   //  Number of texture requests from Shader Decode Execute.  
    gpuStatistics::Statistic *results;      //  Number of texture results from Shader Decode Execute.  
    gpuStatistics::Statistic *fetchOK;      //  Succesful fetch operations.  
    gpuStatistics::Statistic *fetchSkip;    //  Skip already fetched texel.  
    gpuStatistics::Statistic *fetchFail;    //  Unsuccesful fetch operations.  
    gpuStatistics::Statistic *fetchStallFetch;      //  Number of cycles the fetch stage stalls because there are no fetches waiting in the queue.  
    gpuStatistics::Statistic *fetchStallAddress;    //  Number of cycles the fetch stage stalls because the address for the next fetch has not been calculated.  
    gpuStatistics::Statistic *fetchStallWaitRead;   //  Number of cycles the fetch stage stalls because there are no free entries in the wait read window.  
    gpuStatistics::Statistic *fetchStallReadyRead;  //  Number of cycles the fetch stage stalls because the ready read queue has no free entries.  
    gpuStatistics::Statistic *readOK;       //  Succesful read operations.  
    gpuStatistics::Statistic *readFail;     //  Failed read operations.  
    gpuStatistics::Statistic *readBytes;    //  Bytes read from memory.  
    gpuStatistics::Statistic *readTrans;    //  Read transactions to memory.  
    gpuStatistics::Statistic *readyReadLevel;   //  Occupation of the ready read queue.  
    gpuStatistics::Statistic *waitReadLevel;    //  Occupation of the wait read window.  
    gpuStatistics::Statistic *resultLevel;      //  Occupation of the texture result queue.  
    gpuStatistics::Statistic *accessLevel;      //  Occupation of the texture access queue.  
    gpuStatistics::Statistic *requestLevel;     //  Occupation of the texture request queue.  
    gpuStatistics::Statistic *addrALUBusy;      //  Address ALU busy cycles.  
    gpuStatistics::Statistic *filterALUBusy;    //  Filter ALU busy cycles.  
    gpuStatistics::Statistic *textResLat;       //  Texture result latency in cycles.  
    gpuStatistics::Statistic *bilinearSamples;  //  Number of bilinear samples per texture access.  
    gpuStatistics::Statistic *anisoRatio;       //  Anisotropy ratio per texture access.  
    gpuStatistics::Statistic *addrCalcFinished; //  Number of texture access that have finished address calculation.  
    gpuStatistics::Statistic *anisoRatioHisto[MAX_ANISOTROPY + 1];  //  Histogram of computed anisotropy ratio.  
    gpuStatistics::Statistic *bilinearsHisto[MAX_ANISOTROPY * 2];   //  Histogram of bilinear sampler required per request.  
    gpuStatistics::Statistic *magnifiedPixels;  //  Number of magnified pixels.  
    gpuStatistics::Statistic *mipsSampled[2];   //  Number of mipmaps sampled (per request).  
    gpuStatistics::Statistic *pointSampled;     //  Number of point sampled pixels.  

    //  Private functions.  
    /**
     *  Processes a Shader Command.
     *  @param cycle Current simulation cycle.
     *  @param shComm The ShaderCommand to process.
     */
    void processShaderCommand(U64 cycle, ShaderCommand *shComm);

    /**
     *
     *  Processes a texture unit register write.
     *
     *  @param cycle Current simulation cycle.
     *  @param reg The register to write into.
     *  @param subReg The register subregister to write into.
     *  @param data The data to write in the register.
     *
     */

    void processRegisterWrite(U64 cycle, GPURegister reg, U32 subReg, GPURegData data);

    /**
     *
     *  Processes a new Texture Request.
     *
     *  @param request The received texture request.
     *
     */

    void processTextureRequest(TextureRequest *request);

    /**
     *
     *  Processes a memory transaction received by the Texture Unit from the Memory Controller.
     *
     *  @param cycle The current simulation cycle.
     *  @param memTrans Pointer to the received memory transaction.
     *
     */

    void processMemoryTransaction(U64 cycle, MemoryTransaction *memTrans);

    /**
     *
     *  Send texture samples already filtered from the texture access queue
     *  back to the Shader Unit.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void sendTextureSample(U64 cycle);

    /**
     *
     *  Filters the texels from a texture access that has all the texels read from
     *  the texture cache.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void filter(U64 cycle);

    /**
     *
     *  Reads the texel from texture accesses in the texture access queue which have
     *  completed the fetching of all its texel.
     *
     */

    void read(U64 cycle);

    /**
     *
     *  Fetches texels for a texture access in the texture access queue.
     *
     */

    void fetch(U64 cycle);

    /**
     *
     *  Calculates the texture addresses for texture sample (up to trilinear supported):
     *
     */

    void calculateAddress(U64 cycle);

    /**
     *
     *  Generates a new texture access from one of the texture requests in the texture request queue.
     *
     */

    void genTextureAccess();

public:

    /**
     *
     *  Constructor for Texture Unit.
     *
     *  Creates and initializes a new Texture Unit mdu.
     *
     *  @param txEmu Reference to a bmoTextureProcessor object that will be
     *  used to emulate the Texture Unit functionality.
     *  @param frStamp Fragments per stamp.
     *  @param reqCycle Number of texture requests that can be received per cycle.
     *  @param reqQueueSz Texture request queue size.
     *  @param accQueueSz Texture access queue size.
     *  @param resQueueSz Texture result queue size.
     *  @param readWaitWSz Read wait window size.
     *  @param twoLevel Enables the two level texture cache version or the single level version.
     *  @param cacheLineSzL0 Size of the texture cache lines for L0 cache or single level cache.
     *  @param cacheWaysL0 Number of texture cache ways for L0 cache or single level cache.
     *  @param cacheLinesL0 Number of texture cache lines per way for L0 cache or single level cache.
     *  @param portWidth Width in bytes of the texture cache buses for L0 cache or single level cache.
     *  @param reqQSz Size of the texture cache request queue for L0 cache or single level cache.
     *  @param inputReqQSzL0 Size of the texture input request queue for L0 cache or single level cache.
     *  @param missesCycle Number of misses per cycle supported by the Texture Cache for L0 cache or single level cache.
     *  @param decomprLat Decompression latency for compressed texture cache lines for L0 cache or single level cache.
     *  @param cacheLineSzL1 Size of the texture cache lines for L1 cache (two level version).
     *  @param cacheWaysL1 Number of texture cache ways for L1 cache (two level version).
     *  @param cacheLinesL1 Number of texture cache lines per way for L1 cache (two level version).
     *  @param inputReqQSzL1 Size of the texture input request queue for L1 cache (two level version).
     *  @param addrALULat Latency for calculating the addresses for a bilinear sample.
     *  @param filterLat Latency for filtering a bilinear sample.
     *  @param name Name of the Texture Unit mdu.
     *  @param prefix String used to prefix the mdu signals names.
     *  @param parent Pointer to the parent mdu for this mdu.
     *
     *  @return A new Texture Unit object.
     *
     */

    cmoTextureProcessor(
        cgsArchConfig* ArchConf,
        bmoTextureProcessor*  BmTexProc,
        //U32          frStamp,
        //U32 reqCycle, U32 reqQueueSz, U32 accQueueSz,
        //U32 resQueueSz, U32 readWaitWSz, bool twoLevel, U32 cacheLineSzL0, U32 cacheWaysL0, U32 cacheLinesL0,
        //U32 portWidth, U32 reqQSz, U32 inputReqQSzL0, U32 missesCycle, U32 decomprLat,
        //U32 cacheLineSzL1, U32 cacheWaysL1, U32 cacheLinesL1, U32 inputReqQSzL1,
        //U32 addrALULat, U32 filterLat, 
        char *name, char *prefix = 0, cmoMduBase* parent = 0);

    /**
     *
     *  Texture Unit destructor.
     *
     */
     
    ~cmoTextureProcessor();
     
    /**
     *
     *  Clock rutine.
     *
     *  Carries the simulation of the Texture Unit.  It is called
     *  each clock to perform the simulation.
     *
     *  @param cycle  Cycle to simulate.
     *
     */

    void clock(U64 cycle);

    /**
     *
     *  Sets the debug flag for the mdu.
     *
     *  @param debug New value of the debug flag.
     *
     */

    void setDebugMode(bool debug);

    /**
     *
     *  Returns a single line string with state and debug information about the
     *  Texture Unit mdu.
     *
     *  @param stateString Reference to a string where to store the state information.
     *
     */

    void getState(string &stateString);

    /**
     *
     *  Writes into a string a report about the stall condition of the mdu.
     *
     *  @param cycle Current simulation cycle.
     *  @param stallReport Reference to a string where to store the stall state report for the mdu.
     *
     */
     
    void stallReport(U64 cycle, string &stallReport);
    
    /**
     *
     *  Prints information about the TextureAccess object.
     *
     *  @param texAccess Pointer to the TextureAccess object for which to print information.
     *
     */
     
    void printTextureAccessInfo(TextureAccess *texAccess);
};

} // namespace cg1gpu

#endif
