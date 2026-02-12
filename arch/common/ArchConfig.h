/**************************************************************************
 *
 * ConfigLoader Definition file.
 *  This file defines the ConfigLoader class that loads and parses
 *  the simulator configuration file.
 *
 */

#ifndef _ARCHCONFIG_
#define _ARCHCONFIG_
/*
    Needed for using getline (GNU extension).
    Compatibility problems??
    Must be the first include because Parser is also including it.
*/

#include <cstdio>
#include <map>
#include <string>

#include "GPUType.h"
// #include "Parser.h"  // removed: no longer needed

namespace cg1gpu
{

//**  The initial size of the buffer for the Signal section.  
static const U32 SIGNAL_BUFFER_SIZE = 200;

/**
 *
 *  Defines the configuration file sections.
 *
 */

enum Section
{
    SEC_SIM,    //  Simulator section.  Global simulator parameters.  
    SEC_GPU,    //  GPU section.  Global architecture parameters.  
    SEC_COM,    //  Command Processor section.  Command Processor parameters.  
    SEC_MEM,    //  Memory Controller section.  Memory Controller parameters.  
    SEC_STR,    //  cmoStreamController section.  cmoStreamController parameters.  
    SEC_VSH,    //  Vertex Shader section.  Vertex Shader parameters.  
    SEC_PAS,    //  Primitive Assembly section.  Primitive Assembly parameters.  
    SEC_CLP,    //  Clipper section.  Cliper parameters.  
    SEC_RAS,    //  Rasterizer section.  Rasterizer parameters.  
    SEC_FSH,    //  Fragment Shader section.  Fragment shader parameters.  
    SEC_ZST,    //  Z and Stencil Test section.  Z and Stencil test parameters.  
    SEC_CWR,    //  Color Write section.  Color Write parameters.  
    SEC_DAC,    //  DisplayController section.  DisplayController parameters.  
    SEC_SIG,    //  Signal section.  Signal parameters.  
    SEC_UKN     //  Unknown section.  
};

/**
 *
 *  Defines the parameters for a signal.
 *
 */

struct SigParameters
{
    char *name;         //  Signal name.  
    U32 bandwidth;   //  Signal bandwidth.  
    U32 latency;     //  Signal latency.  
};

/**
 *
 *  Defines the GPU architecture parameters.
 *
 */

struct GPUParameters
{
    U32 numVShaders;         //  Number of vertex shader units in the GPU.  
    U32 numFShaders;         //  Number of fragment shader units in the GPU.  
    U32 numStampUnits;       //  Number of stamp units (Z Stencil + Color Write) in the GPU.  
    U32 gpuClock;            //  Frequency of the GPU (main) domain clock in MHz.  
    U32 shaderClock;         //  Frequency of the shader domain clock in MHz.  
    U32 memoryClock;         //  Frequency of the memory domain clock in MHz.   
};

/**
 *
 *  Defines the Command Processor parameters.
 *
 */

struct ComParameters
{
    bool pipelinedBatches;      //  Enable/disable pipelined batch rendering.  
    bool dumpShaderPrograms;    //  Enable/disable dumping shader programs being loaded to files.  
};


/**
 *
 *  Defines the Memory Controller parameters.
 *
 */

struct MemParameters
{
    bool memoryControllerV2;//  Specifies which MC will be selected (FALSE = Legacy MC). 
    U32 memSize;         //  GPU memory size in bytes.  
    U32 clockMultiplier; //  Frequency multiplier applied to the GPU clock to be used as memory reference clock.  
    U32 memoryFrequency; //  GPU memory frequency in cycles from the memory reference clock.  
    U32 busWidth;        //  GPU memory bus width in bits.  
    U32 memBuses;        //  Number of buses to the memory modules.  
    bool sharedBanks;       //  Enables shared access to memory banks from all the gpu memory buses.  
    U32 bankGranurality; //  Access granurality for distributed memory banks in bytes.  
    U32 burstLength;     //  Burst length of accesses (bus width) to GPU memory).  
    U32 readLatency;     //  Cycles from read command to data from GPU memory.  
    U32 writeLatency;    //  Cycles from write command to data to GPU memory.  
    U32 writeToReadLat;  //  Cycles from last written data to GPU memory to next read command.  
    U32 memPageSize;     //  Size of a GPU memory page.  
    U32 openPages;       //  Number of open memory pages per bus.  
    U32 pageOpenLat;     //  Latency of opening a new memory page.  
    U32 maxConsecutiveReads; //  Number of consecutive read transactions before the next write transaction.  
    U32 maxConsecutiveWrites;//  Number of consecutive write transactions before the next read transaction.  
    U32 comProcBus;      //  Command Processor memory bus width in bytes.  
    U32 strFetchBus;     //  cmoStreamController Fetch memory bus width in bytes.  
    U32 strLoaderBus;    //  cmoStreamController Loader memory bus width in byts.  
    U32 zStencilBus;     //  Z Stencil Test memory bus width in bytes.  
    U32 cWriteBus;       //  Color Write memory bus width in bytes.  
    U32 dacBus;          //  DisplayController memory bus width in bytes.  
    U32 textUnitBus;     //  Texture Unit memory bus width in bytes.  
    U32 mappedMemSize;   //  Amount of system memory mapped into the GPU address space in bytes.  
    U32 readBufferLines; //  Number of buffer lines for read transactions.  
    U32 writeBufferLines;//  Number of buffer lines for write transactions.  
    U32 reqQueueSz;      //  Request queue size.  
    U32 servQueueSz;     //  Service queue size.  

    bool v2UseChannelRequestFIFOPerBank; ///< Use N queues per channel (being N = # of banks) to issue channel transactions to channel schedulers
    U32 v2ChannelRequestFIFOPerBankSelection; ///< Algorithm used to select a bank to issue the next channel transation to channel scheduler

    /// Parameters exclusive for Memory Controller V2
    bool v2MemoryTrace; // Tells the Memory Controller V2 to generate a file with the memory transactions received 
    U32 v2MemoryChannels; // Number of channels available (equivalent to old MemoryBuses) 
    U32 v2BanksPerMemoryChannel; // Number of banks per channel (ie: per chip module) 
    U32 v2MemoryRowSize; // Size in bytes of a page(row), equivalent to old MemoryPageSize 
    
    /**
     * 32bit data elements of a burst served per cycle
     * @note in DDR chips the usual value is 2
     * @note Greater values can be used to emulate several chips attached to one channel
     */
    // U32 v2BurstElementsPerCycle;

    /**
     * Bytes of a burst read/written per cycle
     * @note in DDR chips the usual value is 8B/cycle
     * @note Greater values can be used to emulate several chips attached to one channel
     * @note Lower values can be used to emulate slower memories 
     */
    U32 v2BurstBytesPerCycle;

    // Interleaving of memory at channel & bank level
    U32 v2ChannelInterleaving;
    U32 v2BankInterleaving;

    bool v2SecondInterleaving;
    U32 v2SecondChannelInterleaving;
    U32 v2SecondBankInterleaving;
    U32 v2SplitterType;
    char* v2ChannelInterleavingMask;
    char* v2BankInterleavingMask;
    char* v2SecondChannelInterleavingMask;
    char* v2SecondBankInterleavingMask;
    char* v2BankSelectionPolicy;
    // Maximum number of in flight channel transactions per channel
    U32 v2MaxChannelTransactions;
    U32 v2DedicatedChannelReadTransactions;
    U32 v2ChannelScheduler;// Type of channel scheduler. 
    U32 v2PagePolicy;        // Page policy selected. 

    char* v2MemoryType; // should be defined as "gddr3"

    char* v2GDDR_Profile; // "custom", "600", "700" and so on

    U32 v2GDDR_tRRD;
    U32 v2GDDR_tRCD;
    U32 v2GDDR_tWTR;
    U32 v2GDDR_tRTW;
    U32 v2GDDR_tWR;
    U32 v2GDDR_tRP;
    U32 v2GDDR_CAS;
    U32 v2GDDR_WL;

    U32 v2SwitchModePolicy;
    U32 v2ActiveManagerMode;
    U32 v2PrechargeManagerMode;

    bool v2DisablePrechargeManager; 
    bool v2DisableActiveManager;
    U32 v2ManagerSelectionAlgorithm; // 0=active-then-precharge, 1=precharge-then-active, other-more-intelligent-aproaches

    char* v2DebugString;
    bool v2UseClassicSchedulerStates;

    bool v2UseSplitRequestBufferPerROP;

    // for compatibility with previous configuration files
    // By default disables the new memory controller
    // new option v2MaxChannelTransactions set with a default value to prevent
    // malfunction with previous "CG1GPU.ini" files
    MemParameters() : memoryControllerV2(false), 
                      v2MemoryTrace(false),
                      v2MaxChannelTransactions(8), 
                      v2SecondInterleaving(false), // disabled by default
                      v2SplitterType(0), // By default use legacy splitter
                      v2ChannelInterleavingMask(0),
                      v2BankInterleavingMask(0),
                      v2SecondChannelInterleavingMask(0),
                      v2SecondBankInterleavingMask(0),
                      v2BankSelectionPolicy(0),
                      v2MemoryType(0),
                      v2GDDR_Profile(0), // Use default
                      v2DebugString(0)
    {}
};


/**
 *
 *  Defines the cmoStreamController parameters.
 *
 *
 */

struct StrParameters
{
    U32 indicesCycle;        //  Indices to be processed by the cmoStreamController per cycle.  
    U32 idxBufferSize;       //  Index buffer size.  
    U32 outputFIFOSize;      //  Output FIFO size.  
    U32 outputMemorySize;    //  Output Memory Size.  
    U32 verticesCycle;       //  Number of vertices sent to Primitive Assembly per cycle.  
    U32 attrSentCycle;       //  Number of shader output attributes that can be sent per cycle to Primitive assembly.  
    U32 streamerLoaderUnits; //  Number of cmoStreamController Loader Units for the load of attribute data. 
   
    // Individual cmoStreamController Loader Unit parameters.  
  
    U32 slIndicesCycle;        //  Indices per cycle per cmoStreamController Loader unit.  
    U32 slInputReqQueueSize;   //  Input request queue size.  
    U32 slAttributesCycle;     //  Shader input attributes filled with data per cycle.  
    U32 slInCacheLines;        //  Lines in the input cache.  
    U32 slInCacheLineSz;       //  Input cache line size in bytes.  
    U32 slInCachePortWidth;    //  Width in bytes of the input cache read and write ports.  
    U32 slInCacheReqQSz;       //  Input cache request queue size.  
    U32 slInCacheInputQSz;     //  Input cache input request queue size.  
};


/**
 *
 *  Defines the Vertex Shader parameters.
 *
 *
 */

//struct VShParameters
//{
//    U32 numThreads;      //  Number of execution threads per shader.  
//    U32 numInputBuffers; //  Number of input buffers per shader.  
//    U32 numResources;    //  Number of thread resources (registers?) per shader.  
//    U32 threadRate;      //  Vertex shader threads from which to fetch per cycle.  
//    U32 fetchRate;       //  Vertex Shader instructions fetched per cycle and thread.  
//    U32 threadGroup;     //  Number of threads that form a lock step group.  
//    bool lockedMode;        //  Threads execute in lock up mode flag.  
//    bool scalarALU;         //  Enable a scalar ALU in parallel with the SIMD ALU.  
//    bool threadWindow;      //  Enables the fetch unit to search for a ready thread in a window.  
//    U32 fetchDelay;      //  Delay in cycles until the next instruction is fetched from a thread.  
//    bool swapOnBlock;       //  Enables swap thread on block or swap thread always.  
//    U32 inputsCycle;     //  Vertex inputs that can be received per cycle.  
//    U32 outputsCycle;    //  Vertex outputs that can be sent per cycle.  
//    U32 outputLatency;   //  Vertex ouput transmission latency.  
//};


/**
 *
 *  Defines the Primitive Assembly parameters.
 *
 */

struct PAsParameters
{
    U32 verticesCycle;       //  Number of vertices received per cycle from the cmoStreamController.  
    U32 trianglesCycle;      //  Triangles per cycle sent to the Clipper.  
    U32 inputBusLat;         //  Latency of the input vertex bus from the cmoStreamController.  
    U32 paQueueSize;         //  Size in vertices of the assembly queue.  
};

/**
 *
 *  Defines the Clipper parameters.
 *
 */

struct ClpParameters
{
    U32 trianglesCycle;      //  Triangles received from Primitive Assembly and sent to Triangle Setup per cycle.  
    U32 clipperUnits;        //  Clipper test units.  
    U32 startLatency;        //  Triangle clipping start latency.  
    U32 execLatency;         //  Triangle clipping execution latency.  
    U32 clipBufferSize;      //  Clipped triangle buffer size.  
};

/**
 *
 *  Defines the Rasterizer parameters.
 *
 */

struct RasParameters
{
    U32 trianglesCycle;          //  Number of triangles per cycle.  
    U32 setupFIFOSize;           //  Triangle Setup FIFO size.  
    U32 setupUnits;              //  Number of triangle setup units.  
    U32 setupLat;                //  Latency of the triangle setup units.  
    U32 setupStartLat;           //  Start latency of the triangle setup units.  
    U32 trInputLat;              //  Triangle input bus latency (from Primitive Assembly/Clipper).  
    U32 trOutputLat;             //  Triangle output bus latency (between Triangle Setup and TriangleTraversal/Fragment Generation).  
    bool shadedSetup;               //  Triangle setup performed in the unified shader.  
    U32 triangleShQSz;           //  Size of the triangle shader queue in Triangle Setup.  
    U32 stampsCycle;             //  Stamps per cycle.  
    U32 samplesCycle;            //  Number of MSAA samples that can be generated per fragment and cycle in Triangle Traversal/Fragment Generation.  
    U32 overScanWidth;           //  Scan over tile width in scan tiles.  
    U32 overScanHeight;          //  Scan over tile height in scan tiles.  
    U32 scanWidth;               //  Scan tile width in fragments.  
    U32 scanHeight;              //  Scan tile height in fragments.  
    U32 genWidth;                //  Generation tile width in fragments.  
    U32 genHeight;               //  Generation tile heigh in fragments.  
    U32 rastBatch;               //  Number of triangles per rasterization batch (Triangle Traversal).  
    U32 batchQueueSize;          //  Number of triangles in the batch queue (Triangle Traversal).  
    bool recursive;                 //  Determines the rasterization method (TRUE: recursive, FALSE: scanline) (Triangle Traversal).  
    bool disableHZ;                 //  Disables the Hierarchical Z.  
    U32 stampsHZBlock;           //  Stamps per Hierarchical Z block.  
    U32 hzBufferSize;            //  Size of the Hierarchical Z buffer in blocks.  
    U32 hzCacheLines;            //  Lines in the HZ cache.  
    U32 hzCacheLineSize;         //  Block per HZ cache line.  
    U32 earlyZQueueSz;           //  Size of the Hierarchical/Early Z test queue.  
    U32 hzAccessLatency;         //  Access latency to the Hierarchical Z Buffer.  
    U32 hzUpdateLatency;         //  Latency of the update signal to the Hierarchical Z.  
    U32 hzBlockClearCycle;       //  Number of Hierarchical Z blocks cleared per cycle.  
    U32 numInterpolators;        //  Number of attribute interpolators.  
    U32 shInputQSize;            //  Shader input queue size (per shader unit).  
    U32 shOutputQSize;           //  Shader output queue size (per shader unit).  
    U32 shInputBatchSize;        //  Shader inputs per shader unit assigned work batch.  
    bool tiledShDistro;             //  Enable/disable distribution of fragment inputs to the shader units on a tile basis.  
    U32 vInputQSize;             //  Vertex input queue size (Fragment FIFO) (only for unified shader version).  
    U32 vShadedQSize;            //  Shaded vertex queue size (Fragment FIFO) (only for unified shader version).  
    U32 trInputQSize;            //  Triangle input queue size (Fragment FIFO) (only for triangle setup on shaders).  
    U32 trOutputQSize;           //  Triangle output queue size (Fragment FIFO) (only for triangle setup on shaders).  
    U32 genStampQSize;           //  Generated stamp queue size (Fragment FIFO) (per stamp unit).  
    U32 testStampQSize;          //  Early Z tested stamp queue size (Fragment FIFO).  
    U32 intStampQSize;           //  Interpolated stamp queue size (Fragment FIFO).  
    U32 shadedStampQSize;        //  Shaded stamp queue size (Fragment FIFO) (per stamp unit).  
    U32 emuStoredTriangles;      //  Number of triangles that can be kept stored in the rasterizer behaviorModel.  

    //   MicroPolygon Rasterizer parameters.
    bool useMicroPolRast;           //  Use the MicroPolygon Rasterizer.  
    bool preBoundTriangles;         //  Triangles are bound (compute its area and BB) prior to edge equation setup, enabling the Triangle Bound unit (between Clipper and Triangle Setup).  
    U32 trBndOutLat;             //  Triangle bus latency between Triangle Bound and Triangle Setup.  
    U32 trBndOpLat;              //  Latency of a Triangle Bound operation (mainly compute the triangle bounding box).  
    U32 trBndLargeTriFIFOSz;     //  Size of the TriangleBound unit queue storing large triangles to be sent to Triangle Setup (shared path).    
    U32 trBndMicroTriFIFOSz;     //  Size of the TriangleBound unit queue storing microtriangles to be sent to Shader Work Distributor (independent path).   
    bool useBBOptimization;         //  Use the bounding box size optimization pass.  
    U32 subPixelPrecision;       //  Number of bits used in the decimal part of fixed point operations to compute the subpixel positions.  
    F32 largeTriMinSz;           //  The minimum area to consider a large triangle.  
    bool cullMicroTriangles;        //  Removes all detected microtriangles (according to the microtriangle size limit specified). Used for debug purposes.  
    bool microTrisAsFragments;      //  Process micro triangles directly as fragments, skipping setup and traversal operations on the triangle.  
    U32 microTriSzLimit;         //  0: 1-pixel bound triangles only, 1: 1-pixel and 1-stamp bound triangles, 2: Any triangle bound to 2x2, 1x4 or 4x1 stamps.  
    char* microTriFlowPath;         //  "shared": Use the same data path as normal (macro) triangles, "independent": use separate path for micro - allow z stencil and color write disorder.  
    bool dumpBurstHist;             //  Dump of micro and macrotriangle burst size histogram is enabled/disabled.  
};

/**
 *
 *  Defines the Fragment Shader parameters.
 *
 */

struct cgsUnifiedShaderParam
{
    //  Legacy shader parameters
    U32 numThreads;      //  Number of execution threads per shader.  
    U32 numInputBuffers; //  Number of input buffers per shader.  
    U32 numResources;    //  Number of thread resources (registers?) per shader.  
    U32 threadRate;      //  Fragment shader threads from which to fetch per cycle.  
    bool scalarALU;         //  Enable a scalar ALU in parallel with the SIMD ALU.  
    U32 fetchRate;       //  Fragment Shader instructions fetched per cycle and thread.  
    U32 threadGroup;     //  Number of threads that form a lock step group.  
    bool lockedMode;        //  Threads execute in lock up mode flag.  

    //  Vector shader parameters
    bool  useVectorShader;     //  Flags to use the Vector Shader model for the unified shader.  
    U32   vectorThreads;       //  Number of vector threads supported by the vector shader.  
    U32   vectorResources;     //  Number of resources (per vector thread) available in the vector shader.  
    U32   vectorLength;        //  Number of elements in a vector.  
    U32   vectorALUWidth;      //  Number of ALUs in the vector ALU array.  
    char* vectorALUConfig;     //  Configuration of the element ALUs in the vector array.  
    bool  vectorWaitOnStall;   //  Flag that defines if the instruction waits for stall conditions to be cleared or is discarded and repeated (refetched).  
    bool  vectorExplicitBlock; //  Flag that defines if threads are blocked by an explicit trigger in the instruction stream or automatically on texture load.  

    //  Common shader paramters
    bool vAttrLoadFromShader;   //  Vertex attribute load is performed in the shader (LDA instruction).  
    bool threadWindow;      //  Enables the fetch unit to search for a ready thread in a window.  
    U32 fetchDelay;      //  Delay in cycles until the next instruction is fetched from a thread.  
    bool swapOnBlock;       //  Enables swap thread on block or swap thread always.  
    bool fixedLatencyALU;   //  Sets if the all the latency of the ALU is always the same for all the instructions.  
    U32 inputsCycle;     //  Fragment inputs that can be received per cycle.  
    U32 outputsCycle;    //  Fragment outputs that can be sent per cycle.  
    U32 outputLatency;   //  Fragment ouput transmission latency.  
    U32 textureUnits;    //  Number of texture units attached to the shader.  
    U32 textRequestRate; //  Texture request rate to the texture units.  
    U32 textRequestGroup;//  Size of a group of texture requests sent to a texture unit.  
    
    //  Texture unit parameters.  
    U32 addressALULat;   //  Latency of the address ALU.  
    U32 filterLat;       //  Latency of the filter unit.  
    U32 anisoAlgo;       //  Anisotropy detection algorithm to be used.  
    bool forceMaxAniso;     //  Flag used to force the maximum anisotropy from the configuration file to all textures.  
    U32 maxAnisotropy;   //  Maximum anisotropy ratio allowed.  
    U32 triPrecision;    //  Trilinear fractional precision.  
    U32 briThreshold;    //  Brilinear threshold.  
    U32 anisoRoundPrec;  //  Aniso round precision.  
    U32 anisoRoundThres; //  Aniso round threshold.  
    bool anisoRatioMultOf2; //  Aniso ratio must be a multiple of two.  
    U32 texBlockDim;    //  Texture block/line dimension in texels (2^n x 2^n).  
    U32 texSuperBlockDim;   //  Texture super block dimension in blocks (2^m x 2^m).  
    U32 textReqQSize;    //  Texture request queue size.  
    U32 textAccessQSize; //  Texture access queue size.  
    U32 textResultQSize; //  Texture result queue size.  
    U32 textWaitWSize;   //  Texture wait read window size.  
    bool twoLevelCache;     //  Enables the two level texture cache or the single level.  
    U32 txCacheLineSz;   //  Size of a texture cache line in bytes (L0 for two level version).  
    U32 txCacheWays;     //  Ways in the texture cache  (L0 for two level version).  
    U32 txCacheLines;    //  Lines per way in the texture cache  (L0 for two level version).  
    U32 txCachePortWidth;//  Width in bytes of the texture cache read/write ports.  
    U32 txCacheReqQSz;   //  Texture Cache request queue size.  
    U32 txCacheInQSz;    //  Texture Cache input queue size  (L0 for two level version).  
    U32 txCacheMissCycle;//  Texture Cache misses supported per cycle.  
    U32 txCacheDecomprLatency;   //  Texture Cache compressed texture line decompression latency.  
    U32 txCacheLineSzL1; //  Size of a texture cache line in bytes (L1 for two level version).  
    U32 txCacheWaysL1;   //  Ways in the texture cache (L1 for two level version).  
    U32 txCacheLinesL1;  //  Lines per way in the texture cache (L1 for two level version).  
    U32 txCacheInQSzL1;  //  Texture Cache input queue size (L1 for two level version).  

};


/**
 *
 *  Defines the Z Stencil Test parameters.
 *
 */
struct ZSTParameters
{
    U32 stampsCycle;     //  Stamps per cycle.  
    U32 bytesPixel;      //  Bytes per pixel (depth).  
    bool disableCompr;      //  Disables Z compression.  
    U32 zCacheWays;      //  Ways in the Z Cache.  
    U32 zCacheLines;     //  Lines per way in the Z cache.  
    U32 zCacheLineStamps;//  Stamps per Z Cache line.  
    U32 zCachePortWidth; //  Width in bytes of the Z Cache ports.  
    bool extraReadPort;     //  Adds an additional read port to the Color Cache.  
    bool extraWritePort;    //  Adds an additional write port to the Color Cache.  
    U32 zCacheReqQSz;    //  Z Cache request queue size.  
    U32 zCacheInQSz;     //  Z Cache input queue size.  
    U32 zCacheOutQSz;    //  Z Cache output queue size.  
    U32 blockStateMemSz; //  Size of the block state memory (in blocks).  
    U32 blockClearCycle; //  Blocks cleared per cycle.  
    U32 comprLatency;    //  Block compression latency.  
    U32 decomprLatency;  //  Block decompression latency.  
    U32 comprAlgo;       //  Compression algorithm id. 
    //U32 zQueueSz;        //  Size of the Z/Stencil Test stamp queue.  
    U32 inputQueueSize;  //  Size of the input Z/Stencil Test stamp queue.  
    U32 fetchQueueSize;  //  Size of the fetched Z/Stencil Test stamp queue.  
    U32 readQueueSize;   //  Size of the read Z/Stencil Test stamp queue.  
    U32 opQueueSize;     //  Size of the operated Z/Stencil Test stamp queue.  
    U32 writeQueueSize;  //  Size of the written Z/Stencil Test stamp queue.  
    U32 zTestRate;       //  Rate of stamp testing (cycles between two stamps).  
    U32 zOpLatency;      //  Latency of Z/Stencil Test.  
};

/**
 *
 *  Defines the Color Write parameters.
 *
 */
struct CWRParameters
{
    U32 stampsCycle;     //  Stamps per cycle.  
    U32 bytesPixel;      //  Bytes per pixel (color).  
    bool disableCompr;      //  Disables color compression.  
    U32 cCacheWays;      //  Ways in the Color Cache.  
    U32 cCacheLines;     //  Lines per way in the Color cache.  
    U32 cCacheLineStamps;//  Stamps per Color Cache line.  
    U32 cCachePortWidth; //  Width in bytes of the Color Cache ports.  
    bool extraReadPort;     //  Adds an additional read port to the Color Cache.  
    bool extraWritePort;    //  Adds an additional write port to the Color Cache.  
    U32 cCacheReqQSz;    //  Color Cache request queue size.  
    U32 cCacheInQSz;     //  Color Cache input queue size.  
    U32 cCacheOutQSz;    //  Color Cache output queue size.  
    U32 blockStateMemSz; //  Size of the block state memory (in blocks).  
    U32 blockClearCycle; //  Blocks cleared per cycle.  
    U32 comprLatency;    //  Block compression latency.  
    U32 decomprLatency;  //  Block decompression latency.  
    U32 comprAlgo;       //  Compression algorithm id. 
    //U32 colorQueueSz;    //  Size of the Color/Blend stamp queue.  
    U32 inputQueueSize;  //  Size of the input Color Write stamp queue.  
    U32 fetchQueueSize;  //  Size of the fetched Color Write stamp queue.  
    U32 readQueueSize;   //  Size of the read Color Write stamp queue.  
    U32 opQueueSize;     //  Size of the operated Color Write stamp queue.  
    U32 writeQueueSize;  //  Size of the written Color Write stamp queue.  
    U32 blendRate;       //  Rate for stamp blending (cycles between two stamps).  
    U32 blendOpLatency;  //  Latency of blend operation.  
};

/**
 *
 *  Defines the DisplayController parameters.
 *
 */
struct cgsDisplayCtrlParam
{
    U32 bytesPixel;      //  Bytes per pixel (color).  
    U32 blockSize;       //  Size of an uncompressed block in bytes.  
    U32 blockUpdateLat;  //  Latency of the color block update signal.  
    U32 blocksCycle;     //  Blocks that can be update/written per cycle in the state memory.  
    U32 blockReqQSz;     //  Block request queue size.  
    U32 decomprLatency;  //  Block decompression latency.  
    U64 refreshRate;     //  Frame refresh/dumping rate in cycles.  
    bool synchedRefresh;    //  Swap is syncrhonized with frame refresh/dumping.  
    bool refreshFrame;      //  Dump/refresh the frame into a file.  
    bool saveBlitSource;    //  Save the source data for blit operations to a PPM file.  
};

struct cgsSimulationParam
{
    U64 simCycles;       //  Cycles to simulate.  
    U32 simFrames;       //  Number of cycles to simulate.  
    char *inputFile;        //  Simulator trace input file.  
    char *signalDumpFile;   //  Name of the signal trace dump file.  
    char *statsFile;        //  Name of the statistics file.  
    char *statsFilePerFrame;//  Name of the per frame statistics file.  
    char *statsFilePerBatch;//  Name of the per batch statistics file.  
    U32 startFrame;      //  First frame to simulate.  
    U64 startDump;       //  First cycle to dump signal trace.  
    U64 dumpCycles;      //  Number of cycles to dump signal trace.  
    U32 statsRate;       //  Rate (in cycles) at which statistics are updated.  
    bool dumpSignalTrace;   //  Enables signal trace dump.  
    bool statistics;        //  Enables the generation of statistics.  
    bool perFrameStatistics;//  Enable/disable per frame statistics generation.  
    bool perBatchStatistics;//  Enable/disable per batch statistics generation.  
    bool perCycleStatistics;//  Enable/disable per cycle statistics generation.  
    bool detectStalls;      //  Enable/disable stall detection.  
    bool fragmentMap;       //  Generate the fragment propierty map.  
    U32 fragmentMapMode; //  Fragment map mode: 0 => fragment latency from generation to color write/blend.  
    bool colorDoubleBuffer;      //  Enables double buffer (front/back) for the color frame buffer.  
    bool forceMSAA;         //  Forces multisampling for the whole trace.  
    U32 msaaSamples;     //  Number of MSAA samples per pixel when multisampling is forced in the configuration file.  
    bool forceFP16ColorBuffer;   //  Force float16 color buffer. 
    bool enableDriverShaderTranslation;   //  Enables shader program translation in the driver.  
    U32 objectSize0;     //  Size in bytes of the objects in the optimized dynamic memory bucket 0.  
    U32 objectSize1;     //  Size in bytes of the objects in the optimized dynamic memory bucket 1.  
    U32 objectSize2;     //  Size in bytes of the objects in the optimized dynamic memory bucket 2.  
    U32 bucketSize0;     //  Number of objects in the optimized dynamic memory bucket 0;  
    U32 bucketSize1;     //  Number of objects in the optimized dynamic memory bucket 1;  
    U32 bucketSize2;     //  Number of objects in the optimized dynamic memory bucket 2;  
    U32 profilingLevel;

};

/**
 *  Defines the simulator parameters.
 */

struct cgsArchConfig
{
    //  Simulator parameters.  
    cgsSimulationParam sim;

    //  Per gpu unit parameters.  
    GPUParameters gpu;      //  GPU architecture parameters.  
    ComParameters com;      //  Command Processor parameters.  
    MemParameters mem;      //  Memory Controller parameters.  
    StrParameters str;      //  cmoStreamController parameters.  
    PAsParameters pas;      //  Primitive Assembly parameters.  
    ClpParameters clp;      //  Clipper parameters.  
    RasParameters ras;      //  Rasterizer parameters.  
    cgsUnifiedShaderParam ush;      //  Vertex Shader parameters.  
    //cgsUnifiedShaderParam ush;      //  Fragment Shader parameters.  
    ZSTParameters zst;      //  Z Stencil test parameters.  
    CWRParameters cwr;      //  Color Write parameters.  
    cgsDisplayCtrlParam dac;      //  DisplayController parameters.  
    SigParameters *sig;     //  Signal parameters.  
    U32 numSignals;      //  Number of signal (parameters).  
};

}  // namespace cg1gpu

#endif // _ARCHCONFIG_
