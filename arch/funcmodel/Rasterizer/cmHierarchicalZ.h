/**************************************************************************
 *
 * Hierarchical Z class definition file.
 *
 */

/**
 *
 *  @file cmHierarchicalZ.h
 *
 *  This file defines the Hierarchical Z class.
 *
 *  This class implements a Hierarchical Z simulation mdu.
 *
 */


#ifndef _HIERARCHICALZ_

#define _HIERARCHICALZ_

#include "cmMduBase.h"
#include "GPUType.h"
#include "support.h"
#include "GPUReg.h"
#include "cmFragmentInput.h"
#include "cmRasterizerState.h"
#include "cmRasterizerCommand.h"
#include "PixelMapper.h"
#include "cmtoolsQueue.h"

namespace cg1gpu
{

/**
 *
 *  Defines the Hierarchical Z states for the Triangle Traversal.
 *
 */

enum HZState
{
    HZST_READY,         //  The Hierarchical Z Early test can receive new stamps.  
    HZST_BUSY           //  The Hierarchical Z Early test can not receive new stamps.  
};

/**
 *
 *  This structure defines a line in the Hierachical Z Buffer Cache.
 *
 */

struct HZCacheLine
{
    U32 block;       //  Identifier of the line of blocks stored in the cache.  
    U32 *z;          //  The z value stored for each block in the line.  
    U32 reserves;    //  Number of reserves for the cache entry.  
    bool read;          //  The cache entry has received the line from the HZ buffer.  
    bool valid;         //  If the cache line is storing valid data.  
};

/**
 *
 *  Defines the maximum number of HZ blocks that can be required to evaluate a stamp tile.
 *
 */
const U32 MAX_STAMP_BLOCKS = 4;

/**
 *
 *  Defines an entry in the stamp queue for the Hierarchical Early
 *  Z Test.
 *
 */
struct HZQueue
{
    FragmentInput **stamp;          //  Stores a fragment stamp.  
    U32 numBlocks;               //  Number of HZ blocks that are required to evaluate the stamp.  
    U32 block[MAX_STAMP_BLOCKS]; //  Hierarchical Z block for the stamp.  
    U32 stampZ;                  //  Minimum Z value of the stamp.  
    U32 currentBlock;            //  Pointer to the block that is being processed.  
    U32 cache[MAX_STAMP_BLOCKS]; //  Pointer to the cache entry where the stamp block Z value will be stored.  
    U32 blockZ;                  //  Value of the blocks z.  
    bool culled;                    //  Stores if the stamp has been culled.  
};

/**
 *
 *  This class implements the Hierarchical Z mdu.
 *
 *  The Hierarchical Z mdu simulates a Hierarchical Z buffer
 *  for a GPU.
 *
 *  Inherits from the cmoMduBase class that offers basic simulation
 *  support.
 *
 */


class HierarchicalZ : public cmoMduBase
{
    //  Defines a entry of the microStampQueue.
    struct StampInput
    {
        FragmentInput* fragment[STAMP_FRAGMENTS];
    };

private:

    //  Hierarchical Z Signals.  
    Signal *inputStamps;        //  Input stamp signal from Triangle Traversal.  
    Signal *outputStamps;       //  Output stamp signal to Interpolator.  
    Signal *fFIFOState;         //  State signal from the Fragment FIFO mdu.  
    Signal *hzTestState;        //  State signal to the Triangle Traversal mdu.  
    Signal *hzCommand;          //  Command signal from the main rasterizer mdu.  
    Signal *hzState;            //  Command signal to the main rasterizer mdu.  
    Signal **hzUpdate;          //  Array of update signals from the Z Test mdu.  
    Signal *hzBufferWrite;      //  Write signal for the HZ Buffer Level 0.  
    Signal *hzBufferRead;       //  Read signal for the HZ Buffer Level 0.  

    //  Hierarchical Z parameters.  
    U32 stampsCycle;              //  Number of stamps per cycle.  
    U32 overW;                    //  Over scan tile width in scan tiles.  
    U32 overH;                    //  Over scan tile height in scan tiles.  
    U32 scanW;                    //  Scan tile width in generation tiles.  
    U32 scanH;                    //  Scan tile height in generation tiles.  
    U32 genW;                     //  Generation tile width in stamps.  
    U32 genH;                     //  Generation tile height in stamps.  
    bool disableHZ;                  //  Disables HZ support.  
    U32 blockStamps;              //  Stamps per HZ block.  
    U32 hzBufferSize;             //  Size of the Hierarchical Z Buffer (number of blocks stored).  
    U32 hzCacheLines;             //  Number of lines in the fast access Hierarchical Z Buffer cache.  
    U32 hzCacheLineSize;          //  Number of blocks per HZ Cache line.  
    U32 hzQueueSize;              //  Size of the stamp queue for the Early test.  
    U32 hzBufferLatency;          //  Access latency to the HZ Buffer Level 0.  
    U32 hzUpdateLatency;          //  Update signal latency (from Z Stencil Test).  
    U32 clearBlocksCycle;         //  Number of blocks that can be cleared per cycle.  
    U32 numStampUnits;            //  Number of stamp units attached to the Rasterizer.  

    //  Hierarchical Z constants.  
    U32 blockShift;          //  Bits of a fragment address corresponding not related to the block.  
    U32 blockSize;           //  Size in depth elements (32 bit) of a HZ block.  
    U32 blockSizeMask;       //  Bit mask for the block size.  

    //  Hierarchical Z registers.  
    U32 hRes;                //  Display horizontal resolution.  
    U32 vRes;                //  Display vertical resolution.  
    S32 startX;              //  Viewport initial x coordinate.  
    S32 startY;              //  Viewport initial y coordinate.  
    U32 width;               //  Viewport width.  
    U32 height;              //  Viewport height.  
    bool scissorTest;           //  Enables scissor test.  
    S32 scissorIniX;         //  Scissor initial x coordinate.  
    S32 scissorIniY;         //  Scissor initial y coordinate.  
    U32 scissorWidth;        //  Scissor width.  
    U32 scissorHeight;       //  Scissor height.  
    bool hzEnabled;             //  Early Z test enable flag.  
    U32 clearDepth;          //  Current clear depth value.  
    U32 zBufferPrecission;   //  Z buffer depth bit precission.  
    bool modifyDepth;           //  Flag to disable Hierarchical Z if the fragment shader stage modifies the fragment depth component.  
    CompareMode depthFunction;  //  Depth test comparation function.  
    bool multisampling;         //  Flag that stores if MSAA is enabled.  
    U32 msaaSamples;         //  Number of MSAA Z samples generated per fragment when MSAA is enabled.  

    S32 scissorMinX;         //  Scissor bounding mdu.  
    S32 scissorMaxX;         //  Scissor bounding mdu.  
    S32 scissorMinY;         //  Scissor bounding mdu.  
    S32 scissorMaxY;         //  Scissor bounding mdu.  

    //  Pixel Mapper
    PixelMapper pixelMapper;    //  Maps pixels to addresses and processing units.  

    //  Hierarchical Z state.  
    RasterizerState state;      //  Current Hierarchical Z mdu state.  
    RasterizerCommand *lastRSCommand;   //  Last rasterizer command received.  
    U32 fragmentCounter;     //  Number of received fragments.  
    bool lastFragment;          //  Stores if the last fragment has been received.  
    bool dataBus;               //  Stores if the HZ Level 0 data bus is busy.  
    U32 clearCycles;         //  Remaining HZ clear cycles.  

    //  Hierarchical Z structures.  
    U32 *hzLevel0;           //  Level 0 of the Hierachical Z buffer.  
    HZCacheLine *hzCache;       //  Hierarchical Z cache.  
    HZQueue *hzQueue;           //  Hierarchical Z Early Test queue.  
    U32 nextFree;            //  Next free HZ queue entry.  
    U32 nextRead;            //  Next stamp for which to read the block Z value.  
    U32 nextTest;            //  Next stamp to be tested.  
    U32 nextSend;            //  Next stamp to send to the Interpolator mdu.  
    U32 freeHZQE;            //  Number of free entries in the HZ queue.  
    U32 readHZQE;            //  Number of read entries in the HZ queue.  
    U32 testHZQE;            //  Number of test entries in the HZ queue.  
    U32 sendHZQE;            //  Number of send entries in the HZ queue.  
    U32 hzLineMask;          //  Precalculated mask for HZ cache lines.  
    U32 hzBlockMask;         //  Precalculated mask for the blocks inside a HZ cache line.  

    //  Statistics.  
    gpuStatistics::Statistic *inputs;         //  Input fragments.  
    gpuStatistics::Statistic *outputs;        //  Output fragments.  
    gpuStatistics::Statistic *outTriangle;    //  Fragments outside triangle.  
    gpuStatistics::Statistic *outViewport;    //  Fragments outside viewport.  
    gpuStatistics::Statistic *culled;         //  Culled fragments.  
    gpuStatistics::Statistic *cullOutside;    //  Fragments outside triangle or viewport culled.  
    gpuStatistics::Statistic *cullHZ;         //  Fragments culled by HZ test.  
    gpuStatistics::Statistic *misses;         //  Misses to HZ Cache.  
    gpuStatistics::Statistic *hits;           //  Hits to HZ Cache.  
    gpuStatistics::Statistic *reads;          //  Read operations to the HZ Buffer.  
    gpuStatistics::Statistic *writes;         //  Write operations to the HZ Buffer.  


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
     *  Calculates the minimum depth between two Z values
     *  (24 bit format).
     *
     *  @param a First input z value.
     *  @param b Second input z value.
     *
     *  @return The minimum of the two values.
     *
     */

    U32 minimumDepth(U32 a, U32 b);

    /**
     *
     *  Calculates the address of the block for a stamp or fragment inside
     *  the Hierarchical Z buffer.
     *
     *  @param x Horizontal position of the stamp/fragment.
     *  @param y Vertical position of the stamp/fragment.
     *  @param blocks Pointer to a MAX_STAMP_BLOCKS array where to store the addresses of
     *  the blocks required to evaluate the stamp.
     *
     *  @return The number of block address required to evaluate the stamp.
     *
     */

    U32 stampBlocks(S32 x, S32 y, U32 *blocks);

    /**
     *
     *  Search the Hierarchical Z Cache for a given Hierarchical Z block.
     *
     *  U32 block Block (address) to search in the cache.
     *  U32 cacheEntry Reference to a variable where to store which
     *  cache entry stores the block if found inside the cache.
     *
     *  @return If the block was found in the HZ Cache.
     *
     */

    bool searchHZCache(U32 block, U32 &cacheEntry);

    /**
     *
     *  Inserts a new block in the Hierarchical Z Cache.
     *
     *  @param block Block (address) to insert in the HZ Cache.
     *  @param cacheEntry Reference to a variable where to store which
     *  cache entry will store the new block in the HZ Cache.
     *
     *  @return If the block could be added to the HZ cache.
     *  Fails if there are no free HZ cache entries.
     *
     */

    bool insertHZCache(U32 block, U32 &cacheEntry);


    /**
     *
     *  Processes a rasterizer command.
     *
     *  @param command The rasterizer command to process.
     *  @param cycle  Current simulation cycle.
     *
     */

    void processCommand(RasterizerCommand *command);

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

    /**
     *
     *  Compares a fragment and its corresponding hierarchical z values.
     *
     *  @param fragZ Fragment depth.
     *  @param hzZ Corresponding depth stored in the Hierarchical Z buffer.
     *
     *  @return The result of the compare operation.
     *
     */

    bool hzCompare(U32 fragZ, U32 hzZ);

public:

    /**
     *
     *  Hierarchical Z class constructor.
     *
     *  Creates and initializes a Hierarchical Z mdu.
     *
     *  @param stampCyle Stamps received per cycle.
     *  @param overW Over scan tile width in scan tiles (may become a register!).
     *  @param overH Over scan tile height in scan tiles (may become a register!).
     *  @param scanW Scan tile width in fragments.
     *  @param scanH Scan tile height in fragments.
     *  @param genW Generation tile width in fragments.
     *  @param genH Generation tile height in fragments.
     *  @param disableHZ Disables the Hierarchical Z support.
     *  @param stampBlocks Stamps per Hierarchical Z block.
     *  @param hzBufferSize Size of the Hierarchical Z Buffer (number of stored blocks).
     *  @param hzCacheLines Number of lines in the Hierarchical Z Buffer cache.
     *  @param hzCacheLineSize Number of blocks in a line of the Hierarchical Z Buffer cache .
     *  @param hzQueueSize Size of the stamp queue for the early test.
     *  @param hzBufferLatency Access latency to the HZ Buffer level 0.
     *  @param hzUpdateLatency Update signal latency with Z Test.
     *  @param clearBlocksCycle Number of blocks cleared per cycle in the HZ Buffer level 0.
     *  @param nStampUnits Number of stamp units attached to HZ.
     *  @param suPrefixes Array of stamp unit prefixes.
     *  @param microTrisAsFragments Process micro triangles directly as fragments (MicroPolygon Rasterizer)
     *  @param microTriSzLimit 0: 1-pixel bound triangles only, 1: 1-pixel and 1-stamp bound triangles, 2: Any triangle bound to 2x2, 1x4 or 4x1 stamps (MicroPolygon Rasterizer).
     *  @param name Name of the mdu.
     *  @param parent Pointer to the mdu parent mdu.
     *
     *  @return A new initialized Hierarchical Z mdu.
     *
     */

    HierarchicalZ(U32 stampCycle, U32 overW, U32 overH, U32 scanW, U32 scanH,
        U32 genW, U32 genH, bool disableHZ, U32 stampBlocks, U32 hZBufferSize, U32 hzCacheLines,
        U32 hzCacheLineSize, U32 hzQueueSize, U32 hzBufferLatency, U32 hzUpdateLatency,
        U32 clearBlocksCycle, U32 nStampUnits, char **suPrefixes, bool microTrisAsFrag,
        U32 microTriSzLimit, char *name, cmoMduBase* parent);

    /**
     *
     *  Hierarchical Z class simulation function.
     *
     *  Simulates a cycle of the Hierarchical Z mdu.
     *
     *  @param cycle The cycle to simulate in the Hierarchical Z mdu.
     *
     */

    void clock(U64 cycle);

    /**
     *
     *  Returns a single line string with state and debug information about the
     *  Hierarchical Z mdu.
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

};

} // namespace cg1gpu

#endif
