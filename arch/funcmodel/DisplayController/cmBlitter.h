/**************************************************************************
 *
 */

/**
  *
  *  @file cmBlitter.h
  *
  *  This file defines the Blitter class and auxiliary structures. The Blitter class is used to 
  *  perform bit blit operations through a blitter unit present in most of the graphics boards.
  *
  */
 
#ifndef _BLITTER_

#define _BLITTER_

#include "GPUType.h"
#include "cmMemoryTransaction.h"
#include "GPUReg.h"
#include "cmColorCacheV2.h"
#include "cmRasterizerState.h"
#include "cmRasterizerCommand.h"
#include "cmtoolsQueue.h"
#include "cmWakeUpQueue.h"
#include "cmStatisticsManager.h"
#include "PixelMapper.h"
#include <list>
#include <vector>
#include <utility>
#include <set>

namespace cg1gpu
{

/** 
  *  Maximum number of comparisons the wake up queue can perform in a cycle. This "comparison 
  *  capacity" has to be shared between the queue entries and the entry event list size, 
  *  for instance, if MAX_WAKEUPQUEUE_COMPARISONS_PER_CYCLE = 256 then, a wakeup queue allowing
  *  event lists of 16 elements can only have 16 entries, but allowing event lists of 4 elements
  *  the queue can have 64 entries.
  */
const U32 MAX_WAKEUPQUEUE_COMPARISONS_PER_CYCLE = 256;

/**
  *  The BlitterTransactionInterface is the interface used by the blitter in order to access the memory controller and 
  *  perform memory requests. The Blitter constructor requires an implementation of this interface.
  * 
  *  @note Only a simulator cmoMduBase directly connected to the memory controller can implement this interface and thus
  *        can host the Blitter component of the GPU.
  *
  *  @note The simulator cmoMduBase can notify to the Blitter that a read transaction is complete using the 
  *        receivedReadTransaction() public method in of the Blitter class.
  *
 */
 
class BlitterTransactionInterface
{
public:

    /**
      *  Sends a memory read transaction.
      *  
      *  @return return False if the corresponding transaction was not possible this cycle, True otherwise.
      */
    virtual bool sendReadTransaction(
                            U64 cycle, 
                            U32 size, 
                            U32 address, 
                            U08* data, 
                            U32 ticket) = 0;

    /**
      *  Sends a memory write transaction.
      *  
      *  @return return False if the corresponding transaction was not possible this cycle, True otherwise.
      */
    virtual bool sendWriteTransaction(
                            U64 cycle, 
                            U32 size, 
                            U32 address, 
                            U08* data, 
                            U32 ticket) = 0;
                            
    /**
      *  Sends a masked memory write transaction.
      *  
      *  @return return False if the corresponding transaction was not possible this cycle, True otherwise.
      */
    virtual bool sendMaskedWriteTransaction(
                            U64 cycle, 
                            U32 size, 
                            U32 address, 
                            U08* data, 
                            U32 *mask, 
                            U32 ticket) = 0;

    /**
      *  Gets a reference to StatisticsManager.
      *  
      *  This is needed because only cmoMduBase classes can have access to the statistics manager.
      */
    virtual gpuStatistics::StatisticsManager& getSM() = 0;
    
    /**
      *  Gets the cmoMduBase name of this interface implementator.
      *  
      *  @note The Blitter class will use this name for panic and warning messages.
      */
    virtual std::string getBoxName() = 0;
    

}; // class BlitterTransactionInterface


/**
  *  Some auxiliar structures used by the blitter.
  *  
  *  The following are structures to store the processing information of memory blocks. 
  */


typedef std::vector<S32> swizzleMask;  /**< Definition of the swizzle mapping. Each position tells the correspondence between pixel and texel bytes.
                                            *  Negative values will be used to specify that the corresponding pixel bytes doesn�t map to any texel byte. */               
/**
  *  Stores all the data and state related to the requested color buffer memory block of a single 
  *  texture block.
  *  It holds the compressed and uncompressed contents of the requested block to memory.
  */
class ColorBufferBlock
{

public:

    U32              address;            //  The color buffer block start address.  
    U32              block;              //  The color buffer address block (address >> log2(colorBufferblockSize)).  
    ROPBlockState       state;              //  State (compression) of the block.   
    swizzleMask         swzMask;            //  The swizzling mapping to the texture block texels.  
    U08*              compressedContents; //  The compressed contents from memory.  
    U08*              contents;           //  The contents once decompressed.  
    U32              requested;          //  The texture block bytes already requested.  
    std::vector<U32> tickets;            //  The transaction ticket ids. Used as wait events for requested data blocks. 
    
    /**
      *  Color Buffer Block constructor
      *  
      *  @note Initializes pointers to NULL
      */
    ColorBufferBlock();
    
    /**
      *  Color Buffer Block destructor
      *  
      *  @note Deletes non-null pointers
      */
     ~ColorBufferBlock();
     
     /**
       *  Color Buffer Block print method
       */
     void print(std::ostream& os) const;
    
     friend std::ostream& operator<<(std::ostream& os, const ColorBufferBlock& b)
     {
         b.print(os);
         return os;
     }
     
};

        
/**
  *  Stores all the data and state of a texture block inside the texture region to update.
  *  The texture block is the main object that flows through the blitter pipeline.
  *  
  *  It holds a vector of up to as many color buffer blocks as necessary to
  *  fill all the destination texture bytes in the texture block that lay inside 
  *  the destination texture region.
  * 
  */  
class TextureBlock
{
    
public:

    U32 xBlock;                                         //  X "tile" position of the texture region block.  
    U32 yBlock;                                         //  Y "tile" position of the texture region block.  
    U32 address;                                        //  The texture block start address.  
    U32 block;                                          //  The texture address block (address >> log2(TextureblockSize)).   
    U08* contents;                                       //  Block contents to write. 
    bool*  writeMask;                                      //  Mask telling which bytes correspond to valid data (block bytes corresponding to texels to update in memory). 
    U32 written;                                        //  Bytes already written to memory.  
    std::vector<ColorBufferBlock*> colorBufferBlocks;      //  The related color buffer blocks.  
    
    /**
      *  Texture Block constructor
      *  
      *  @note Initializes pointers to NULL
      */
    TextureBlock();

    /**
      *  Texture Block destructor
      *  
      *  @note Deletes non-null pointers.
      */
    ~TextureBlock();
    
    /**
      *  Texture Block print method
      */
    void print(std::ostream& os) const;
    
    friend std::ostream& operator<<(std::ostream& os, const TextureBlock& b)
    {
        b.print(os);
        return os;
    }
    
};

/**
  *  The swizzling buffer emulates the functionality of a hardware swizzler working on a entire buffer of bytes.
  *
  *  Two arrays of bytes actually exists, the input buffer and the output buffer, where the result is stored.
  *
  *  The input buffer can be filled using the writeInputData() operation, writing as many bytes as the write port
  *  width. Once performed the operation with the swizzle() op, the results can be obtained, reading read port width bytes
  *  using the readOutputData() operation.
  *
  *  To specify the swizzling mask (correspondence of input bytes to output bytes), other different than the identity 
  *  permutation (used by default if no swizzling mask is speficied after reset()) the setSwzMask() can be used.
  */
class SwizzlingBuffer
{

private:
    
    //  Swizzling Buffer parameters.  
    U32 size;                                     //  Permutation buffer size. This Hw piece can perform a swizzling over this amount of bytes. 
    U32 writePortWidth;                           //  Width of the write port. log2(size/width) bits to identify the write position in the buffer.  
    U32 readPortWidth;                            //  Width of the read port. log2(size/width) bits to identify the write position in the buffer.  

    //  Swizzling Buffer registers.  
    U08* inBuffer;                                 //  Input Buffer where to store the source of the pixel permutation. 
    U08* outBuffer;                                //  Output Buffer where to store the result of the pixel permutation. 
    std::vector<S32> swzMap;                      //  Table mapping the swizzle.  
    
    //  Swizzling Buffer structures.  
    U32 max_read_position;                        //  Last position where to read in the buffer. 
    U32 max_write_position;                       //  Last position where to write in the buffer. 
    
    U32 highest_written_position;                //  The highest written position.  
    U32 highest_read_position;                   //  The highest read position.  

public:
    
    /**
      *  Swizzling buffer constructor
      *
      *  @param size            The number of bytes of the swizzling buffer.
      *  @param readPortWidth   The result read port width.
      *  @param writePortWidth  The write port width.
      *  @note                  size must be multiple of readPortWidth and writePortWidth, if not a panic is thrown.
      */
    SwizzlingBuffer(U32 size = 1024, U32 readPortWidth = 256, U32 writePortWidth = 256);
  
    /**
      *  Swizzling buffer destructor
      */
    ~SwizzlingBuffer();
    
    /**
      *  Resets the swizzling buffer. 
      *
      *  @note Basically, puts the swizzling mask to the identity permutation.
      *
      */
    void reset();

    /**
      *  Fills the corresponding portion of the input buffer.
      */      
    void writeInputData(U08* inputData, U32 position);
    
    /**
      *  Reads the corresponding portion of the ouput/result buffer.
      */
    void readOutputData(U08* outputData, U32 position);
    
    /**
      *  Performs the data swizzling.
      */
    void swizzle();
    
    /**
      *  Sets the swizzling mask for the corresponding portion.
      */
    void setSwzMask(std::vector<S32> swzMap, U32 position);
};

/**
  *  The Blitter class implements the functionality of the blitter unit.
  *
  */    
class Blitter
{
private:
    
    // Blitter state registers    
    U32                     hRes;                            //  Display horizontal resolution.  
    U32                     vRes;                            //  Display vertical resolution.  
    S32                     startX;                          //  Viewport initial x coordinate.  
    S32                     startY;                          //  Viewport initial y coordinate.  
    U32                     width;                           //  Viewport width.  
    U32                     height;                          //  Viewport height.  
    Vec4FP32                  clearColor;                      //  The current clear color.  
    TextureFormat              colorBufferFormat;               //  The current color buffer format.  
    U32                     backBufferAddress;               //  Address of the back buffer (source blit) in memory.  
    U32                     blitIniX;                        //  Bit blit initial (bottom-left) color buffer X coordinate.  
    U32                     blitIniY;                        //  Bit blit initial (bottom-left) color buffer Y coordinate.  
    U32                     blitHeight;                      //  Bit blit color buffer region height.  
    U32                     blitWidth;                       //  Bit blit color buffer region width.  
    U32                     blitXOffset;                     //  X coordinate offset in the destination texture. 
    U32                     blitYOffset;                     //  Y coordinate offset in the destination texture. 
    U32                     blitDestinationAddress;          //  GPU memory address for blit operation destination. 
    U32                     blitTextureWidth2;               //  Ceiling log 2 of the destination texture width.  
    TextureFormat              blitDestinationTextureFormat;    //  Texture format for blit destination.  
    TextureBlocking            blitDestinationBlocking;         //  Texture tiling/blocking format used for the destination texture.  
    std::vector<ROPBlockState> colorBufferState;                //  Current state of the color buffer blocks.  
    U32 bytesPixel;                                          //  Bytes per pixel.  
    bool multisampling;                                         //  Flag that enables or disables multisampling antialiasing.  
    U32 msaaSamples;                                         //  Number of multisampling samples per pixel.  
    
    //  Blitter parameters.  
    U32 stampH;                              //  Stamp tile height.  
    U32 stampW;                              //  Stamp tile width.  
    U32 genH;                                //  Generation tile height in stamps.  
    U32 genW;                                //  Generation tile width in stamps.  
    U32 scanH;                               //  Scan tile height in generation tiles.  
    U32 scanW;                               //  Scan tile width in generation tiles.  
    U32 overH;                               //  Over scan tile heigh in scan tiles.  
    U32 overW;                               //  Over scan tile width in scan tiles.  
    U32 mortonBlockDim;                      //  The morton block size.  
    U32 mortonSBlockDim;                     //  The morton superblock block size.  
    U32 blockStateLatency;                   //  Latency of the color block state signal.  
    U32 blocksCycle;                         //  Number of block states received per cycle.  
    U32 colorBufferBlockSize;                //  Size of color block in bytes (uncompressed).  Derived from generation tile and pixel data size.  
    U32 maxTransactionsPerTextureBlock;      //  Maximum transactions for each texture block.  
    U32 maxColorBufferBlocksPerTextureBlock; //  Maximum color buffer blocks per each texture block.  
    U32 textureBlockSize;                    //  Size of the texture block in bytes. It should correspond to the size in bytes of a 8x8 texels morton block.  
    U32 blockQueueSize;                      //  Number of entries in the block request queue.  
    U32 newBlockLatency;                     //  Number of cycles it takes build a texture block and all the related information.  
    U32 decompressionLatency;                //  Number of cycles it takes a block to be decompressed.  
    U32 swizzlingLatency;                    //  Number of cycles it takes the swizzling buffer to swizzle bytes.  
    bool saveBlitSource;                        //  Flag that stores if the blit source data must be saved as a PPM file.  

    //  Pixel Mapper.
    PixelMapper pixelMapper;                    //  Maps pixels to addresses and processing units. 
    
    //  Blitter queues  
    tools::Queue<TextureBlock*>       requestPendingQueue;           //  Queue with texture blocks whose color buffer corresponding blocks requests are pending.  
    WakeUpQueue<TextureBlock, U32> receiveDataPendingWakeUpQueue; //  Queue for blocks waiting to receive the color buffer blocks data.  
    tools::Queue<TextureBlock*>       decompressionPendingQueue;     //  Queue for blocks waiting for color buffer blocks decompression.  
    tools::Queue<TextureBlock*>       swizzlingPendingQueue;         //  Queue with texture blocks waiting for byte swizzling operation.  
    tools::Queue<TextureBlock*>       writePendingQueue;             //  Queue with texture blocks ready to be written to memory.  
    
    //  Blitter variables & structures  
    U32 colorBufferSize;                      //  The current color buffer size.  
    bool colorBufferStateReady;                  //  Flag telling if color buffer state information already available.  
    U32 startXBlock;                          //  First texture block that includes the texture subarea (X coord).   
    U32 startYBlock;                          //  First texture block that includes the texture subarea (Y coord).  
    U32 lastXBlock;                           //  Last texture block that includes the texture subarea (X coord).  
    U32 lastYBlock;                           //  Last texture block that includes the texture subarea (Y coord).  
    U32 currentXBlock;                        //  Current texture block being processed (X coord).  
    U32 currentYBlock;                        //  Current texture block being processed (Y coord).  
    U32 totalBlocksToWrite;                   //  Total number of blocks to write to memory. 
    U32 blocksWritten;                        //  Blocks already sent to memory.  
    SwizzlingBuffer  swzBuffer;                  //  The swizzling hardware to permute pixel bytes in color buffer blocks to texel bytes in texture blocks.  
    U32 swizzlingCycles;                      //  The number of remaining cycles to finish the current swizzling operation.  
    U32 newBlockCycles;                       //  The number of remaining cycles to finish the generation of a new texture block.  
    U32 decompressionCycles;                  //  The number of remaining cycles to finish a color buffer block decompression.  
    
    U08 *colorBuffer;     //  Pointer to an array where to store the data read from the color buffer.  
    
    U32 nextTicket;                           //  Ticket counter to identify read transactions.  
    
    bool memWriteRequestThisCycle;               //  Flag telling if a memory write transaction was sent to the request signal this cycle. 

    //  DisplayController Interface 
    BlitterTransactionInterface& transactionInterface;  //  Interface to communicate read and write memory transactions through DisplayController signals. 

    //  Blitter statistics.  
    
    gpuStatistics::Statistic &readBlocks;                       //  Number of color buffer blocks read from memory.  
    gpuStatistics::Statistic &writtenBlocks;                    //  Number of texture blocks written to memory.  
                                                                
    gpuStatistics::Statistic &clearColorBlocks;                 //  Number of clear color blocks.  
    gpuStatistics::Statistic &uncompressedBlocks;               //  Number of uncompressed color blocks.  
    gpuStatistics::Statistic &compressedBestBlocks;             //  Number of best compressed color blocks.  
    gpuStatistics::Statistic &compressedNormalBlocks;           //  Number of normal compressed color blocks.  
    gpuStatistics::Statistic &compressedWorstBlocks;             //  Number of worst compressed color blocks.  

    gpuStatistics::Statistic &requestPendingQueueFull;           //  Cycles the request pending queue is full.  
    gpuStatistics::Statistic &receiveDataPendingWakeUpQueueFull; //  Cycles the receive pending wakeup queue is full.  
    gpuStatistics::Statistic &decompressionPendingQueueFull;     //  Cycles the decompression pending queue is full.  
    gpuStatistics::Statistic &swizzlingPendingQueueFull;         //  Cycles the swizzling pending queue is full.  
    gpuStatistics::Statistic &writePendingQueueFull;             //  Cycles the write pending queue is full.  
    
public:

    /**
      *  Blitter constructor method.
      *
      *  @note All the Blitter parameters required in the constructor with any default value.
      */
    Blitter(BlitterTransactionInterface& transactionInterface, 
            U32 stampH, U32 stampW, U32 genH, U32 genW, U32 scanH, U32 scanW, U32 overH, U32 overW,
            U32 mortonBlockDim, U32 mortonSBlockDim, U32 textureBlockSize,
            U32 blockStateLatency, U32 blocksCycle, U32 colorBufferBlockSize, U32 blockQueueSize, U32 decompressLatency,
            bool saveBlitSource);

    /**
      *  Reset current blitter state registers values.
      */
    void reset();
    
    /**
      *  Register setting functions. 
      */
    void setDisplayXRes(U32 hRes);
    void setDisplayYRes(U32 vRes);
    void setClearColor(Vec4FP32 clearColor);
    void setViewportStartX(U32 startX);
    void setViewportStartY(U32 startY);
    void setViewportWidth(U32 width);
    void setViewportHeight(U32 height);
    void setColorBufferFormat(TextureFormat format);
    void setBackBufferAddress(U32 address);
    void setBlitIniX(U32 iniX);
    void setBlitIniY(U32 iniY);
    void setBlitWidth(U32 width);
    void setBlitHeight(U32 height);
    void setBlitXOffset(U32 xoffset);
    void setBlitYOffset(U32 yoffset);
    void setDestAddress(U32 address);
    void setDestTextWidth2(U32 width);
    void setDestTextFormat(TextureFormat format);
    void setDestTextBlock(TextureBlocking blocking);    
    void setColorBufferState(ROPBlockState* _colorBufferState, U32 colorStateBufferBlocks);
    void setMultisampling(bool enabled);
    void setMSAASamples(U32 samples);
    
    /**
      *
      *  Initiates the bit blit operation.
      *
      *  @note  Initializes all the variables & structures properly to start the bit blit programmed operation.
      */
    void startBitBlit();
    
    /**
      *  Notifies that a memory read transaction has been completed.
      *
      *  @param ticket    Received transaction ticket 
      *  @param size      Received transaction size
      *
      */
    void receivedReadTransaction(U32 ticket, U32 size);
    
       
    /**
      *  Notifies that the current bit blit operation has finished.
      *
      *  @note It remains returning true until the startBitBlit() method is
      *        called again.
      */
    bool currentFinished();
    
    /**
     *
     *  Dumps the contents of the portion of the framebuffer accesed by the blit operation to a file.
     *    
     *  @param frameCounter Current frame counter.
     *  @param blitCounter Current blit operation counter.
     *
     */

    void dumpBlitSource(U32 frameCounter, U32 blitCounter);
    
    /**
      *
      *  Simulates a cycle of the blitter unit.
      *
      *  @param cycle Current simulation cycle.
      *
      */
    void clock(U64 cycle);
    

private:

    /***********************************************************
      *  Auxiliar methods used internaly by the Blitter class  *
      ***********************************************************/
         
    /**
      *  Static method used by the blitter constructor to compute the maximum number of necessary color buffer blocks 
      *  to fill a single texture block. If the texture block and the color buffer block don�t overlap in a perfect way,
      *  although blocks of the same size, one texture block would require more than one single color buffer block.
      *
      *  @param TextureBlockSize2      The log2 of the texture block size.
      *  @param ColorBufferBlockSize2  The log2 of the color buffer block size.
      */
    static U32 computeColorBufferBlocksPerTextureBlock(U32 TextureBlockSize2, U32 ColorBufferBlockSize2);
             
    /**
      *  Translates texel coordinates to the morton address offset starting from the texture base address.
      *
      *  @param i         The horizontal coordinate of the texel.
      *  @param j         The vertical coordinate of the texel.
      *  @param blockDim  The morton block dimension.
      *  @param sBlockDim The morton superblock dimension.
      *  @param width     The ceiling log2 width of the destination texture.
      *
      */
    U32 texel2MortonAddress(U32 i, U32 j, U32 blockDim, U32 sBlockDim, U32 width);
    
    /**
      *  Returns the number of bytes of each texel for a texture using this format.
      *
      *  @param format   The texture format.
      *  @return         How many bytes for each texel.
      */
    U32 getBytesPerTexel(TextureFormat format);
    
    /**
      *
      *  Decompresses a block according to the compression state and using the specified clearColor.
      *
      *  @note                        The output buffer is assumed to have enough space to decompress an entire block.
      *
      *  @param outbuffer             Location where to store the decompressed block.
      *  @param inbuffer              The compressed block buffer.
      *  @param colorBufferBlockSize  The size in bytes of the block
      *  @param state                 The compression state ( CLEAR, UNCOMPRESSED, COMPRESSION_NORMAL and COMPRESSION_BEST)
      *  @param clearColor            The clear color to fill CLEAR blocks.
      *
      */
    void decompressBlock(U08* outBuffer, U08* inBuffer, U32 colorBufferBlockSize, ROPBlockState state, Vec4FP32 clearColor);

    /**
      *  Translates an address to the start of a block into a block number.
      *
      *  @param address     The address of the block.
      *  @param blockSize   The block size.          
      *  @return            The corresponding block number
      */           
    U32 address2block(U32 address, U32 blockSize);
    
    /**
      *  Generates the proper write mask for the memory transaction given the input mask
      *  as a boolean array (one value per byte).
      *
      *  @note inputMask must be indexable in the range [0..size], and outputMask must have
      *        (size >> 2) elements of allocated space.
      *
      *  @param outputMask The output write mask in the memory transaction required format.
      *  @param inputMask  The input write mask as an array of booleans.
      *  @param size       The number of inputMask elements.
      */
    void translateBlockWriteMask(U32* outputMask, bool* inputMask, U32 size);
      

    /***********************************************************
      *  Private Methods for texture block processing.         *
      ***********************************************************/  

    /**
      *  Sends the next write memory transaction for the specified texture block.
      *
      *  @note If a write transaction is not allowed this cycle, then the method returns false.
      *
      *  @param cycle            The simulation cycle.
      *  @param texureBlock      The texture block to write bytes from.
      *
      *  @return False if write transaction fails, otherwise True.
      *
      */
    bool sendNextWriteTransaction(U64 cycle, TextureBlock* textureBlock);

    /**
      *  Examines ready texture blocks and sends the next transaction of the head block.
      *
      *  @param cycle    The simulation cycle.
      *  @return         False if ready texture blocks is empty or write transaction couldn�t be
      *                  sent.
      */
    bool sendNextReadyTextureBlock(U64 cycle, tools::Queue<TextureBlock*>& writePendingQueue);
        
    /**
      *  Generates a texture block with all the necessary information.
      *
      *  @note               The texture block pointer memory must be previously allocated.
      *  @param cycle        The simulation cycle.
      *  @param xBlock       The X coordinate of the texture block tile.  
      *  @param yBlock       The Y coordinate of the texture block tile.
      *  @param textureBlock Pointer to the TextureBlock structure to fill.
      *
      */
    void processNewTextureBlock(U64 cycle, U32 xBlock, U32 yBlock, TextureBlock* textureBlock);
   
    /**
      *  Computes the total number of transactions needed to request all the color buffer blocks data
      *  of the texture block.
      *
      *  @param textureBlock    The texture block.
      *  @return                The total number of transactions to request.
      */
    U32 computeTotalTransactionsToRequest(TextureBlock* textureBlock);
    
    /**
      *  Sends as many memory read requests as allowed in one cycle for the color buffer blocks in the texture block.
      *
      *  @param cycle             The simulation cycle.
      *  @param textureBlock      The texture block.
      *  @param generatedTickets  The output list with all the generated tickets for the sent transactions.
      *
      */
    bool requestColorBufferBlocks(U64 cycle, TextureBlock* textureBlock, set<U32>& generatedTickets);

    /**
      *  This set of functions send as many memory read requests as allowed in one cycle for the color buffer block 
      *  according to the compression format of the block. The result is the list of generated tickets for the
      *  sent transactions.
      */
    bool requestColorBlock(U64 cycle, ColorBufferBlock *bInfo, U32 blockRequestSize, set<U32>& generatedTickets);

    bool requestUncompressedBlock(U64 cycle, ColorBufferBlock *cbBlock, std::set<U32>& generatedTickets);
    
    bool requestCompressedNormalBlock(U64 cycle, ColorBufferBlock *cbBlock, std::set<U32>& generatedTickets);
    
    bool requestCompressedBestBlock(U64 cycle, ColorBufferBlock *cbBlock, std::set<U32>& generatedTickets);
    
    /**
      *  Decompresses the color buffer blocks data of the texture block.
      *
      *  @param textureBlock    The texture block.
      */
    void decompressColorBufferBlocks(TextureBlock* textureBlock);
    
    /**
      *  Performs swizzling of the texture block contents. To do this sets up and uses the swizzling
      *  hardware emulation class.
      *
      *  @note The corresponding color buffer blocks are assumed to contain the allocated space with 
      *        valid data
      *
      *  @param textureBlock    The texture Block.
      *
      */
    void swizzleTextureBlock(TextureBlock* textureBlock);
};

} // namespace cg1gpu

#endif
