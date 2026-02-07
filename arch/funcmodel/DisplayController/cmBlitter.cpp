/**************************************************************************
 *
 */

/**
  *
  * @file Blitter.cpp
  *
  * Implements the Blitter class. This class is used to perform BitBlit operations
  *
  */

#include "cmBlitter.h"
#include "GPUMath.h"
#include "bmFragmentOperator.h"
#include <algorithm> // STL find() function
#include <map> 
#include <utility> // For pair<> data type


using namespace cg1gpu;
using namespace std;
using namespace cg1gpu::gpuStatistics;

/******************************************************************************************
 *                       Color Buffer Block class implementation                          *
 ******************************************************************************************/

ColorBufferBlock::ColorBufferBlock()
    :   address(0), block(0), state(ROPBlockState::UNCOMPRESSED), 
        compressedContents(0), contents(0), requested(0)
{
}

ColorBufferBlock::~ColorBufferBlock()
{
    if (compressedContents) delete[] compressedContents;
    if (contents) delete[] contents;
}

void ColorBufferBlock::print(std::ostream& os) const
{
    os << block;
    switch(state.state)
    {
       case ROPBlockState::CLEAR: os << "(CLEAR_COLOR)"; break;
       case ROPBlockState::UNCOMPRESSED: os << "(UNCOMPRESSED)"; break;
       case ROPBlockState::COMPRESSED: os << "(COMP_LEVEL_)" << state.comprLevel; break;
       default:
          panic("Blitter::ColorBufferBlock","print","Color buffer block state not expected");
    }

    if (!tickets.empty())
    {
        vector<U32>::const_iterator iter2 = tickets.begin();
 
        os << " Ticket list: {";
        
        while ( iter2 != tickets.end() )
        {
            os << (*iter2);
            iter2++;           
            if (iter2 != tickets.end()) os << ",";
        }    
        
        os << "}";
    }
}

/******************************************************************************************
 *                       Texture Block class implementation                               *
 ******************************************************************************************/
 
TextureBlock::TextureBlock() 
    :   xBlock(0), yBlock(0), address(0), block(0), contents(0), writeMask(0), written(0)
{
}

TextureBlock::~TextureBlock()
{
    if (contents) delete[] contents;
    if (writeMask) delete[] writeMask;

    vector<ColorBufferBlock*>::iterator iter = colorBufferBlocks.begin();
              
    while ( iter != colorBufferBlocks.end() )
    {
       delete (*iter);
       iter++;
    }
}

void TextureBlock::print(ostream& os) const
{
    os << "Texture Block: (" << xBlock << "," << yBlock << ")" << endl;
    os << "\tBlock id = " << block << ", address = " << address;
    
    if (colorBufferBlocks.empty())

        os << ", no colorBuffer blocks defined. " << endl;

    else    // color buffer blocks defined
    {   
        os << ", colorBuffer blocks: {";
              
        vector<ColorBufferBlock*>::const_iterator iter = colorBufferBlocks.begin();
              
        while ( iter != colorBufferBlocks.end() )
        {
           os << (*(*iter));
           iter++;
           if (iter != colorBufferBlocks.end()) os << ",";
        }
        os << "}" << endl;
    }
    
    if (!contents) os << "\tNo contents defined. " << endl;
    if (!writeMask) os << "\tNo write mask defined. " << endl;
    os << "\tWritten bytes = " << written << endl;
}


/******************************************************************************************
 *                       Swizzling Buffer class implementation                            *
 ******************************************************************************************/

//  Swizzling buffer constructor.  
SwizzlingBuffer::SwizzlingBuffer(U32 size, U32 readPortWidth, U32 writePortWidth):
    size(size), readPortWidth(readPortWidth), writePortWidth(writePortWidth), 
    highest_written_position(0), highest_read_position(0)
{
    if ( size % readPortWidth != 0 )
        CG_ASSERT("The buffer size must be multiple of the read port width");
        
    if ( size % writePortWidth != 0 )
        CG_ASSERT("The buffer size must be multiple of the write port width");
        
    max_read_position = (U32)((F32) size / (F32) readPortWidth) - 1;
    max_write_position = (U32)((F32) size / (F32) writePortWidth) - 1;
    
    inBuffer = new U08[size];
    outBuffer = new U08[size];
    
    for (int i = 0; i < size; i++)
        swzMap.push_back(i);
}

//  Swizzling buffer destructor.  
SwizzlingBuffer::~SwizzlingBuffer()
{
    delete[] inBuffer;
    delete[] outBuffer;
}
 
//  Resets the swizzling buffer.  
void SwizzlingBuffer::reset()
{
    for (int i = 0; i < size; i++)
        swzMap[i] = i;
        
    highest_written_position = highest_read_position = 0;
}

//  Fills the corresponding portion of the input buffer.  
void SwizzlingBuffer::writeInputData(U08* inputData, U32 position)
{
    if (position > max_write_position)
        CG_ASSERT("Write position out of range");
    
    if (position > highest_written_position)
        highest_written_position = position;
            
    U32 bufferOffset = position * writePortWidth;
    
    memcpy(&inBuffer[bufferOffset], inputData, writePortWidth);
}
    
//  Reads the corresponding portion of the ouput/result buffer.  
void SwizzlingBuffer::readOutputData(U08* outputData, U32 position)
{
    if (position > max_read_position)
        CG_ASSERT("Read position out of range");

    if (position > highest_read_position)
        highest_read_position = position;
        
    U32 bufferOffset = position * readPortWidth;
    
    memcpy(outputData, &outBuffer[bufferOffset], readPortWidth);
}

//  Performs the data swizzling.  
void SwizzlingBuffer::swizzle()
{
    for (int i = 0; i < (highest_written_position + 1) * writePortWidth; i++)
    {
        U32 idx = i;
        S32 dest = swzMap[i];

//printf("Swizzling Buffer  => Moving input byte %d to output byte position %d.\n", idx, dest);
        if (dest >= size && dest != -1)
            CG_ASSERT("a swizzle map element points to a buffer position out of range");
        
        if (dest != -1)
            outBuffer[dest] = inBuffer[idx];
    }    
}

//  Sets the swizzling mask for the corresponding portion.  
void SwizzlingBuffer::setSwzMask(std::vector<S32> _swzMap, U32 position)
{
    if (position > max_write_position)
        CG_ASSERT("Swz Map position out of range");
    
    U32 maskOffset = position * writePortWidth;
    
    for (int i = maskOffset; i < maskOffset + writePortWidth; i++)
        swzMap[i] = _swzMap[i - maskOffset];
}


/******************************************************************************************
 *                          Blitter class implementation                                  *
 ******************************************************************************************/

//  Blitter constructor method.  
Blitter::Blitter(BlitterTransactionInterface& transactionInterface,
                 U32 stampH, U32 stampW, U32 genH, U32 genW, U32 scanH, U32 scanW, U32 overH, U32 overW, 
                 U32 mortonBlockDim, U32 mortonSBlockDim, U32 textureBlockSize,
                 U32 blockStateLatency, U32 blocksCycle, U32 colorBufferBlockSize, U32 blockQueueSize, U32 decompressLatency,
                 bool saveBlitSource) :

    readBlocks(transactionInterface.getSM().getNumericStatistic("ReadBlocks", U32(0), "Blitter", "Blitter")),
    writtenBlocks(transactionInterface.getSM().getNumericStatistic("WrittenBlocks", U32(0), "Blitter", "Blitter")),
    
    requestPendingQueueFull(transactionInterface.getSM().getNumericStatistic("RequestPendingQueueFull", U32(0), "Blitter", "Blitter")),
    receiveDataPendingWakeUpQueueFull(transactionInterface.getSM().getNumericStatistic("ReceiveDataPendingWakeUpQueueFull", U32(0), "Blitter", "Blitter")),
    decompressionPendingQueueFull(transactionInterface.getSM().getNumericStatistic("DecompressionPendingQueueFull", U32(0), "Blitter", "Blitter")),
    swizzlingPendingQueueFull(transactionInterface.getSM().getNumericStatistic("SwizzlingPendingQueueFull", U32(0), "Blitter", "Blitter")),
    writePendingQueueFull(transactionInterface.getSM().getNumericStatistic("WritePendingQueueFull", U32(0), "Blitter", "Blitter")),
    
    clearColorBlocks(transactionInterface.getSM().getNumericStatistic("ColorStateBlocksClear", U32(0), "Blitter", "Blitter")),
    uncompressedBlocks(transactionInterface.getSM().getNumericStatistic("ColorStateBlocksUncompressed", U32(0), "Blitter", "Blitter")),
    compressedBestBlocks(transactionInterface.getSM().getNumericStatistic("ColorStateBlocksCompressedBest", U32(0), "Blitter", "Blitter")),
    compressedNormalBlocks(transactionInterface.getSM().getNumericStatistic("ColorStateBlocksCompressedNormal", U32(0), "Blitter", "Blitter")),
    compressedWorstBlocks(transactionInterface.getSM().getNumericStatistic("ColorStateBlocksCompressedWorst", U32(0), "Blitter", "Blitter")),

    transactionInterface(transactionInterface),
    stampH(stampH), stampW(stampW), genH(genH), genW(genW), scanH(scanH), scanW(scanW), overH(overH), overW(overW),
    mortonBlockDim(mortonBlockDim), mortonSBlockDim(mortonSBlockDim), textureBlockSize(textureBlockSize), blockStateLatency(blockStateLatency), 
    blocksCycle(blocksCycle), colorBufferBlockSize(colorBufferBlockSize), blockQueueSize(blockQueueSize), saveBlitSource(saveBlitSource),
    
    maxColorBufferBlocksPerTextureBlock(computeColorBufferBlocksPerTextureBlock( (U32) CG_LOG2((F64)textureBlockSize), 
                                                                                 (U32) CG_LOG2((F64)colorBufferBlockSize))),

    maxTransactionsPerTextureBlock(computeColorBufferBlocksPerTextureBlock( (U32) CG_LOG2((F64)textureBlockSize), 
                                                                                 (U32) CG_LOG2((F64)colorBufferBlockSize)) *
                                   (U32)(ceil((F32) colorBufferBlockSize / (F32) MAX_TRANSACTION_SIZE)) ),
                                                                                 
    decompressionLatency(decompressLatency), 
    colorBufferStateReady(false), blitIniX(0), blitIniY(0), blitXOffset(0), blitYOffset(0), blitHeight(0), blitWidth(0), 
    blitDestinationAddress(0x800000), blitDestinationBlocking(GPU_TXBLOCK_TEXTURE), blitTextureWidth2(0), blitDestinationTextureFormat(GPU_RGBA8888), 
    hRes(400), vRes(400), startX(0), startY(0), width(400), height(400),

    receiveDataPendingWakeUpQueue(0, maxTransactionsPerTextureBlock),
    
    swzBuffer(colorBufferBlockSize * maxColorBufferBlocksPerTextureBlock, textureBlockSize, colorBufferBlockSize),
    
    // The hardware specific latencies.  
    newBlockLatency(maxColorBufferBlocksPerTextureBlock),
    swizzlingLatency(maxColorBufferBlocksPerTextureBlock)
{
    string queueName;

    queueName.clear();
    queueName.append("Blitter::requestPendingQueue");
    requestPendingQueue.setName(queueName);
    requestPendingQueue.resize(4);

    queueName.clear();
    queueName.append("Blitter::receiveDataPendingWakeUpQueue");
    receiveDataPendingWakeUpQueue.setName(queueName);
    receiveDataPendingWakeUpQueue.resize((U32)((F32)MAX_WAKEUPQUEUE_COMPARISONS_PER_CYCLE/(F32)maxTransactionsPerTextureBlock));
    
    queueName.clear();
    queueName.append("Blitter::decompressionPendingQueue");
    decompressionPendingQueue.setName(queueName);
    decompressionPendingQueue.resize(4);
    
    queueName.clear();
    queueName.append("Blitter::swizzlingPendingQueue");
    swizzlingPendingQueue.setName(queueName);
    swizzlingPendingQueue.resize(4);

    queueName.clear();
    queueName.append("Blitter::writePendingQueue");
    writePendingQueue.setName(queueName);
    writePendingQueue.resize(4);
    
    /*colorBufferSize = ((U32) ceil(vRes / (F32) (overH * scanH * genH * stampH))) *
                      ((U32) ceil(hRes / (F32) (overW * scanW * genW * stampW))) *
                      overW * overH * scanW * scanH * genW * genH * stampW * stampH * bytesPixel;*/

    // Setup display in the Pixel Mapper.
    pixelMapper.setupDisplay(hRes, vRes, stampW, stampH, genW, genH, scanW, scanH, overW, overH, 1, 4);
    colorBufferSize = pixelMapper.computeFrameBufferSize();                      

    colorBuffer = NULL;
                          
    clearColor[0] = 0.0f;
    clearColor[1] = 0.0f;
    clearColor[2] = 0.0f;
    clearColor[3] = 1.0f;
    
    colorBufferFormat = GPU_RGBA8888;
}

//  Reset current blitter state registers values.  
void Blitter::reset()
{
    hRes = 400;
    vRes = 400;
    startX = 0;
    startY = 0;
    width = 400; 
    height = 400;
    blitIniX = 0;
    blitIniY = 0;
    blitHeight = 400;
    blitWidth = 400; 
    blitXOffset = 0;
    blitYOffset = 0;
    blitDestinationAddress = 0x8000000;
    blitTextureWidth2 = 9; // 2^9 = 512 (> 400)
    blitDestinationTextureFormat = GPU_RGBA8888;
    blitDestinationBlocking = GPU_TXBLOCK_TEXTURE;

    clearColor[0] = 0.0f;
    clearColor[1] = 0.0f;
    clearColor[2] = 0.0f;
    clearColor[3] = 1.0f;
    colorBufferFormat = GPU_RGBA8888;
    bytesPixel = 4;
    multisampling = false;
    msaaSamples = 2;
        
    /*colorBufferSize = ((U32) ceil(vRes / (F32) (overH * scanH * genH * stampH))) *
                      ((U32) ceil(hRes / (F32) (overW * scanW * genW * stampW))) *
                      overW * overH * scanW * scanH * genW * genH * stampW * stampH * bytesPixel;*/

    // Setup display in the Pixel Mapper.
    U32 samples = multisampling ? msaaSamples : 1;
    pixelMapper.setupDisplay(hRes, vRes, stampW, stampH, genW, genH, scanW, scanH, overW, overH, samples, bytesPixel);
    colorBufferSize = pixelMapper.computeFrameBufferSize();                      

    if (colorBuffer != NULL)
        delete [] colorBuffer;
        
    colorBuffer = NULL;
                    
    colorBufferState.clear();
}

/**
 *  Register setting functions. 
 */
void Blitter::setDisplayXRes(U32 _hRes)
{
    hRes = _hRes;
    
    /*colorBufferSize = ((U32) ceil(vRes / (F32) (overH * scanH * genH * stampH))) *
                      ((U32) ceil(hRes / (F32) (overW * scanW * genW * stampW))) *
                      overW * overH * scanW * scanH * genW * genH * stampW * stampH * bytesPixel;*/
}

void Blitter::setDisplayYRes(U32 _vRes)
{
    vRes = _vRes;
    
    /*colorBufferSize = ((U32) ceil(vRes / (F32) (overH * scanH * genH * stampH))) *
                      ((U32) ceil(hRes / (F32) (overW * scanW * genW * stampW))) *
                      overW * overH * scanW * scanH * genW * genH * stampW * stampH * bytesPixel;*/
}

void Blitter::setClearColor(Vec4FP32 _clearColor)
{
    clearColor = _clearColor;
}

void Blitter::setColorBufferFormat(TextureFormat format)
{
    colorBufferFormat = format;
    
    switch(colorBufferFormat)
    {
        case GPU_RGBA8888:
        case GPU_RG16F:
        case GPU_R32F:
            bytesPixel = 4;
            break;
        
        case GPU_RGBA16F:
        case GPU_RGBA16:
            bytesPixel = 8;
            break;
        default:
            CG_ASSERT("Unsupported color buffer format.");
            break;
    }
}

void Blitter::setBackBufferAddress(U32 address)
{
    backBufferAddress = address;
}

void Blitter::setMultisampling(bool enable)
{
    multisampling = enable;
}

void Blitter::setMSAASamples(U32 samples)
{
    msaaSamples = samples;
}

void Blitter::setViewportStartX(U32 _startX)
{
    startX = _startX;
}
    
void Blitter::setViewportStartY(U32 _startY)
{
    startY = _startY;
}
    
void Blitter::setViewportWidth(U32 _width)
{
    width = _width;
}
    
void Blitter::setViewportHeight(U32 _height)
{
    height = _height;
}
   
void Blitter::setBlitIniX(U32 iniX)
{
    blitIniX = iniX;
}

void Blitter::setBlitIniY(U32 iniY)
{
    blitIniY = iniY;
}

void Blitter::setBlitXOffset(U32 xoffset)
{
    blitXOffset = xoffset;
}
    
void Blitter::setBlitYOffset(U32 yoffset)
{
    blitYOffset = yoffset;
}
    
void Blitter::setBlitWidth(U32 width)
{
    blitWidth = width;
}

void Blitter::setBlitHeight(U32 height)
{
    blitHeight = height;
}

void Blitter::setDestAddress(U32 address)
{
    blitDestinationAddress = address;
}

void Blitter::setDestTextWidth2(U32 width)
{
    blitTextureWidth2 = width;
}

void Blitter::setDestTextFormat(TextureFormat format)
{
    blitDestinationTextureFormat = format;
}

void Blitter::setDestTextBlock(TextureBlocking blocking)
{
    blitDestinationBlocking = blocking;
}

void Blitter::setColorBufferState(ROPBlockState* _colorBufferState, U32 colorStateBufferBlocks)
{
    if ( (colorBufferSize >> GPUMath::calculateShift(colorBufferBlockSize)) != colorStateBufferBlocks )
        panic("Blitter","setColorBufferState","A different number of color buffer state blocks is specified");

    //  Clear old color buffer state.
    colorBufferState.clear();
            
    //  Set new color buffer state.
    for (int i = 0; i < colorStateBufferBlocks; i++)
        colorBufferState.push_back(_colorBufferState[i]);

    //printf("Blitter => Setting color buffer state | num blocks = %d\n", colorStateBufferBlocks);
    //printf("Blitter => Color block 153 (param) -> state = %d level = %d\n", _colorBufferState[153].getState(), _colorBufferState[153].getComprLevel());
    //printf("Blitter => Color block 153 (buffer) -> state = %d level = %d\n", colorBufferState[153].getState(), colorBufferState[153].getComprLevel());
            
    colorBufferStateReady = true;
}

//  Initiates the bit blit operation.  
void Blitter::startBitBlit()
{
    //  Initialize the coordinates for the first texture block to update.  
    startXBlock = blitXOffset >> mortonBlockDim;
    startYBlock = blitYOffset >> mortonBlockDim;
    
    //  Compute last texture block.  
    lastXBlock = (blitXOffset + blitWidth - 1) >> mortonBlockDim;
    lastYBlock = (blitYOffset + blitHeight - 1) >> mortonBlockDim;
    
    //  Current texture bloks begins as the first.  
    currentXBlock = startXBlock;
    currentYBlock = startYBlock;
    
    totalBlocksToWrite = (lastXBlock - startXBlock + 1) * (lastYBlock - startYBlock + 1);

    GPU_DEBUG(
        printf("%s::Blitter => Bit blit operation initiated. %d texture blocks to write starting from address = %X.\n", 
                transactionInterface.getBoxName().c_str(), totalBlocksToWrite, blitDestinationAddress);
    )
        
    // Setup display in the Pixel Mapper.
    U32 samples = multisampling ? msaaSamples : 1;
    pixelMapper.setupDisplay(hRes, vRes, stampW, stampH, genW, genH, scanW, scanH, overW, overH, samples, bytesPixel);
    colorBufferSize = pixelMapper.computeFrameBufferSize();
        
    //  Initialize other counters.          

    swizzlingCycles = swizzlingLatency;
    newBlockCycles = newBlockLatency;
    decompressionCycles = decompressionLatency;

    blocksWritten = 0;
    nextTicket = 0;

    //  Check if the blit source data must be saved into a file.
    if (saveBlitSource)
    {
        //  Allocate space for the color buffer.
        colorBuffer = new U08[colorBufferSize];
        
        CG_ASSERT_COND(!(colorBuffer == NULL), "Error allocating memory for the source blit color buffer data.");        
    }
    
//printf("Blitter::startBitBlit => created color buffer %p with size %d\n", colorBuffer, colorBufferSize);
}

//  Notifies that a memory read transaction has been completed.  
void Blitter::receivedReadTransaction(U32 ticket, U32 size)
{
    GPU_DEBUG(
        printf("%s::Blitter => Received transaction with ticket id = %d and size = %d.\n", transactionInterface.getBoxName().c_str(), ticket, size);
    )

        /*  Wake up all the texture blocks waiting for this transaction completeness. Texture Blocks waiting
         *  more than one transaction will remain asleep until all transactions wake them up. */
    receiveDataPendingWakeUpQueue.wakeup(ticket);
}

//  Notifies that the current bit blit operation has finished.  
bool Blitter::currentFinished()
{
    return (blocksWritten == totalBlocksToWrite);
}

/**
 *  Simulates a cycle of the blitter unit.
 *  
 *  The blitter pipeline is implemented as follows:
 *
 *      Given the destination texture image partitioned in texture blocks of texture block size bytes:
 *
 *      1. The First stage generates texture block structures for the tiles (X,Y) that cover the 
 *         texture region to update. This structure is filled with all the necessary information, such as:
 *    
 *         1.1 The color buffer blocks to request to memory to fill the texture block with data,
 *         1.2 The write mask of the valid bytes of the texture block that will be finally written
 *             to memory (the texture region can partially cover the generated texture block.
 *         1.3 The start address in memory of the texture block.
 *         1.4 The mapping or correspondence between pixel byte positions in the color buffer blocks and
 *             the texel bytes position in the texture block. This is necessary when the destination texture
 *             is arranged in a different blocking format (such as Morton) or when the color buffer block
 *             doesn�t fit exactly in the texture block shape.
 *
 *         The operation is performed each newBlockLatency cycles. When finished the texture block is 
 *         enqueued in the "pendingRequestQueue".
 *
 *      2. The Second stage generates memory read transactions for the Texture Blocks in the "requestPendingQueue". 
 *         A memory read request is only possible if there is any write request the same cycle, avoiding
 *         the situation that only reads are performed and no writes make progress, and consecuently, the different 
 *         pipeline queues get full and the pipeline gets stalled. The first time we get the head texture block of the
 *         pending request queue, the block is enqueued on the "receivedDataPendingWakeUpQueue" with the corresponding
 *         transaction tickets (events) to wait for. Next cycles, the event list can be completed adding new transactions 
 *         tickets as new memory read transactions for the texture block are generated.
 *         
 *      3. The Third stage tries to pick-up/select those Texture blocks whose corresponding memory transactions ticket
 *         have arrived (transactions are completed) and hence, the corresponding memory blocks are available for
 *         further processing. The selected Texture Block is enqueued in the "decompressionPendingQueue".
 *
 *      4. The Fourth stage decompress the read color buffer blocks of the head Texture Block in the decompressionPendingQueue.
 *         The operation is performed each decompressionLatency cycles. When finished the texture block is enqueued in the
 *         "swizzlingPendingQueue".
 *  
 *      5. The Fifth stage performs the swizzling operation over the head of the "swizzlingPendingQueue". It takes all the color
 *         buffer blocks data and swizzling masks and performs the operation using a Swizzling Buffer hardware. The corresponding
 *         operation latency is applied and, when finished, the texture block is enqueued in the "writePendingQueue".
 *      
 *      6. Finally, the ready texture blocks of the "writePendingQueue" are written to memory sending the related memory write
 *         transactions.
 *       
 *    As can be seen in the clock() code, the stages actually are processed in inversed order. This is because
 *    the work performed in a stage is only available for the next stage on the next cycle (following a
 *    pipelined model).
 */

void Blitter::clock(U64 cycle)
{
    /* Sixth stage: Send a new write transacion for the next ready texture block in the write pending queue. 
     *              If a write is performed this cycle then a block read request will not be allowed.
     */
    memWriteRequestThisCycle = sendNextReadyTextureBlock(cycle, writePendingQueue);
        
    /** 
     * Fifth stage: Look up the swizzling pending queue for texture blocks and apply swizzling operation.
     *
     */
    if (!swizzlingPendingQueue.empty())
    {
        //  Decrement the swizzling remaining cycles.  
        if (swizzlingCycles > 0) swizzlingCycles--;

        GPU_DEBUG(
           printf("%s::Blitter %lld => Swizzling remaining cycles = %d.\n", transactionInterface.getBoxName().c_str(), cycle, swizzlingCycles);
        )
        
        if (swizzlingCycles == 0 && !writePendingQueue.full())
        {
            GPU_DEBUG(
               printf("%s::Blitter %lld => Swizzling finished.\n", transactionInterface.getBoxName().c_str(), cycle);
            )
        
            TextureBlock* textureBlock = swizzlingPendingQueue.head();
            
            //  Perform swizzling.  
            swizzleTextureBlock(textureBlock);
            
            //  Enqueue it to the texture block queue for blocks ready to be written.  
            writePendingQueue.add(textureBlock);
            
            //  Remove from the swizzling pending queue.             
            swizzlingPendingQueue.remove();
            
            //  Reset swizzling operation remaining cycles.  
            swizzlingCycles =  swizzlingLatency;
        }
        else if (writePendingQueue.full()) //  Update statistic.  
            writePendingQueueFull++;
    }

    /**
     * Fourth stage: Select next texture block whose color buffer blocks must be decompressed.
     *
     */
    if (!decompressionPendingQueue.empty())
    {
        //  Decrement the decompression remaining cycles.  
        if (decompressionCycles > 0) decompressionCycles--;

        GPU_DEBUG(
           printf("%s::Blitter %lld => Decompression remaining cycles = %d.\n", transactionInterface.getBoxName().c_str(), cycle, decompressionCycles);
        )
        
        //  Check if decompression operation finished.  
        if (decompressionCycles == 0 && !swizzlingPendingQueue.full())
        {
            GPU_DEBUG(
                printf("%s::Blitter %lld => Decompression finished.\n", transactionInterface.getBoxName().c_str(), cycle);
            ) 
        
            TextureBlock* textureBlock = decompressionPendingQueue.head();
            
            //  Perform decompression.      
            decompressColorBufferBlocks(textureBlock);

            //  Enqueue to the swizzling operation queue.  
            swizzlingPendingQueue.add(textureBlock);

            //  Remove from decompression queue.              
            decompressionPendingQueue.remove();
            
            //  Reset decompression remainder cycles.  
            decompressionCycles = decompressionLatency;
        }
        else if (swizzlingPendingQueue.full()) //  Update statistic.  
            swizzlingPendingQueueFull++;

    }

    /**
     * Third stage: Select completely received texture blocks. These texture blocks have all the 
     *              color buffer block necessary data already available.
     */
    if (!receiveDataPendingWakeUpQueue.empty())
    {
        //  Select the next ready/selectable texture block.  
        TextureBlock* textureBlock = receiveDataPendingWakeUpQueue.select();

        if (textureBlock && !decompressionPendingQueue.full())
        {
            GPU_DEBUG(
               printf("%s::Blitter %lld => Block (%d,%d) Fully received.\n", transactionInterface.getBoxName().c_str(), cycle, textureBlock->xBlock, textureBlock->yBlock);
            )
            
            //  Update statistic.  
            for (int i = 0; i < textureBlock->colorBufferBlocks.size(); i++)
                readBlocks++;

            //  Send Color Buffer Blocks to the decompression queue.  
            decompressionPendingQueue.add(textureBlock);
            
            //  Remove from wakeup queue.  
            receiveDataPendingWakeUpQueue.remove(textureBlock);
        }
        else if (decompressionPendingQueue.full()) //  Update statistic.  
            decompressionPendingQueueFull++;
    }
    
    /**
     * Second stage: Request the color buffer blocks sending 
     *              the corresponding memory read transactions  
     */
    if (!requestPendingQueue.empty() && !memWriteRequestThisCycle)
    {
        /*  There�s an entry of request pending (either already in the Wakeup queue or not) and 
         *  the read requests are allowed (no write request this cycle).  */
        
        //  Tells when we have finished with a block.  
        bool allBlocksRequested = false; 
        
        TextureBlock* textureBlock = requestPendingQueue.head();
        
        /*  Check if already exists in the Wakeup queue. If not generate transactions and enqueue it 
         *  for the first time.  */
        if (!receiveDataPendingWakeUpQueue.exists(textureBlock) && !receiveDataPendingWakeUpQueue.full())
        {
             set<U32> generatedTickets;
             
             //  For each block send as many read request as possible.   
             allBlocksRequested = requestColorBufferBlocks(cycle, textureBlock, generatedTickets);
             
             //  Enqueue at the Wakeup queue with the generated tickets as the events to wait for.  
             receiveDataPendingWakeUpQueue.add(textureBlock, generatedTickets, computeTotalTransactionsToRequest(textureBlock));
        }
        else if (receiveDataPendingWakeUpQueue.exists(textureBlock)) 
        { 
             // Already enqueued in the Wakeup queue.  

             set<U32> generatedTickets;
             
             //  For each block send as many read request as possible.   
             allBlocksRequested = requestColorBufferBlocks(cycle, textureBlock, generatedTickets);
             
             //  Update the Wakeup queue with the new generated tickets to wait for.  
             receiveDataPendingWakeUpQueue.addEventsToEntry(textureBlock, generatedTickets);
             
             if (receiveDataPendingWakeUpQueue.full()) //  Update statistic.  
                 receiveDataPendingWakeUpQueueFull++;
        }
        else if (receiveDataPendingWakeUpQueue.full()) //  Update statistic.  
                 receiveDataPendingWakeUpQueueFull++;
        
        if ( allBlocksRequested )
        {
            GPU_DEBUG(
               printf("%s::Blitter %lld => Block (%d,%d) Fully requested.\n", transactionInterface.getBoxName().c_str(), cycle, textureBlock->xBlock, textureBlock->yBlock);
            )
            
            //  If all read transactions generated remove head texture block.  
            requestPendingQueue.remove();
        }
    }

    /**  
     *  First stage: Every texture block creation latency cycles, 
     *               fill the request pending queue with a new texture block.
     */
    if (colorBufferStateReady && !requestPendingQueue.full())
    {
        if (currentYBlock <= lastYBlock && currentXBlock <= lastXBlock)
        {
              //  If no remaining cycles perform new texture block generation.  
             if (newBlockCycles == 0)
             {   
                    //  The texture block allocated space will be deleted after going out of the last queue.               
                 TextureBlock* textureBlock = new TextureBlock;
                 
                 textureBlock->xBlock = currentXBlock;
                 textureBlock->yBlock = currentYBlock;
     
                 /*  The texture block information such as the texture block address in memory,
                  *  the write mask, the color buffer blocks to request and mapping of pixels
                  *  to texels for each requested block, is generated using this function.
                  */
                 processNewTextureBlock(cycle, currentXBlock, currentYBlock, textureBlock);
 
                 GPU_DEBUG(
                    printf("%s::Blitter %lld => New block (%d,%d) generated.\n", transactionInterface.getBoxName().c_str(), cycle, textureBlock->xBlock, textureBlock->yBlock);
                    //cout << (*textureBlock) << endl;
                 )
                 
                 //  Enqueue the texture block for the next stage processing.  
                 requestPendingQueue.add(textureBlock);
                                      
                 if (currentXBlock == lastXBlock)
                 {
                     currentXBlock = startXBlock;
                     currentYBlock++;
                 }
                 else
                     currentXBlock++;
                 
                 //  Reset new block generation remaining cycles.  
                 newBlockCycles = newBlockLatency;
             }
             else 
             {
                 GPU_DEBUG(
                    printf("%s::Blitter %lld => New block generation remaining cycles = %d.\n", transactionInterface.getBoxName().c_str(), cycle, newBlockCycles);
                 )
 
                 //  Decrement new block generation remaining cycles.  
                 newBlockCycles--;
             }
         }        
    }
    if (requestPendingQueue.full()) //  Update statistic.  
        requestPendingQueueFull++;
}

/****************************************************************************************
 *                          Auxiliar functions implementation                           *
 ****************************************************************************************/
 
//  Computes the maximum number of necessary color buffer blocks to fill a single texture block.  
U32 Blitter::computeColorBufferBlocksPerTextureBlock(U32 TextureBlockSize2, U32 ColorBufferBlockSize2)
{
    const U32& t = TextureBlockSize2;
    const U32& c = ColorBufferBlockSize2;
    
    U32 base = (U32) ceil( (F32)(t - c) / 2.0f ) + 2;
    
    return base * base; // base^2 x 
}
 

//  Translates texel coordinates to the morton address offset starting from the texture base address.  
U32 Blitter::texel2MortonAddress(U32 i, U32 j, U32 blockDim, U32 sBlockDim, U32 width)
{
    U32 address;
    U32 texelAddr;
    U32 blockAddr;
    U32 sBlockAddr;

    //  Compute the address of the texel inside the block using Morton order.  
    texelAddr = GPUMath::morton(blockDim, i, j);

    //  Compute the address of the block inside the superblock using Morton order.  
    blockAddr = GPUMath::morton(sBlockDim, i >> blockDim, j >> blockDim);

    //  Compute the address of the superblock inside the cache.  
    sBlockAddr = ((j >> (sBlockDim + blockDim)) << GPU_MAX(S32(width - (sBlockDim + blockDim)), S32(0))) + (i >> (sBlockDim + blockDim));

    //  Compute the final address.  
    address = (((sBlockAddr << (2 * sBlockDim)) + blockAddr) << (2 * blockDim)) + texelAddr;

    return address;
}

//  Returns the number of bytes of each texel for a texture using this format.  
U32 Blitter::getBytesPerTexel(TextureFormat format)
{
    //  Only one format supported at the moment.  
    switch(format)
    {
        case GPU_RGBA8888: return 4; 
            break;
        default:
            CG_ASSERT("Unsupported blit destination texture format.");

            //  Remove VS2005 warning.
            return 0;
            break;
    }
}

//  Decompresses a block according to the compression state and using the specified clearColor.  
void Blitter::decompressBlock(U08* outBuffer, U08* inBuffer, U32 colorBufferBlockSize, ROPBlockState state, Vec4FP32 clearColor)
{
    int i;
    
    U08 clearColorData[MAX_BYTES_PER_COLOR];
    
    //  Convert the float point clear color to the color buffer format.
    switch(colorBufferFormat)
    {
        case GPU_RGBA8888:
        
            clearColorData[0] = U08(clearColor[0] * 255.0f);
            clearColorData[1] = U08(clearColor[1] * 255.0f);
            clearColorData[2] = U08(clearColor[2] * 255.0f);
            clearColorData[3] = U08(clearColor[3] * 255.0f);
            
            break;

        case GPU_RG16F:
        
            ((f16bit *) clearColorData)[0] = GPUMath::convertFP32ToFP16(clearColor[0]);
            ((f16bit *) clearColorData)[1] = GPUMath::convertFP32ToFP16(clearColor[1]);

            break;
            
        case GPU_R32F:
        
            ((F32 *) clearColorData)[0] = clearColor[0];

            break;

        case GPU_RGBA16F:
        
            ((f16bit *) clearColorData)[0] = GPUMath::convertFP32ToFP16(clearColor[0]);
            ((f16bit *) clearColorData)[1] = GPUMath::convertFP32ToFP16(clearColor[1]);
            ((f16bit *) clearColorData)[2] = GPUMath::convertFP32ToFP16(clearColor[2]);
            ((f16bit *) clearColorData)[3] = GPUMath::convertFP32ToFP16(clearColor[3]);
            
            break;
            
        default:
            CG_ASSERT("Unsupported color buffer format.");
            break;
    }
    
    //  Select compression method.  
    switch (state.state)
    {
        case ROPBlockState::CLEAR:

            GPU_DEBUG(
                printf("%s::Blitter => Decompressing CLEAR block.\n", transactionInterface.getBoxName().c_str());
            )

            //  Fill with the clear color.  
            for (i = 0; i < colorBufferBlockSize ; i += bytesPixel)
                for(U32 j = 0; j < (bytesPixel >> 2); j++)
                    *((U32 *) &outBuffer[i + j * 4]) = ((U32 *) clearColorData)[j];
            
            //  Update stat.  
            clearColorBlocks++;
                
            break;

        case ROPBlockState::UNCOMPRESSED:

            GPU_DEBUG(
                printf("%s::Blitter => Decompressing UNCOMPRESSED block.\n", transactionInterface.getBoxName().c_str());
            )
            
            //  Fill with the read data.  
            for (i = 0; i < colorBufferBlockSize ; i+= sizeof(U32))
                *((U32 *) &outBuffer[i]) = *((U32 *) &inBuffer[i]);

            //  Update stat.  
            uncompressedBlocks++;
                            
            break;

        case ROPBlockState::COMPRESSED:

            GPU_DEBUG(
                printf("%s::Blitter => Decompressing COMPRESSED LEVEL %i block.\n", 
                        transactionInterface.getBoxName().c_str(), state.comprLevel);
            )

            //  Uncompress the block.  
            bmoColorCompressor::getInstance().uncompress(
                    inBuffer, 
                    (U32 *) outBuffer, 
                    colorBufferBlockSize >> 2, 
                    state.getComprLevel());

            //  Update stat.  
            switch (state.comprLevel) {
            case 0: compressedBestBlocks++; break;
            case 1: compressedNormalBlocks++; break;
            case 2: compressedWorstBlocks++; break;
            default:
                GPU_DEBUG(
                        printf("Warning: Blitter: There isn't statistic for compression level 2."); 
                )
                break;
            }
                            
            break;

        /*case ROP_BLOCK_COMPRESSED_BEST:

            GPU_DEBUG(
                printf("%s::Blitter => Decompressing COMPRESSED BEST block.\n", transactionInterface.getBoxName().c_str());
            )

              Uncompress the block.  
            bmoFragmentOperator::hiloUncompress(
                inBuffer,
                (U32 *) outBuffer,
                colorBufferBlockSize >> 2,
                bmoFragmentOperator::COMPR_L1,
                ColorCacheV2::COMPRESSION_HIMASK_NORMAL,
                ColorCacheV2::COMPRESSION_HIMASK_BEST,
                ColorCacheV2::COMPRESSION_LOSHIFT_NORMAL,
                ColorCacheV2::COMPRESSION_LOSHIFT_BEST);
                
              Update stat.  
            compressedBestBlocks++;
            
            break;*/

        default:
            CG_ASSERT("Undefined compression method.");
            break;
    }
}

//  Translates an address to the start of a block into a block number.  
U32 Blitter::address2block(U32 address, U32 blockSize)
{
    U32 block;

    //  Calculate the shift bits for block addresses.  
    U32 blockShift = GPUMath::calculateShift(blockSize);

    //  Translate start block/line address to a block address.  
    block = address >> blockShift;

    return block;
}

//  Generates the proper write mask for the memory transaction.  
void Blitter::translateBlockWriteMask(U32* outputMask, bool* inputMask, U32 size)
{
    for (int i = 0; i < size; i++)
    {
        if (inputMask[i])
            ((U08 *) outputMask)[i] = 0xff;
        else
            ((U08 *) outputMask)[i] = 0x00;
    }
}
    
//  Sends the next write memory transaction for the specified texture block.  
bool Blitter::sendNextWriteTransaction(U64 cycle, TextureBlock* textureBlock)
{
    //  Compute the transaction size.  
    U32 size = GPU_MIN(MAX_TRANSACTION_SIZE, textureBlockSize - textureBlock->written);

    U32 blockOffset = textureBlock->written;
    
    //  Get the write mask in the memory transaction format.  
    U32* writeMask = new U32[(U32) ((F32) textureBlockSize / (F32) sizeof(U32))];

    translateBlockWriteMask(writeMask, &textureBlock->writeMask[blockOffset], size);

    //  Send the memory transaction using the related interface.      
    
    if (transactionInterface.sendMaskedWriteTransaction(
        cycle, 
        size, 
        textureBlock->address + blockOffset, 
        &textureBlock->contents[blockOffset],
        writeMask,
        0))
    {
        GPU_DEBUG(
              printf("%s::Blitter %lld => Writing memory at %x %d bytes for block %d.\n",
                     transactionInterface.getBoxName().c_str(), cycle, textureBlock->address + blockOffset, size, textureBlock->block);
        )
                    
        //  Update requested block bytes counter.  
        textureBlock->written += size;
     
        delete[] writeMask;            
        return true;
    }
    else
    {
        //  No memory transaction generated.  

        delete[] writeMask;
        return false;
    }
}

//  Examines ready texture blocks and sends the next transaction of the head block.  
bool Blitter::sendNextReadyTextureBlock(U64 cycle, tools::Queue<TextureBlock*>& writePendingQueue)
{
    if (writePendingQueue.empty())
    {
        //  No memory write transaction generated.  
        return false;
    }
    else
    {
        TextureBlock* textureBlock;
        
        textureBlock = writePendingQueue.head();
        
        if (textureBlock->written == textureBlockSize)
        {
            //  Remove head block and pick up the next one.         
            writePendingQueue.remove();
            
            //  Update block counter for finishing condition.  
            blocksWritten++;
            
            //  Update statistic.  
            writtenBlocks++;

            GPU_DEBUG(
                    printf("%s::Blitter => Block (%d,%d) fully written to memory (%d of %d written).\n", transactionInterface.getBoxName().c_str(), textureBlock->xBlock, textureBlock->yBlock, blocksWritten, totalBlocksToWrite);
            )
            
            //  Delete the allocated space for this object.  
            delete textureBlock;
            
            if (writePendingQueue.empty())
            {
                //  No memory write transaction generated.  
                return false;
            }
            else
            {
                textureBlock = writePendingQueue.head();
                return sendNextWriteTransaction(cycle, textureBlock);
            }
        }
        else
        {
             return sendNextWriteTransaction(cycle, textureBlock);
        }
    }
}

//  Generates a texture block with all the necessary information.  
void Blitter::processNewTextureBlock(U64 cycle, U32 xBlock, U32 yBlock, TextureBlock* textureBlock)
{

    U32 texelsPerDim;
    U32 texelsPerBlock;
    
    switch(blitDestinationBlocking)
    {
        case GPU_TXBLOCK_TEXTURE:
        
            //  Compute the number of horizontal or vertical texels in the morton block.
            texelsPerDim = (U32) GPU_POWER2OF((F64)mortonBlockDim);
            
            //  Compute total number of texels in the morton block.
            texelsPerBlock = (U32) GPU_POWER2OF ((F64) (mortonBlockDim << 1));
        
            break;

        case GPU_TXBLOCK_FRAMEBUFFER:
            
            CG_ASSERT("Blits to textures in framebuffer format are not working.");
            
            texelsPerDim = STAMP_WIDTH;
            texelsPerBlock = STAMP_WIDTH * STAMP_HEIGHT;
            
            break;
        
        default:
        
            CG_ASSERT("Unknown destination tile blocking format.");
        
            break;
    }
    
     
    //  Compute the texel coordinates of the bottom-left corner of the texture block.
    U32 startBlockTexelX = xBlock * texelsPerDim;
    U32 startBlockTexelY = yBlock * texelsPerDim;
    
    //  Compute the texel coordinate of the top-right corner of the texture block.
    U32 endBlockTexelX = startBlockTexelX + texelsPerDim - 1;
    U32 endBlockTexelY = startBlockTexelY + texelsPerDim - 1;
    

    //  Compute the bytes per texel according to the texture format.
    U32 bytesTexel = getBytesPerTexel(blitDestinationTextureFormat);
    
    //  Bytes in a texture morton block.
    U32 bytesPerBlock = texelsPerBlock * bytesTexel;
    
    //  Allocate the block mask space. Allocating one boolean for each block byte.
    bool* mask = new bool[bytesPerBlock];
    
    //  Initialize mask all to false.
    for (int i = 0; i < bytesPerBlock; i++)
        mask[i] = false;
    
    U32 startBlockAddress;

    //  Compute the address (in fact, the offset from texture base address) 
    //  of the first texel inside the texture block. It depends on the destination
    //  texture blocking/tiling order. 
    //
    switch(blitDestinationBlocking)
    {
        case GPU_TXBLOCK_TEXTURE:
            
            //  Compute the morton address for the first corner texel of the block.
            startBlockAddress = texel2MortonAddress(startBlockTexelX, 
                                                    startBlockTexelY, 
                                                    mortonBlockDim, 
                                                    mortonSBlockDim, 
                                                    blitTextureWidth2) * bytesTexel;
                                                    
            break;

        case GPU_TXBLOCK_FRAMEBUFFER:

            //  Compute the framebuffer tiling/blocking address for the first corner texel of the block.
            startBlockAddress = pixelMapper.computeAddress(startBlockTexelX, startBlockTexelY);
            break;            
        
        default:
            CG_ASSERT("Unknown destination tile blocking format.");
            break;
    }

//printf("%s::Blitter => Block texels: (%d,%d)->(%d,%d). Start address: %d\n", transactionInterface.getBoxName().c_str(), startBlockTexelX, startBlockTexelY, endBlockTexelX, endBlockTexelY, startBlockAddress);

    //  Fill the texture block address information.  */
    textureBlock->address = blitDestinationAddress + startBlockAddress;
    textureBlock->block = address2block(textureBlock->address, bytesPerBlock);
    
    //  Iterate over all the texels in the texture block, masking all the texels inside the covered texture region. 
    //  At the same time, the corresponding color buffer data blocks are stored in a set, and the correspondece/mapping between
    //  pixels and texels is stored.  
    //
    set<U32> colorBufferBlocks;
    
    map<U32, list<pair<U32,U32> > > correspondenceMap;
    
    for (int j = startBlockTexelY; j <= endBlockTexelY; j++)
    {        
        for (int i = startBlockTexelX; i <= endBlockTexelX; i++)
        {
            U32 texelAddress;

            switch(blitDestinationBlocking)
            {
                case GPU_TXBLOCK_TEXTURE:

                    //  Compute the morton address for the texel.
                    texelAddress = texel2MortonAddress(i, j, mortonBlockDim, mortonSBlockDim, blitTextureWidth2) * bytesTexel;
                    break;

                case GPU_TXBLOCK_FRAMEBUFFER:

                    //  Compute the framebuffer tiling/blocking address for the first corner texel of the block */
                    texelAddress = pixelMapper.computeAddress(i, j);                    
                    break;

                default:                    
                    CG_ASSERT("Unknown destination tile blocking format.");
                    break;
            }
              
            GPU_ASSERT(
                U32 maxTexelAddress = bytesTexel * ((U32)GPU_POWER2OF((F64)blitTextureWidth2)) * ((U32)GPU_POWER2OF((F64)blitTextureWidth2));

                if (texelAddress > maxTexelAddress)
                {
                    printf("%s::Blitter => Texels: (%d,%d) inside the texture region. Texel address: %d \n", transactionInterface.getBoxName().c_str(), i, j, texelAddress);
                    CG_ASSERT("Generated texture block address not in texture memory region.");
                }
            )
              
            // Ask if texel inside the texture region.
            if ( blitXOffset <= i && i < blitXOffset + blitWidth && blitYOffset <= j && j < blitYOffset + blitHeight)
            {
                int k;

                //  Compute the texture block offset of the texel.
                U32 texelBlockOffset = texelAddress - startBlockAddress;
                  
// printf("%s::Blitter => Texels: (%d,%d) inside the texture region. Offset: %d \n", transactionInterface.getBoxName().c_str(), i, j, texelBlockOffset);

if ((texelBlockOffset + bytesTexel) > (bytesPerBlock))
{
U32 a = 0;
}

                //  Set the mask for all the texel bytes.
                for (k = 0; k < bytesTexel; k++)
                    mask[texelBlockOffset + k] =  true;
                  
                //  Compute the color buffer data blocks to request.
                U32 pixelX = (i - blitXOffset) + blitIniX;
                U32 pixelY = (j - blitYOffset) + blitIniY;

                //  Compute the address of the corresponding texel in the source sourface.
                U32 colorBufferAddress = pixelMapper.computeAddress(pixelX, pixelY);

                //  Compute the block and offset in the source surface.
                U32 colorBufferBlock = colorBufferAddress >> GPUMath::calculateShift(colorBufferBlockSize);
                U32 colorBufferBlockOffset = colorBufferAddress % colorBufferBlockSize;

                //  Insert color buffer block in the set (the set avoids repetitions).
                colorBufferBlocks.insert(colorBufferBlock);

                //  Insert the correspondences in the map (for all the texel bytes).
                for (k = 0; k < bytesTexel; k++)
                   correspondenceMap[colorBufferBlock].push_back(make_pair(colorBufferBlockOffset+k, texelBlockOffset+k));
             }
        }
    }
    
    //  Store the mask.
    textureBlock->writeMask = mask;
    
    //  Allocate space for the final contents of the texture block. This could be done somewhere else, 
    //  for instance, before performing swizzling.  
    //
    textureBlock->contents = new U08[bytesPerBlock];

    //  Initialize the color buffer blocks structures.
    for (set<U32>::const_iterator iter = colorBufferBlocks.begin(); iter != colorBufferBlocks.end(); iter++)
    {
        ColorBufferBlock* cbBlock = new ColorBufferBlock;
        
        cbBlock->address = (*iter) * colorBufferBlockSize;
        cbBlock->block = (*iter);
        cbBlock->state = colorBufferState[cbBlock->block];
        
        U32 blockSize;
        
        //  Allocate space for the compressed color buffer block accordling to the block state.
        switch(cbBlock->state.state)
        {
            case ROPBlockState::CLEAR: blockSize = 0; break;
            case ROPBlockState::UNCOMPRESSED: blockSize = colorBufferBlockSize; break;
            case ROPBlockState::COMPRESSED: 
                blockSize = bmoColorCompressor::getInstance()
                                                        .getLevelBlockSize(cbBlock->state.comprLevel);
                break;
            default:
                CG_ASSERT("Undefined compression method.");
                break;
        }

        if (blockSize > 0)
            cbBlock->compressedContents = new U08[blockSize];
        else
            cbBlock->compressedContents = NULL;
        
        //  Fill the correspoding information to request the 
        //  corresponding color buffer data in the next stages.  
        //
        for (int i = 0; i < colorBufferBlockSize; i++)
            cbBlock->swzMask.push_back(-1);
            
        list<pair<U32,U32> >::const_iterator iter2 = correspondenceMap[cbBlock->block].begin();
        
        while ( iter2 != correspondenceMap[cbBlock->block].end() )
        {
            cbBlock->swzMask[iter2->first] = iter2->second;
            iter2++;
        }

        //  Push the block into the texture block information.
        textureBlock->colorBufferBlocks.push_back(cbBlock);
    }
}

//  Computes the total number of transactions needed to request all the color buffer blocks data of the texture block.  
U32 Blitter::computeTotalTransactionsToRequest(TextureBlock* textureBlock)
{
    vector<ColorBufferBlock*>::const_iterator iter = textureBlock->colorBufferBlocks.begin();
    
    U32 totalTransactions = 0;
    
    while ( iter != textureBlock->colorBufferBlocks.end())
    {
        const ROPBlockState& block = (*iter)->state;
        
        switch(block.state)
        {
            case ROPBlockState::CLEAR: 

                //  No request needed. 
                break;
                        
            case ROPBlockState::UNCOMPRESSED: 

                totalTransactions += (U32) ceil((F32) colorBufferBlockSize / (F32) MAX_TRANSACTION_SIZE );
                break;
               
            case ROPBlockState::COMPRESSED: 

                totalTransactions += (U32) ceil((F32) bmoColorCompressor::getInstance()
                       .getLevelBlockSize(block.comprLevel) / (F32) MAX_TRANSACTION_SIZE );
                break;

            default:
                CG_ASSERT("Color buffer block state not expected");
        }
             
        iter++;
    }

    return totalTransactions;
}


//  Sends as many memory read requests as allowed in one cycle for the color buffer blocks in the texture block.  
bool Blitter::requestColorBufferBlocks(U64 cycle, TextureBlock* textureBlock, set<U32>& generatedTickets)
{
    vector<ColorBufferBlock*>::const_iterator iter = textureBlock->colorBufferBlocks.begin();
    
    bool transactionSent = false;
    
    bool blockFinished = true;
    
    while ( iter != textureBlock->colorBufferBlocks.end() && blockFinished )
    {
        const ROPBlockState& block = (*iter)->state;
        
        switch(block.state)
        {
            case ROPBlockState::CLEAR: 

                //  No request needed. 
                break;
                        
            case ROPBlockState::UNCOMPRESSED: 

                blockFinished = requestColorBlock(cycle,(*iter), colorBufferBlockSize, generatedTickets);
                
                //blockFinished = requestUncompressedBlock(cycle,(*iter), generatedTickets);
                
                break;
               
            case ROPBlockState::COMPRESSED: 

                blockFinished = requestColorBlock(cycle, (*iter), 
                                                  bmoColorCompressor::getInstance().getLevelBlockSize(block.comprLevel),
                                                  generatedTickets);
                                                  
                //switch (block.comprLevel) {
                //case 0: blockFinished = requestCompressedBestBlock(cycle,(*iter), generatedTickets); break;
                //case 1: blockFinished = requestCompressedNormalBlock(cycle,(*iter), generatedTickets); break;
                //case 2: // TODO: Implementar requestCompressedBlock para level 2
                    //break;
                //default: 
                //    CG_ASSERT("Compression level not supported !!!");
                //}
                
                break;
                
            default:
                panic("Blitter::TextureBlock","print","Color buffer block state not expected");
        }
             
        iter++;
    }

    return blockFinished;
}

bool Blitter::requestColorBlock(U64 cycle, ColorBufferBlock *bInfo, U32 blockRequestSize, set<U32>& generatedTickets)
{
    if (bInfo->requested >= blockRequestSize)
        return true;

    //  Compute transaction size.  
    U32 size = GPU_MIN(MAX_TRANSACTION_SIZE, blockRequestSize - bInfo->requested);
    
    U32 blockOffset = bInfo->requested;
    
    U32 requestAddress = bInfo->address + blockOffset + backBufferAddress;
    
    if (transactionInterface.sendReadTransaction(cycle, size, requestAddress, bInfo->compressedContents + blockOffset , nextTicket))
    {
        GPU_DEBUG(
              printf("%s::Blitter %lld => Requesting %d bytes at %x for color buffer block %d (ticket %d).\n",
                     transactionInterface.getBoxName().c_str(), cycle, size, requestAddress,
                     bInfo->block, nextTicket);
        )
                    
        bInfo->tickets.push_back(nextTicket);
        generatedTickets.insert(nextTicket);
        
        nextTicket++;
                    
        //  Update requested block bytes counter.
        bInfo->requested += size;
    }
    
    return false;
};

bool Blitter::requestUncompressedBlock(U64 cycle, ColorBufferBlock *bInfo, set<U32>& generatedTickets)
{
    if (bInfo->requested >= colorBufferBlockSize)
        return true;

    //  Compute transaction size.  
    U32 size = GPU_MIN(MAX_TRANSACTION_SIZE, colorBufferBlockSize);
    
    U32 blockOffset = bInfo->requested;
    
    if (transactionInterface.sendReadTransaction(cycle, size, bInfo->address + blockOffset, bInfo->compressedContents + blockOffset , nextTicket))
    {
        GPU_DEBUG(
              printf("%s::Blitter %lld => Requesting %d bytes at %x for color buffer block %d (ticket %d).\n",
                     transactionInterface.getBoxName().c_str(), cycle, size, bInfo->address + blockOffset,
                     bInfo->block, nextTicket);
        )
                    
        bInfo->tickets.push_back(nextTicket);
        generatedTickets.insert(nextTicket);
        
        nextTicket++;
                    
        //  Update requested block bytes counter.
        bInfo->requested += size;
    }
    
    return false;
};

bool Blitter::requestCompressedNormalBlock(U64 cycle, ColorBufferBlock *bInfo, set<U32>& generatedTickets)
{
    if (bInfo->requested >= ColorCacheV2::COMPRESSED_BLOCK_SIZE_NORMAL)
        return true;

    //  Compute transaction size.  
    U32 size = GPU_MIN(MAX_TRANSACTION_SIZE, ColorCacheV2::COMPRESSED_BLOCK_SIZE_NORMAL);
    
    U32 blockOffset = bInfo->requested;
    
    if (transactionInterface.sendReadTransaction(cycle, size, bInfo->address + blockOffset, bInfo->compressedContents + blockOffset , nextTicket))
    {
        GPU_DEBUG(
              printf("%s::Blitter %lld => Requesting %d bytes at %x for color buffer block %d (ticket %d).\n",
                     transactionInterface.getBoxName().c_str(), cycle, size, bInfo->address + blockOffset,
                     bInfo->block, nextTicket);
        )
                    
        bInfo->tickets.push_back(nextTicket);
        generatedTickets.insert(nextTicket);
        
        nextTicket++;
                    
        //  Update requested block bytes counter.
        bInfo->requested += size;
    }
    
    return false;
};

bool Blitter::requestCompressedBestBlock(U64 cycle, ColorBufferBlock *bInfo, set<U32>& generatedTickets)
{
    if (bInfo->requested >= ColorCacheV2::COMPRESSED_BLOCK_SIZE_BEST)
        return true;

    //  Compute transaction size.  
    U32 size = GPU_MIN(MAX_TRANSACTION_SIZE, ColorCacheV2::COMPRESSED_BLOCK_SIZE_BEST);
    
    U32 blockOffset = bInfo->requested;
    
    if (transactionInterface.sendReadTransaction(cycle, size, bInfo->address + blockOffset, bInfo->compressedContents + blockOffset , nextTicket))
    {
        GPU_DEBUG(
              printf("%s::Blitter %lld => Requesting %d bytes at %x for color buffer block %d (ticket %d).\n",
                     transactionInterface.getBoxName().c_str(), cycle, size, bInfo->address + blockOffset,
                     bInfo->block, nextTicket);
        )
                    
        bInfo->tickets.push_back(nextTicket);
        generatedTickets.insert(nextTicket);
        
        nextTicket++;
                    
        //  Update requested block bytes counter.
        bInfo->requested += size;
    }
    
    return false;
};

//  Decompresses the color buffer blocks data of the texture block.  
void Blitter::decompressColorBufferBlocks(TextureBlock* textureBlock)
{
    std::vector<ColorBufferBlock*>::iterator iter = textureBlock->colorBufferBlocks.begin();
          
    while (iter != textureBlock->colorBufferBlocks.end())
    {
        (*iter)->contents = new U08[colorBufferBlockSize];

//printf("Blitter::decompressColorBufferBlocks => block %d address %x size %d | state = %d | level = %d\n",
//    (*iter)->block , (*iter)->address, (*iter)->requested, (*iter)->state.getState(), (*iter)->state.getComprLevel());
//printf("Compressed data (%d bytes) : \n    ", (*iter)->requested);
//for(U32 b = 0; b < (*iter)->requested; b++)
//printf("%02x ", (*iter)->compressedContents[b]);
//printf("\n");
        
        decompressBlock((*iter)->contents, (*iter)->compressedContents, colorBufferBlockSize, (*iter)->state, clearColor);
        
        //  Check if saving the blit source data to a file is enabled.
        if (saveBlitSource)
        {
            //  Copy decompressed data to color buffer array.
            for(U32 b = 0; b < colorBufferBlockSize; b += 4)
                *((U32 *) &colorBuffer[(*iter)->address + b]) = *((U32 *) &(*iter)->contents[b]);        
        }
        
//printf("(1) Uncompressed data (%d bytes) : \n    ", colorBufferBlockSize);
//for(U32 b = 0; b < colorBufferBlockSize; b++)
//printf("%02x ", (*iter)->contents[b]);
//printf("\n");

//printf("(2) Uncompressed data (%d bytes) : \n    ", colorBufferBlockSize);
//for(U32 b = 0; b < colorBufferBlockSize; b++)
//printf("%02x ", colorBuffer[(*iter)->address + b]);
//printf("\n");
        
        iter++;
    }
}

//  Performs swizzling of the texture block contents.  
void Blitter::swizzleTextureBlock(TextureBlock* textureBlock)
{
    std::vector<ColorBufferBlock*>::const_iterator iter = textureBlock->colorBufferBlocks.begin();
    
    U32 position = 0;
    
    //  Reset for new swizzling operation.  
    swzBuffer.reset();
    
    //   Write each color buffer block data and swizzling mask in one swizzling Buffer position.   
    while ( iter != textureBlock->colorBufferBlocks.end() )
    {
        swzBuffer.writeInputData((*iter)->contents, position);
        swzBuffer.setSwzMask((*iter)->swzMask, position);
        position++;
        iter++;
    }

    //  Perform swizzling.  
    swzBuffer.swizzle();
    
    //  Collect swizzled data.  
    swzBuffer.readOutputData(textureBlock->contents, 0);
}

#define GAMMA(x) F32(GPU_POWER(F64(x), F64(1.0f / 2.2f)))
#define LINEAR(x) F32(GPU_POWER(F64(x), F64(2.2f)))
//#define GAMMA(x) (x)

//  Dump source data for the blit operation to a file.
void Blitter::dumpBlitSource(U32 frameCounter, U32 blitCounter)
{

    //  Check if saving the blit source data to a ppm file is enabled.
    if (!saveBlitSource)
        return;
        
    FILE *fout;
    char filename[30];
    U32 address;
    S32 x,y;

    //  Create current frame filename.  
    sprintf(filename, "blitop-f%04d-%04d.ppm", frameCounter, blitCounter);

    //  Open/Create the file for the current frame.  
    fout = fopen(filename, "wb");

    //  Check if the file was correctly created.  
    CG_ASSERT_COND(!(fout == NULL), "Error creating frame color output file.");
    //  Write file header.  

    //  Write magic number.  
    fprintf(fout, "P6\n");

    //  Write frame size.  
    fprintf(fout, "%d %d\n", blitWidth, blitHeight);

    //  Write color component maximum value.  
    fprintf(fout, "255\n");

    U08 red;
    U08 green;
    U08 blue;

    //  Check if multisampling is enabled.
    if (!multisampling)
    {
        // Do this for the whole picture now 
        for(y = S32(blitIniY + blitHeight) - 1; y >= S32(blitIniY); y--)
        {
            for(x = S32(blitIniX); x < S32(blitIniX + blitWidth); x++)
            {
                //*  NOTE THERE SURE ARE FASTER WAYS TO DO THIS ...  *
                //  Calculate address from pixel position.  
                //address = GPUMath::pixel2Memory(x, y, startX, startY, width, height, hRes, vRes,
                //    STAMP_WIDTH, STAMP_HEIGHT, genW, genH, scanW, scanH, overW, overH, 1, bytesPixel);
                address = pixelMapper.computeAddress(x, y);

                //  Convert data from the color color buffer to 8-bit PPM format.
                switch(colorBufferFormat)
                {
                    case GPU_RGBA8888:
                        red   = colorBuffer[address];
                        green = colorBuffer[address + 1];
                        blue  = colorBuffer[address + 2];
                        break;
                        
                    case GPU_RG16F:
                        red   = U08(GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 0])) * 255.0f);
                        green = U08(GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 2])) * 255.0f);
                        blue  = 0;
                        break;

                    case GPU_R32F:
                        red   = U08(*((F32 *) &colorBuffer[address]) * 255.0f);
                        green = 0;
                        blue  = 0;
                        break;
                                                
                    case GPU_RGBA16F:
                        red   = U08(GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 0])) * 255.0f);
                        green = U08(GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 2])) * 255.0f);
                        blue  = U08(GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 4])) * 255.0f);
                        break;
                }
                
                //  Write color to the file.
                fputc((char) red, fout);
                fputc((char) green, fout);
                fputc((char) blue, fout);
            }
        }
    }
    else
    {
        Vec4FP32 sampleColors[MAX_MSAA_SAMPLES];
        Vec4FP32 resolvedColor;
        Vec4FP32 referenceColor;
        Vec4FP32 currentColor;

        //  Resolve the whole framebuffer.
        for(y = (blitIniY + blitHeight) - 1; y >= blitIniY; y--)
        {
            for(x = blitIniX; x < S32(blitWidth); x++)
            {
                U32 sampleX;
                U32 sampleY;
                
                sampleX = x - GPU_MOD(x, STAMP_WIDTH);
                sampleY = y - GPU_MOD(y, STAMP_HEIGHT);
                //U32 msaaBytesPixel = bytesPixel * msaaSamples;
                
                //  Calculate address for the first sample in the stamp.
                //address = GPUMath::pixel2Memory(sampleX, sampleY, startX, startY, width, height,
                //    hRes, vRes, STAMP_WIDTH, STAMP_HEIGHT, genW, genH,
                //    scanW, scanH, overW, overH, msaaSamples, bytesPixel);
                address = pixelMapper.computeAddress(sampleX, sampleY);
                    
                //  Calculate address for the first sample in the pixel.                    
                address = address + msaaSamples * (GPU_MOD(y, STAMP_HEIGHT) * STAMP_WIDTH + GPU_MOD(x, STAMP_WIDTH)) * bytesPixel;
                
                //  Zero resolved color.
                resolvedColor[0] = 0.0f;
                resolvedColor[1] = 0.0f;
                resolvedColor[2] = 0.0f;
                
                //  Convert data from the color color buffer 32 bit fp internal format.
                switch(colorBufferFormat)
                {
                    case GPU_RGBA8888:
                        referenceColor[0] = F32(colorBuffer[address + 0]) / 255.0f;
                        referenceColor[1] = F32(colorBuffer[address + 1]) / 255.0f;
                        referenceColor[2] = F32(colorBuffer[address + 2]) / 255.0f;
                        break;
                        
                    case GPU_RG16F:
                        referenceColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 0]));
                        referenceColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 2]));
                        referenceColor[2] = 0.0f;
                        break;
                        
                    case GPU_R32F:
                        referenceColor[0] = *((F32 *) &colorBuffer[address]);
                        referenceColor[1] = 0.0f;
                        referenceColor[2] = 0.0f;
                        break;                        

                    case GPU_RGBA16F:
                        referenceColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 0]));
                        referenceColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 2]));
                        referenceColor[2] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + 4]));
                        break;
                }
                
                bool fullCoverage = true;

                //  Accumulate the color for all the samples in the pixel
                for(U32 i = 0; i < msaaSamples; i++)
                {
                    //  Convert data from the color color buffer 32 bit fp internal format.
                    switch(colorBufferFormat)
                    {
                        case GPU_RGBA8888:
                            currentColor[0] = F32(colorBuffer[address + i * 4 + 0]) / 255.0f;
                            currentColor[1] = F32(colorBuffer[address + i * 4 + 1]) / 255.0f;
                            currentColor[2] = F32(colorBuffer[address + i * 4 + 2]) / 255.0f;
                            break;
                            
                        case GPU_RG16F:
                            currentColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + i * 4 + 0]));
                            currentColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + i * 4 + 2]));
                            currentColor[2] = 0.0f;
                            break;
                            
                        case GPU_R32F:
                            currentColor[0] = *((F32 *) &colorBuffer[address + i * 4]);
                            currentColor[1] = 0.0f;
                            currentColor[2] = 0.0f;
                            break;                        
                                    
                        case GPU_RGBA16F:
                            currentColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + i * 8 + 0]));
                            currentColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + i * 8 + 2]));
                            currentColor[2] = GPUMath::convertFP16ToFP32(*((U16 *) &colorBuffer[address + i * 8 + 4]));
                            break;
                    }

                    sampleColors[i][0] = currentColor[0];
                    sampleColors[i][1] = currentColor[1];
                    sampleColors[i][2] = currentColor[2];
                    
                    resolvedColor[0] += LINEAR(currentColor[0]);
                    resolvedColor[1] += LINEAR(currentColor[1]);
                    resolvedColor[2] += LINEAR(currentColor[2]);
                    
                    fullCoverage = fullCoverage && (referenceColor[0] == currentColor[0])
                                                && (referenceColor[1] == currentColor[1])
                                                && (referenceColor[2] == currentColor[2]);
                }
                
                //  Check if there is a single sample for the pixel
                if (fullCoverage)
                {
                    //  Convert from RGBA32F (internal format) to RGBA8 (PPM format)
                    red   = U08(referenceColor[0] * 255.0f);
                    green = U08(referenceColor[1] * 255.0f);
                    blue  = U08(referenceColor[2] * 255.0f);
                    
                    //  Write pixel color into the output file.
                    fputc(char(red)  , fout);
                    fputc(char(green), fout);
                    fputc(char(blue) , fout);
                }
                else
                {
                    //  Resolve color as the average of all the sample colors.
                    resolvedColor[0] = GAMMA(resolvedColor[0] / F32(msaaSamples));
                    resolvedColor[1] = GAMMA(resolvedColor[1] / F32(msaaSamples));
                    resolvedColor[2] = GAMMA(resolvedColor[2] / F32(msaaSamples));

                    //  Write resolved color into the output file.
                    fputc(char(resolvedColor[0] * 255.0f), fout);
                    fputc(char(resolvedColor[1] * 255.0f), fout);
                    fputc(char(resolvedColor[2] * 255.0f), fout);
                }
            }                    
        }                
    }
    
    fclose(fout);

    //  Remove the buffer where the data has been stored.
    delete [] colorBuffer;
}
