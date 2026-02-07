/**************************************************************************
 *
 * cmoDisplayController mdu definition file.
 * This file defines the cmoDisplayController mdu.
 * This class implements the cmoDisplayController unit in a GPU unit.
 * Currently just consumes memory and outputs a file.
 *
 */

#include "GPUType.h"
#include "cmMduBase.h"
#include "GPUReg.h"
#include "cmRasterizerState.h"
#include "cmRasterizerCommand.h"
//#include "cmMemoryController.h"
#include "cmColorCacheV2.h"
#include "cmBlitter.h"
#include "PixelMapper.h"
#include "Compressor.h"


#ifndef _DAC_

#define _DAC_

namespace cg1gpu
{

/**
 *
 *  Defines a block request to memory.
 *
 */

struct BlockRequest
{
    U32 address;         //  Address in memory of the block to request.  
    U32 block;           //  Block identifier.  
    ROPBlockState state;    //  State (compression) of the block.  
    U32 size;            //  Size of the block in memory.  
    U32 requested;       //  Block bytes requested to memory.  
    U32 received;        //  Block bytes received from memory.  
};

/**
 *
 *  This class implements a cmoDisplayController mdu.
 *
 *  The cmoDisplayController converts the color buffer to signals for the display
 *  device (LCD, CRT, ...) in a real hardware.  The simulation mdu
 *  just consumes GPU memory bandwidth and stores the current
 *  colorbuffer in a file when the buffers are swapped.
 *
 *  Inherits from the cmoMduBase class that offers basic simulation
 *  support.
 *
 */

class cmoDisplayController : public cmoMduBase
{
private:

    //  cmoDisplayController signals.  
    Signal *dacCommand;         //  Command signal from the Command Processor.  
    Signal *dacState;           //  State signal to the Command Processor.  
    Signal *memoryRequest;      //  Request signal to the Memory Controller.  
    Signal *memoryData;         //  Data signal from the Memory Controller.  
    Signal **blockStateCW;      //  Array of signal from the Color Write units with the information about the color buffer block states (compressed, cleared, ...).  
    Signal **blockStateZST;     //  Array of signal from the Z Stencil Test units with the information about the color buffer block states (compressed, cleared, ...).  

    //  cmoDisplayController registers.  
    U32 hRes;                //  Display horizontal resolution.  
    U32 vRes;                //  Display vertical resolution.  
    S32 startX;              //  Viewport initial x coordinate.  
    S32 startY;              //  Viewport initial y coordinate.  
    bool d3d9PixelCoordinates;  //  Use D3D9 pixel coordinates convention -> top left is pixel (0, 0).  
    U32 width;               //  Viewport width.  
    U32 height;              //  Viewport height.  
    U32 frontBuffer;         //  Address in the GPU memory of the front (primary) buffer.  
    U32 backBuffer;          //  Address in the GPU memory of the back (secondary) buffer.  
    U32 zStencilBuffer;      //  Address in the GPU memory of the z stencil buffer.  
    TextureFormat colorBufferFormat;    //  Format of the color buffer.  
    Vec4FP32 clearColor;       //  The current clear color.  
    bool multisampling;         //  Flag that stores if MSAA is enabled.  
    U32 msaaSamples;         //  Number of MSAA Z samples generated per fragment when MSAA is enabled.  
    U32 bytesPixel;          //  Bytes per pixel.  
    U32 clearDepth;          //  Current clear depth value.  
    U32 depthPrecission;     //  Depth bit precission.  
    U08 clearStencil;         //  Current clear stencil value.  
    bool zStencilCompression;   //  Flag that stores if if z/stencil compression is enabled.  
    bool colorCompression;      //  Flag that stores if color compression is enabled.  
    bool rtEnable[MAX_RENDER_TARGETS];              //  Render target enable.  
    TextureFormat rtFormat[MAX_RENDER_TARGETS];     //  Render target format.  
    U32 rtAddress[MAX_RENDER_TARGETS];           //  Render target address.  

    U08 clearColorData[MAX_BYTES_PER_COLOR];      // Color clear value converted to the color buffer format.  
                            
    //  cmoDisplayController parameters.  
    U32 genH;                //  Generation tile height in stamps.  
    U32 genW;                //  Generation tile width in stamps.  
    U32 scanH;               //  Scan tile height in generation tiles.  
    U32 scanW;               //  Scan tile width in generation tiles.  
    U32 overH;               //  Over scan tile heigh in scan tiles.  
    U32 overW;               //  Over scan tile width in scan tiles.  
    U32 blockStateLatency;   //  Latency of the color block state signal.  
    U32 blocksCycle;         //  Number of block states received per cycle.  
    U32 blockSize;           //  Size of color block in bytes (uncompressed).  Derived from generation tile and pixel data size.  
    U32 blockShift;          //  Bits to shift to retrieve the block number from a color buffer address.  
    U32 blockQueueSize;      //  Number of entries in the block request queue.  
    U32 decompressLatency;   //  Number of cycles it takes a block to be decompressed.  
    U32 numStampUnits;       //  Number of Color Write units attached to the cmoDisplayController.  
    U32 startFrame;          //  Number of the first frame (non simulation related, first frame number for framebuffer dumping).  
    U64 refreshRate;         //  Number of cycles between the dumping/refresh of the color buffer.  
    bool synchedRefresh;        //  Flag storing if the swap of the color buffer must be synchronized with the refresh.  
    bool refreshFrame;          //  Flag that stores if the color buffer must be refreshed/dumped to a file.  
    bool saveBlitSourceData;    //  Save the source data of blit operations to a PPM file.  

    //  cmoDisplayController state.  
    RasterizerState state;              //  Current cmoDisplayController state.  
    RasterizerCommand *lastRSCommand;   //  Stores the last Rasterizer Command received (for signal tracing).  
    U08 *colorBuffer;                 //  Buffer where to store the full color buffer.  
    ROPBlockState *colorBufferState;    //  Current state of the color buffer blocks.  
    U32 colorStateBufferBlocks;      //  Number of blocks in the current color buffer state.  
    U32 frameCounter;                //  Counts the number of rendered frames.  
    U32 batchCounter;                //  Counts the number of draw calls in the current frame.  
    U32 blitCounter;                 //  Counts the number of blit ops in the current frame.  
    U32 requested;                   //  Requested color buffer bytes (blocks).  
    U32 colorBufferSize;             //  Size in bytes of the color buffer.  
    U32 stateUpdateCycles;           //  Number of cycles remaining for updating the color buffer state memory.  
    U32 decompressCycles;            //  Number of cycles remaining for block decompression.  
    U08 *zstBuffer;                   //  Buffer where to store the full z stencil buffer.  
    ROPBlockState *zStencilBufferState; //  Current state of the z stencil buffer blocks.  
    U32 zStencilStateBufferBlocks;   //  Number of blocks in the current z stencil buffer state.  
    U32 zStencilBufferSize;          //  Size in bytes of the z stencil buffer.  
    U32 zStencilBlockSize;           //  Size in bytes of a z stencill buffer block.  
    U32 zStencilBytesPixel;          //  Bytes per z stencil pixel.  
    U32 clearZStencilData;           //  Clear value for the z stencil buffer.  

    //  cmoDisplayController block queue.  
    BlockRequest *blockQueue;   //  Queue for the blocks being processed by the cmoDisplayController.  
    U08 **blockBuffer;        //  Buffer where to store block data.  
    U32 ticket2queue[MAX_MEMORY_TICKETS];    //  Associates memory tickets with block queue entries.  
    U32 nextFree;            //  Next free block queue entry.  
    U32 nextRequest;         //  Next block in the queue to request to memory.  
    U32 nextDecompress;      //  Next block in the queue to decompress.  
    U32 numFree;             //  Number of free queue entries.  
    U32 numToRequest;        //  Number of blocks to request in the queue.  
    U32 numToDecompress;     //  Number of blocks to decompress in the queue.  

    //  cmoDisplayController memory state.  
    MemState memState;                  //  Current memory state.  
    tools::Queue<U32> ticketList;    //  List with the memory tickets available to generate requests to memory.  
    U32 freeTickets;                 //  Number of free memory tickets.  
    U32 busCycles;                   //  Counts the number of cycles remaining for the end of the current memory transmission.  
    U32 lastSize;                    //  Size of the last memory transaction received.  
    U32 lastTicket;                  //  Ticket of the last memory transaction received.  

    //  Pixel mapper.
    PixelMapper colorPixelMapper;       //  Maps pixels to addresses and processing units.  
    PixelMapper zstPixelMapper;         //  Maps pixels to addresses and processing units.  
    
    //  cmoDisplayController Blitter component 
    Blitter* blt;                     // Blitter unit in cmoDisplayController cmoMduBase 
    bool bufferStateUpdatedAtBlitter; //  Flag telling the buffer state block structure has been sent to the blitter.  
    
    //  cmoDisplayController statistics.  
    gpuStatistics::Statistic &stateBlocks;                  //  Total number of state blocks read from CW units.  
    gpuStatistics::Statistic &stateBlocksClear;             //  Number of clear color blocks.  
    gpuStatistics::Statistic &stateBlocksUncompressed;      //  Number of uncompressed color blocks.  
    gpuStatistics::Statistic &stateBlocksCompressedBest;     //  Number of best compressed color blocks.  
    gpuStatistics::Statistic &stateBlocksCompressedNormal;   //  Number of normal compressed color blocks.  
    gpuStatistics::Statistic &stateBlocksCompressedWorst;     //  Number of worst compressed color blocks.  
    
    //  Private functions.  

    /**
     *
     *  Processes a rasterizer command.
     *
     *  @param cycle Current simulation cycle.
     *  @param command The rasterizer command to process.
     *
     */

    void processCommand(U64 cycle, RasterizerCommand *command);

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
     *  @param memTrans Pointer to the memory transaction to process.
     *
     */

    void processMemoryTransaction(MemoryTransaction *memTrans);

    /**
     *
     *  Outputs the current color buffer as a PPM file.
     *
     */

    void writeColorBuffer();

    /**
     *
     *  Outputs the current depth buffer as a PPM file.
     *
     */

    void writeDepthBuffer();

    /**
     *
     *  Outputs the current stencil buffer as a PPM file.
     *
     */

    void writeStencilBuffer();

    /**
     *
     *  Translates an address to the current color buffer into
     *  a block index in the color buffer state memory.
     *
     *  @param address The address to translate.
     *
     *  @return The block index for that address.
     *
     */

    U32 address2block(U32 address);


    /**
     *
     *  Calculates the block assigned stamp unit.
     *
     *  @param block Block number/address.
     *  @param pixelMapper The pixel mapper used to obtain the unit corresponding to the block.
     *
     *  @return The stamp unit to which the block of fragments was assigned.
     *
     */

    U32 blockUnit(PixelMapper &pixelMapper, U32 block);

    /**
     *
     *  Initializes the refresh/dumping of the color buffer.
     *
     */

    void startRefresh();

    /**
     *
     *  Update memory transmission.
     *
     *  @param cycle The current simulation cycle. 
     *
     */
     
    void updateMemory(U64 cycle);
    
    /**
     *
     *  Update the block decompressor state.
     *
     *  @param cycle The current simulation cycle.
     *  @param compressorEmulator A pointer to the compress behaviorModel used to decompress blocks.
     *  @param dumpBuffer A pointer to the buffer where to store the block decompressed data.
     *  @param clearValue A pointer to a byte array with the clear value data.
     *  @param bytesPerPixel Bytes per pixel for the current format of the buffer to decompress.
     *  @param baseBufferAddress Address in GPU memory of the buffer to decompress.
     *
     */
     
    void updateDecompressor(U64 cycle, bmoCompressor *compressorEmulator, U08 *dumpBuffer, 
                             U08 *clearValue, U32 bytesPerPixel, U32 baseBufferAddress);
    
    
    /**
     *
     *  Update the block request stage.
     *
     *  @param cycle Current simulation cycle.
     *
     */
     
    void updateBlockRequest(U64 cycle);
 
    /**
     *
     *  Update request queue stage.
     *
     *  @param cycle Current simulation cycle.
     *  @param compressorEmulator A pointer to the compress behaviorModel used to decompress blocks.
     *  @param baseBufferAddress Address in GPU memory of the buffer to request.
     *  @param dumpBufferSize Size in bytes of the buffer to request.
     *  @param bufferBlockState Pointer to an array with per block state information for the buffer to request.
     *  @param bufferBlockSize Size in bytes of a block in the buffer to request.
     *
     */

    void updateRequestQueue(U64 cycle, bmoCompressor *compressorEmulator, U32 baseBufferAddress,
                            U32 dumpBufferSize, ROPBlockState *bufferBlockState, U32 bufferBlockSize);

    /**
     *
     *  Reset the state of the cmoDisplayController unit.
     *
     */     
     
    void reset();
    
    class BlitterTransactionInterfaceImpl;
    
    friend class BlitterTransactionInterfaceImpl; // Allow internal access to parent class (cmoDisplayController) 
    
    class BlitterTransactionInterfaceImpl: public BlitterTransactionInterface
    {
        
    private:
        
        cmoDisplayController& dac;

    public:
        
        BlitterTransactionInterfaceImpl(cmoDisplayController& dac);

        /**
         *  Callback methods available for the Blitter unit to send memory transactions
         *
         */
        bool sendWriteTransaction(U64 cycle, U32 size, U32 address, U08* data, U32 id);
        bool sendMaskedWriteTransaction(U64 cycle, U32 size, U32 address, U08* data, U32 *mask, U32 id);
        bool sendReadTransaction(U64 cycle, U32 size, U32 address, U08* data, U32 ticket);
    
        gpuStatistics::StatisticsManager& getSM();
        
        std::string getBoxName();
        
    } bltTransImpl; // The BlitterTransactionInterfaceImpl implementor object
    
    
public:

    /**
     *
     *  cmoDisplayController mdu constructor.
     *
     *  Creates and initializes a cmoDisplayController mdu.
     *
     *  @param overW Over scan tile width in scan tiles.
     *  @param overH Over scan tile height in scan tiles.
     *  @param scanW Scan tile width in pixels.
     *  @param scanH Scan tile height in pixels.
     *  @param genW Generation tile width in pixels.
     *  @param genH Generation tile height in pixels.
     *  @param mortonBlockDim Dimension of a texture morton block.
     *  @param mortonSBlockDim Dimension of a texture morton superblock.
     *  @param blockSz Size of an uncompressed block in bytes.
     *  @param blockStateLatency Color block state signal latency.
     *  @param blocksCycle Blocks updated per cycle in the state memory.
     *  @param blockQueueSize Number of entries in the block request queue.
     *  @param decompLatency Number of cycles it takes to decompress a block.
     *  @param nStampUnits Number of stamp units attached to HZ.
     *  @param suPrefixes Array of stamp unit prefixes.
     *  @param startFrame Number of the first frame to be dumped.
     *  @param refresh Refresh rate (cycles between generation/dumping of the color buffer).
     *  @param synched Framebuffer dumping/refresh syncronized with refresh rate.
     *  @param refreshFrame Enables or disables the dumping/refresh of the frame buffer.
     *  @param name The mdu name.
     *  @param mdu The parent mdu.
     *
     *  @return An initialized cmoDisplayController mdu.
     *
     */

    cmoDisplayController(U32 overW, U32 overH, U32 scanW, U32 scanH, U32 genW, U32 genH,
        U32 mortonBlockDim, U32 mortonSBlockDim,
        U32 blockSz, U32 blockStateLatency, U32 blocksCycle, U32 blockQueueSize,
        U32 decompLatency, U32 nStampUnits, char **suPrefixes, 
        U32 startFrame, U64 refresh, bool synched, bool refreshFrame,
        bool saveBlitSourceData, char *name, cmoMduBase *parent);

    /**
     *
     *  cmoDisplayController simulation function.
     *
     *  Simulates a cycle of the cmoDisplayController mdu.
     *
     *  @param cycle The current simulation cycle.
     *
     */

    void clock(U64 cycle);
};

} // namespace cg1gpu

#endif
