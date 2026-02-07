/**************************************************************************
 *
 * GPU Driver implementation.
 *
 */

#include "HAL.h"
#include "GPUType.h"
#include "ShaderOptimization.h"
#include "PixelMapper.h"

#include "GlobalProfiler.h"

#include <cstdio>
#include <cmath>
#include <iostream>
#include <sstream>
#include <cstring>

using namespace std;
using namespace cg1gpu;

/**
 * Define these flags to enable debug of cgoMetaStreams
 *
 * - The 1st flag enables the dump of all transactions returned by nxtMetaStream() method
 *   in "Driver_nextcgoMetaStreams.txt" file
 *
 * - The 2nd flag enables the dump of all transactions received by the internal method sendcgoMetaStream
 *   in "Driver_SentcgoMetaStreams.txt" file.
 *
 * - The 3rd flag disables the driver WriteBuffer cache -> Set its operation mode to WriteBuffer::Inmediate
 *
 * - The 4th flag disables the dump of the transaction sequence number
 */
//#define ENABLE_DUMP_NEXTcgoMetaStream_TO_FILE
//#define ENABLE_DUMP_SENDMetaStreamTRANSTION_TO_FILE
//#define DISABLE_WRITEBUFFER_CACHE

#define DISABLE_DUMP_TRANSACTION_COUNTERS

#ifdef ENABLE_DUMP_NEXTcgoMetaStream_TO_FILE
    #define DUMP_NEXTcgoMetaStream_TO_FILE(code) { code }
#else
    #define DUMP_NEXTcgoMetaStream_TO_FILE(code) {}
#endif

#ifdef ENABLE_DUMP_SENDMetaStreamTRANSTION_TO_FILE
    #define DUMP_SENDMetaStreamTRANSTION_TO_FILE(code) { code }
#else
    #define DUMP_SENDMetaStreamTRANSTION_TO_FILE(code) {}
#endif

#ifdef DISABLE_DUMP_TRANSACTION_COUNTERS
    #define DUMP_TRANSACTION_COUNTERS(code) {}
#else
    #define DUMP_TRANSACTION_COUNTERS(code) { code }
#endif

const S32 HAL::outputAttrib[MAX_VERTEX_ATTRIBUTES] =
{
    0, // VS_VERTEX
    VS_NOOUTPUT, // VS_WEIGHT
    VS_NOOUTPUT, // VS_NORMAL
    1, // VS_COLOR
    2, // VS_SEC_COLOR
    3, // VS_FOG
    VS_NOOUTPUT, // VS_SEC_WEIGHT
    VS_NOOUTPUT, // VS_THIRD_WEIGHT
    4, // VS_TEXTURE_0
    5, // VS_TEXTURE_1
    6, // VS_TEXTURE_2
    7, // VS_TEXTURE_3
    8, // VS_TEXTURE_4
    9, // VS_TEXTURE_5
    10, // VS_TEXTURE_6
    11 // VS_TEXTURE_7
};


#define _DRIVER_STATISTICS

#define MSG(msg) printf( "%s\n", msg )

#define INC_MOD(a) a = GPU_MOD(a + 1, META_STREAM_BUFFER_SIZE)

#define CHECK_BIT(bits,bit) ((((1<<(bit))&(bits))!=0)?true:false)

#define CHECK_MEMORY_ACCESS(md, offset, dataSize)\
((((offset)+(dataSize))>((md)->lastAddress-(md)->firstAddress+1))?false:true)

#define UPDATE_HIGH_ADDRESS_WRITTEN(memDesc,offset,dataSize)\
if ( (memDesc)->highAddressWritten < (memDesc)->firstAddress+(offset)+(dataSize)-1 ) {\
    (memDesc)->highAddressWritten = (memDesc)->firstAddress+(offset)+(dataSize)-1;\
}

U08 HAL::_mortonTable[256];
#define max(a,b)\
    (a>b?a:b)

//const S32 HAL::outputAttrib[MAX_VERTEX_ATTRIBUTES];

/**
 * This version:
 *    - controls MetaStreamBuffer underrun ( informs when cgoMetaStream is lost )
 *    - controls access to MetaStreamBuffer
 *    - controls 'in' var for MetaStreamBuffer access
 *    - controls MetaStreamcount
 */
bool HAL::_sendcgoMetaStream( cgoMetaStream* agpt )
{
    // Remember to select register buffer write policy to RegisterBuffer::Inmediate to
    // see _sendcgoMetaStreams instantly when writing registers
    // NEW: This can be done defining DISABLE_WRITEBUFFER_CACHE
    DUMP_SENDMetaStreamTRANSTION_TO_FILE(
        static ofstream f;
        static U32 tCounter = 0;
        if ( !f.is_open() )
            f.open("Driver_SentcgoMetaStreams.txt");

        DUMP_TRANSACTION_COUNTERS( f << tCounter++ << ": ";)

        agpt->dump(f);
        f.flush();
    )

    #ifdef _DRIVER_STATISTICS
        metaStreamGenerated++;
    #endif

    if ( metaStreamCount >= META_STREAM_BUFFER_SIZE )
        CG_ASSERT("Buffer overflow in MetaStreamBuffer");

    metaStreamBuffer[in] = agpt;
    INC_MOD(in);
    metaStreamCount++;

    return true;
}


/**
 * Singleton HAL object
 */
HAL* HAL::driver = NULL;


HAL::HAL() : metaStreamCount(0), in(0), out(0), nextMemId(1), setGPUParametersCalled(false),
setResolutionCalled(false), hRes(0), vRes(0), lastBlock(0),
gpuAllocBlocks(0), systemAllocBlocks(0),
// statistics
metaStreamGenerated(0), memoryAllocations(0), memoryDeallocations(0), mdSearches(0),
addressSearches(0), memPreloads(0), memWrites(0), memPreloadBytes(0), memWriteBytes(0), ctx(0), preloadMemory(false), 
#ifdef DISABLE_WRITEBUFFER_CACHE
    registerWriteBuffer(this, RegisterWriteBuffer::Inmediate),
#else
    registerWriteBuffer(this, RegisterWriteBuffer::WaitUntilFlush),
#endif
//vshSched(this, 512, ShaderProgramSched::VERTEX),
//fshSched(this, 512, ShaderProgramSched::FRAGMENT)
shSched(this, 4096), frame(0), batch(0)
{
    //GPU_DEBUG( cout << "Creating Driver object" << endl; )

    /*
     * @note: Code moved to setGPUParameters
     * map = new U08[physicalCardMemory / 4]; // 1 byte per 4 Kbytes
     * memset( map, 0, physicalCardMemory / 4 ); // all memory available
     */
    metaStreamBuffer = new cgoMetaStream*[META_STREAM_BUFFER_SIZE];

    registerWriteBuffer.initAllRegisterStatus();

    memoryDescriptors.clear();

    MortonTableBuilder();
}

HAL::~HAL()
{
    //GPU_DEBUG( cout << "Destroying Driver object" << endl; )
    delete[] metaStreamBuffer;
    delete[] gpuMap;
    delete[] systemMap;

    map<U32, _MemoryDescriptor*>::iterator it;
    it = memoryDescriptors.begin();
    while (it != memoryDescriptors.end())
    {
        delete it->second;
        it++;
    }

    memoryDescriptors.clear();

    //_MemoryDescriptor* md;
    //for ( md = mdList; md != NULL; md = md->next )
    //{
    //    delete md;
    //}
}


U32 HAL::getFetchRate() const
{
    CG_ASSERT_COND(setGPUParametersCalled, "setGPUParameters was not called");
    return fetchRate;
}

void HAL::setGPUParameters(U32 gpuMemSz, U32 sysMemSz, U32 blocksz_, U32 sblocksz_, U32 scanWidth_,
    U32 scanHeight_, U32 overScanWidth_, U32 overScanHeight_,
    bool doubleBuffer_, bool forceMSAA_, U32 msaaSamples, bool forceFP16CB_, U32 fetchRate_,
    bool memoryControllerV2_, bool secondInterleavingEnabled_,
    bool convertToLDA, bool convertToSOA, bool enableTransformations, bool microTrisAsFrags
    )
{
    if ( !setGPUParametersCalled )
    {
        gpuMemory = gpuMemSz;
        systemMemory = sysMemSz;
        blocksz = blocksz_;
        sblocksz = sblocksz_;
        scanWidth = scanWidth_;
        scanHeight = scanHeight_;
        overScanWidth = overScanWidth_;
        overScanHeight = overScanHeight_;
        doubleBuffer = doubleBuffer_;
        fetchRate = fetchRate_;
        forceMSAA = forceMSAA_;
        forcedMSAASamples = msaaSamples;
        forceFP16ColorBuffer = forceFP16CB_;
        secondInterleavingEnabled = memoryControllerV2_ && secondInterleavingEnabled_;
        convertShaderProgramToLDA = convertToLDA;
        convertShaderProgramToSOA = convertToSOA;
        enableShaderProgramTransformations = enableTransformations;
        microTrianglesAsFragments = microTrisAsFrags;

        //  Allocate GPU memory map.  
        gpuMap = new U08[gpuMemory / BLOCK_SIZE]; // 1 byte per 4 Kbytes
        memset( gpuMap, 0, gpuMemory / BLOCK_SIZE ); // all memory available

        //  Allocate system memory map.  
        systemMap = new U08[systemMemory / BLOCK_SIZE]; // 1 byte per 4 Kbytes
        memset( systemMap, 0, systemMemory / BLOCK_SIZE ); // all memory available

        setGPUParametersCalled = true;
    }
    else
        CG_ASSERT("This method must be called once");

}


void HAL::getGPUParameters(U32& gpuMemSz, U32 &sysMemSz, U32& blocksz_, U32& sblocksz_,
    U32& scanWidth_, U32& scanHeight_, U32& overScanWidth_, U32& overScanHeight_,
    bool& doubleBuffer_, U32& fetchRate_) const
{
    if ( !setGPUParametersCalled )
        CG_ASSERT("setGPUParameters was not called");

    gpuMemSz = gpuMemory;
    sysMemSz = systemMemory;
    blocksz_ = blocksz;
    sblocksz_ = sblocksz;
    scanWidth_ = scanWidth;
    scanHeight_ = scanHeight;
    overScanWidth_ = overScanWidth;
    overScanHeight_ = overScanHeight;
    doubleBuffer_ = doubleBuffer;
    fetchRate_ = fetchRate;
}

//  Get the texture tiling parameters.  
void HAL::getTextureTilingParameters(U32 &blockSz, U32 &sBlockSz) const
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")

    blockSz = blocksz;
    sBlockSz = sblocksz;

    GLOBAL_PROFILER_EXIT_REGION()

}


void HAL::getFrameBufferParameters(bool &multisampling, U32 &samples, bool &fp16color)
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")

    multisampling = forceMSAA;
    samples = forcedMSAASamples;
    fp16color = forceFP16ColorBuffer;

    GLOBAL_PROFILER_EXIT_REGION()
}

void HAL::initBuffers(U32* mdCWFront_, U32* mdCWBack_, U32* mdZS_)
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")

    if ( !isResolutionDefined() )
        CG_ASSERT("Resolution have not been set");

    if ( !setGPUParametersCalled )
        CG_ASSERT("setGPUParameters must be called before calling this method");

    U32 cwBufSz;
    U32 zsBufSz;

    U32 bytesPerPixelCW;
    U32 bytesPerPixelZST;

    if (!forceFP16ColorBuffer)
        bytesPerPixelCW = 4;
    else
        bytesPerPixelCW = 8;

    bytesPerPixelZST = 4;

    //  Initialize the pixel mapper used to compute the size of the render buffer.
    //
    //  NOTE:  The render buffer size shouldn't be directly affected by the dimensions
    //  of the stamp tile or the generation tile.  However the generation tile size must
    //  be at least 4x4 due to limitations of the current implementation.  For this reason
    //  the stamp tile is force to 1x1 and generation tile to 4x4 even if those aren't
    //  the real values used.  The size of the scan tile is adjusted to match the selected
    //  sizes of the stamp and generation tiles.
    //
    //  WARNING: Check correctness of the current implementation.
    //

    PixelMapper cwPixelMapper;
    PixelMapper zstPixelMapper;
    U32 samples = forceMSAA ? forcedMSAASamples : 1;
    cwPixelMapper.setupDisplay(hRes, vRes, 1, 1, 4, 4, scanWidth / 4, scanHeight / 4, overScanWidth, overScanHeight, samples, bytesPerPixelCW);
    zstPixelMapper.setupDisplay(hRes, vRes, 1, 1, 4, 4, scanWidth / 4, scanHeight / 4, overScanWidth, overScanHeight, samples, bytesPerPixelZST);

    //  Check if forced multisampling is enabled.
    if (!forceMSAA)
    {
        cwBufSz = cwPixelMapper.computeFrameBufferSize();
        zsBufSz = zstPixelMapper.computeFrameBufferSize();
    }
    else
    {
        cout << "HAL ==> Forcing MSAA to " << forcedMSAASamples << "X" << endl;

        cwBufSz = cwPixelMapper.computeFrameBufferSize();
        zsBufSz = zstPixelMapper.computeFrameBufferSize();
    }



    U32 mdCWFront = obtainMemory(cwBufSz);

    U32 mdCWBack = mdCWFront;

    //  Check if double buffer is enabled. 
    if (doubleBuffer)
    {
        //  Get a color buffer for the back buffer.  
        mdCWBack = obtainMemory(cwBufSz);
    }

    U32 mdZS = obtainMemory(zsBufSz);

    writeGPUAddrRegister(GPU_FRONTBUFFER_ADDR, 0, mdCWFront);
    writeGPUAddrRegister(GPU_BACKBUFFER_ADDR, 0, mdCWBack);
    writeGPUAddrRegister(GPU_ZSTENCILBUFFER_ADDR, 0, mdZS);

    //  Check if forced multisampling is enabled.
    if (forceMSAA)
    {
        GPURegData data;

        //  Enable MSAA in the GPU.
        data.booleanVal = true;
        writeGPURegister(GPU_MULTISAMPLING, data);

        //  Set samples per pixel for MSAA.
        data.uintVal = forcedMSAASamples;
        writeGPURegister(GPU_MSAA_SAMPLES, data);
    }

    if (forceFP16ColorBuffer)
    {
        GPURegData data;

        cout << "HAL ==> Forcing Color Buffer format to RGBA16F." << endl;

        //  Change format of the color buffer to RGBA16F.
        data.txFormat = GPU_RGBA16F;
        writeGPURegister(GPU_COLOR_BUFFER_FORMAT, data);
    }

    // secondInterleavingEnabled == using memoryControllerV2 AND secondInterleaving is enabled
    if ( secondInterleavingEnabled )
    {
        // obtain start address of the second interleaving
        mdZS  = obtainMemory(1);
        // write the start address of the second interleaving
        writeGPUAddrRegister(GPU_MCV2_2ND_INTERLEAVING_START_ADDR, 0, mdZS);
        // deallocate the descriptor used to compute memory start address
        releaseMemory(mdZS);

        // MANDATORY FLUSH !!!
        // force a flush in the RegisterWriteBuffer object to update the register start
        // address of the second interleaving if not, preaload memory transactions
        // of previous skipped frames are, very likely, written in incorrect memory
        // addresses (mapping is based in the contents of the
        // GPU_MCV2_2ND_INTERLEAVING_START_ADDR)
        registerWriteBuffer.flush();
    }

    // if requested, assign memory descriptors
    if(mdCWFront_ != 0) *mdCWFront_ = mdCWFront;
    if(mdCWBack_ != 0) *mdCWBack_ = mdCWBack;
    if(mdZS_ != 0) *mdZS_ = mdZS;

    GLOBAL_PROFILER_EXIT_REGION()

}

//  Allocates space in GPU memory for a render buffer with the defined characteristics and returns a memory descriptor.
U32 HAL::createRenderBuffer(U32 width, U32 height, bool multisampling, U32 samples, TextureFormat format)
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")

    //  Determine the bytes per pixel based on the requested format.
    U32 bytesPerPixel;
    switch(format)
    {
        case GPU_RGBA8888:
        case GPU_RG16F:
        case GPU_DEPTH_COMPONENT24:
        case GPU_R32F:

            bytesPerPixel = 4;
            break;

        case GPU_RGBA16:
        case GPU_RGBA16F:

            bytesPerPixel = 8;
            break;

        default:

            CG_ASSERT("Unsupported render buffer format.");
            break;
    }

    //  Determine the samples per pixel based on the requested MSAA mode.
    U32 samplesPerPixel = multisampling ? samples : 1;

    //  Initialize the pixel mapper used to compute the size of the render buffer.
    //
    //  NOTE:  The render buffer size shouldn't be directly affected by the dimensions
    //  of the stamp tile or the generation tile.  However the generation tile size must
    //  be at least 4x4 due to limitations of the current implementation.  For this reason
    //  the stamp tile is force to 1x1 and generation tile to 4x4 even if those aren't
    //  the real values used.  The size of the scan tile is adjusted to match the selected
    //  sizes of the stamp and generation tiles.
    //
    //  WARNING: Check correctness of the current implementation.
    //

    PixelMapper pixelMapper;
    pixelMapper.setupDisplay(width, height, 1, 1, 4, 4, scanWidth / 4, scanHeight / 4, overScanWidth, overScanHeight,
                             samplesPerPixel, bytesPerPixel);

    //  Obtain the size of the render buffer in bytes.
    U32 renderBufferSize = pixelMapper.computeFrameBufferSize();

    //  Allocate memory and obtain a memory descriptor for the requested render buffer.
    U32 mdRenderBuffer = obtainMemory(renderBufferSize);

    GLOBAL_PROFILER_EXIT_REGION()

    //  Return the memory descriptor of the requested render buffer.
    return mdRenderBuffer;
}

void HAL::tileRenderBufferData(U08 *sourceData, U32 width, U32 height, bool multisampling, U32 samples,
                                     cg1gpu::TextureFormat format, bool invertColors, U08* &destData, U32 &renderBufferSize)
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")

    //  Determine the bytes per pixel based on the requested format.
    U32 bytesPerPixel;
    switch(format)
    {
        case GPU_RGBA8888:
        case GPU_RG16F:
        case GPU_DEPTH_COMPONENT24:
        case GPU_R32F:

            bytesPerPixel = 4;
            break;

        case GPU_RGBA16:
        case GPU_RGBA16F:

            bytesPerPixel = 8;
            break;

        default:

            CG_ASSERT("Unsupported render buffer format.");
            break;
    }

    if (multisampling)
        CG_ASSERT("MSAA render buffers not supported.");

    //  Determine the samples per pixel based on the requested MSAA mode.
    U32 samplesPerPixel = multisampling ? samples : 1;

    //  Initialize the pixel mapper used to compute the size of the render buffer.
    //
    //  NOTE:  The render buffer size shouldn't be directly affected by the dimensions
    //  of the stamp tile or the generation tile.  However the generation tile size must
    //  be at least 4x4 due to limitations of the current implementation.  For this reason
    //  the stamp tile is force to 1x1 and generation tile to 4x4 even if those aren't
    //  the real values used.  The size of the scan tile is adjusted to match the selected
    //  sizes of the stamp and generation tiles.
    //
    //  WARNING: Check correctness of the current implementation.
    //

    PixelMapper pixelMapper;
    pixelMapper.setupDisplay(width, height, 2, 2, 4, 4, scanWidth / 8, scanHeight / 8, overScanWidth, overScanHeight,
                             samplesPerPixel, bytesPerPixel);

    //  Obtain the size of the render buffer in bytes.
    renderBufferSize = pixelMapper.computeFrameBufferSize();

    //  Allocate the array for the tiled render buffer data.
    destData = new U08[renderBufferSize];

    for(U32 y = 0; y < height; y++)
    {
        for(U32 x = 0; x < width; x++)
        {
            // Compute address in the input surface.
            U32 sourceAddress = (y * width + x) * bytesPerPixel;

            // Compute address in the tiled render buffer.
            U32 destAddress = pixelMapper.computeAddress(x, y);

            switch(bytesPerPixel)
            {
                case 4:

                    if (invertColors)
                    {
                        destData[destAddress + 0] = sourceData[sourceAddress + 2];
                        destData[destAddress + 1] = sourceData[sourceAddress + 1];
                        destData[destAddress + 2] = sourceData[sourceAddress + 0];
                        destData[destAddress + 3] = sourceData[sourceAddress + 3];
                    }
                    else
                    {
                        *((U32 *) &destData[destAddress]) = *((U32 *) &sourceData[sourceAddress]);
                    }
                    break;

                case 8:

                    if (invertColors)
                    {
                        *((U16 *) &destData[destAddress + 0]) = *((U16 *) &sourceData[sourceAddress + 2]);
                        *((U16 *) &destData[destAddress + 1]) = *((U16 *) &sourceData[sourceAddress + 1]);
                        *((U16 *) &destData[destAddress + 2]) = *((U16 *) &sourceData[sourceAddress + 0]);
                        *((U16 *) &destData[destAddress + 3]) = *((U16 *) &sourceData[sourceAddress + 3]);
                    }
                    else
                    {                    
                        *((U64 *) &destData[destAddress]) = *((U64 *) &sourceData[sourceAddress]);
                    }
                    break;

                default:

                    CG_ASSERT("Unsupported pixel byte size.");
                    break;
            }
        }
    }

    GLOBAL_PROFILER_EXIT_REGION()
}


void HAL::writeGPURegister( GPURegister regId, GPURegData data, U32 md )
{
    registerWriteBuffer.writeRegister(regId, 0, data, md);
    //_sendcgoMetaStream(new cgoMetaStream( regId, 0, data ));
}

void HAL::writeGPURegister( GPURegister regId, U32 index, GPURegData data, U32 md )
{
    registerWriteBuffer.writeRegister(regId, index, data);
    //_sendcgoMetaStream(new cgoMetaStream( regId, index, data ));
}

void HAL::readGPURegister(cg1gpu::GPURegister regID, U32 index, cg1gpu::GPURegData &data)
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")

    registerWriteBuffer.readRegister(regID, index, data);

    GLOBAL_PROFILER_EXIT_REGION()
}

void HAL::readGPURegister(cg1gpu::GPURegister regID, cg1gpu::GPURegData &data)
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")

    registerWriteBuffer.readRegister(regID, 0, data);

    GLOBAL_PROFILER_EXIT_REGION()
}

void HAL::writeGPUAddrRegister( GPURegister regId, U32 index, U32 md, U32 offset )
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")
    _MemoryDescriptor* mdesc = _findMD(md);
    if ( mdesc == NULL )
    {
        cout << "regID = " << regId << "  | index = " << index << " | md = " << md << " | offset = " << offset << endl;
        CG_ASSERT("MemoryDescription does not exist");
    }
    GPURegData data;
    data.uintVal = mdesc->firstAddress + offset;

    if ( data.uintVal > mdesc->lastAddress )
        CG_ASSERT("offset out of bounds");

    registerWriteBuffer.writeRegister(regId, index, data, md);

    GLOBAL_PROFILER_EXIT_REGION()

    //_sendcgoMetaStream(new cgoMetaStream(regId, index, data));
}

HAL* HAL::getHAL()
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")
    if ( driver == NULL )
        driver = new HAL;
    GLOBAL_PROFILER_EXIT_REGION()
    return driver;
}


void HAL::sendCommand( GPUCommand com )
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")

    // This code moved before "if (preloadMemory)" to release pendentReleases
    if ( com == cg1gpu::GPU_DRAW )
    {
        //vshSched.dump();
        //fshSched.dump();

        //vshSched.dumpStatistics();
        //fshSched.dumpStatistics();

        vector<U32>::iterator it = pendentReleases.begin();
        for ( ; it != pendentReleases.end(); it ++ )
            _releaseMD(*it); // do real release
        pendentReleases.clear();
        batchCounter++; // next batch
#ifdef  DUMP_SYNC_REGISTERS_TO_GPU
        dumpRegisterStatus(frame, batch);
#endif
        batch++;
    }

    if (com == cg1gpu::GPU_SWAPBUFFERS)
    {
        frame++;
        batch = 0;
    }

    if ( preloadMemory )
    {
        GLOBAL_PROFILER_EXIT_REGION()
        return ;
    }

    // Before perform a command update the GPU register state machine
    registerWriteBuffer.flush();

    //  Send end of frame signal to the simulator before the swap command.
    if (com == cg1gpu::GPU_SWAPBUFFERS)
    {
        signalEvent(GPU_END_OF_FRAME_EVENT, "Frame rendered.");
        frame++;
        batch = 0;
    }

    _sendcgoMetaStream( new cgoMetaStream( com ) );

    GLOBAL_PROFILER_EXIT_REGION()
}


void HAL::sendCommand( GPUCommand* listOfCommands, int numberOfCommands )
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")

    int i;
    for ( i = 0; i < numberOfCommands; i++ )
        sendCommand(listOfCommands[i]);

    GLOBAL_PROFILER_EXIT_REGION()
}

void HAL::signalEvent(GPUEvent gpuEvent, string eventMsg)
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")

    _sendcgoMetaStream(new cgoMetaStream(gpuEvent, eventMsg));

    GLOBAL_PROFILER_EXIT_REGION()
}

cgoMetaStream* HAL::nextMetaStream()
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")
    if ( metaStreamCount != 0 ) {
        int oldOut = out;
        INC_MOD(out);
        metaStreamCount--;
        if ( metaStreamBuffer[oldOut]->getDebugInfo() != "" )
            cout << "cgoMetaStream debug info: " << metaStreamBuffer[oldOut]->getDebugInfo() << endl;

        DUMP_NEXTcgoMetaStream_TO_FILE(
            static ofstream f;
            static U32 tCounter = 0;
            if ( !f.is_open() )
                f.open("DrivercgoMetaStreams.txt");
            DUMP_TRANSACTION_COUNTERS( f << tCounter++ << ": "; )
            metaStreamBuffer[oldOut]->dump(f);
            f << endl;
        )

        GLOBAL_PROFILER_EXIT_REGION()

        return metaStreamBuffer[oldOut];
    }

    GLOBAL_PROFILER_EXIT_REGION()

    return NULL;
}

bool HAL::setSequentialStreamingMode( U32 count, U32 start )
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")
    GPURegData data;

    data.uintVal = count; // elements in streams 
    registerWriteBuffer.writeRegister(GPU_STREAM_COUNT, 0, data);

    // disable GPU indexMode
    data.uintVal = 0;
    registerWriteBuffer.writeRegister(GPU_INDEX_MODE, 0, data);

    data.uintVal = start;
    registerWriteBuffer.writeRegister(GPU_STREAM_START,0, data);

    GLOBAL_PROFILER_EXIT_REGION()

    return true;
}



bool HAL::setIndexedStreamingMode( U32 stream, U32 count, U32 start,
                                         U32 mdIndices, U32 offsetBytes, StreamData indicesType )
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")
    if ( stream >= MAX_STREAM_BUFFERS )
    {
        CG_ASSERT("invalid vertex attribute identifier");
    }

    GPURegData data;

    // enable GPU indexMode
    data.uintVal = 1;
    registerWriteBuffer.writeRegister(GPU_INDEX_MODE, 0, data);

    // select 'stream' as the stream used for streaming indices
    data.uintVal = stream;
    registerWriteBuffer.writeRegister(GPU_INDEX_STREAM, 0, data);

    // set the address of indices in gpu local memory
    _MemoryDescriptor* mdesc = _findMD( mdIndices );
    data.uintVal = mdesc->firstAddress + offsetBytes;
    registerWriteBuffer.writeRegister(GPU_STREAM_ADDRESS, stream, data, mdIndices);

    // no stride for indices
    data.uintVal = 0;
    registerWriteBuffer.writeRegister(GPU_STREAM_STRIDE, stream, data);

    // Type of indices
    data.streamData = indicesType;
    registerWriteBuffer.writeRegister(GPU_STREAM_DATA, stream, data);

    // One element(index) contains just one component (the index value)
    data.uintVal = 1;
    registerWriteBuffer.writeRegister(GPU_STREAM_ELEMENTS, stream, data);

    // Number of elements that will be streamed to vertex shader
    data.uintVal = count;
    registerWriteBuffer.writeRegister(GPU_STREAM_COUNT, 0, data);

    // Set first index (commonly 0)
    data.uintVal = start;
    registerWriteBuffer.writeRegister(GPU_STREAM_START, 0, data);

    GLOBAL_PROFILER_EXIT_REGION()

    return false;
}

bool HAL::setVShaderOutputWritten( ShAttrib attrib , bool isWritten )
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")
    CG_ASSERT("Method deprecated, cannot be used");

    GPURegData data;
    data.booleanVal = isWritten;

    if ( outputAttrib[attrib] == VS_NOOUTPUT && isWritten )
    {
        char msg[256];
        sprintf(msg, "Attribute (%d) cannot be written (it is not and output in VS)", attrib);
        CG_ASSERT(msg);
    }

    if ( isWritten )
        registerWriteBuffer.writeRegister( GPU_VERTEX_OUTPUT_ATTRIBUTE, outputAttrib[attrib], data );

    GLOBAL_PROFILER_EXIT_REGION()

    return true;
}

bool HAL::configureVertexAttribute( const VertexAttribute& va )
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")
    U32 stream = va.stream;
    U32 attrib = va.attrib;

    if (attrib >= MAX_VERTEX_ATTRIBUTES )
    {
        CG_ASSERT("invalid vertex attribute identifier");
    }

    GPURegData data;

    if ( va.enabled )
    {
        data.uintVal = stream;

        if ( stream >= MAX_STREAM_BUFFERS )
            CG_ASSERT("Stream out of range");

        registerWriteBuffer.writeRegister( GPU_VERTEX_ATTRIBUTE_MAP, attrib, data );

        // Set the local GPU stream buffer address for this attribute
        _MemoryDescriptor* md = _findMD( va.md );
        // compute start address in GPU local memory for this stream
        data.uintVal = md->firstAddress + va.offset;
        registerWriteBuffer.writeRegister( GPU_STREAM_ADDRESS, stream, data, va.md);

        // Set stream buffer stride for this attribute
        U32 stride = va.stride;
        if ( stride == 0 )
        {
            stride = va.components;
            switch ( va.componentsType )
            {
                case SD_UNORM8:
                case SD_SNORM8:
                case SD_UINT8:
                case SD_SINT8:
                    // nothing to do
                    break;

                case SD_UNORM16:
                case SD_SNORM16:
                case SD_UINT16:
                case SD_SINT16:
                case SD_FLOAT16:
                    stride *= 2;
                    break;

                case SD_UNORM32:
                case SD_SNORM32:
                case SD_FLOAT32:
                case SD_UINT32:
                case SD_SINT32:
                    stride *= 4;
                    break;
                default:
                    CG_ASSERT("Unknown components type");
            }
        }
        // else: use specific stride
        data.uintVal = stride;
        registerWriteBuffer.writeRegister( GPU_STREAM_STRIDE, stream, data );

        // Set stream data type for this attribute
        data.streamData = va.componentsType;
        registerWriteBuffer.writeRegister( GPU_STREAM_DATA, stream, data );

        // Set number of components for this attribute
        data.uintVal = va.components;
        registerWriteBuffer.writeRegister( GPU_STREAM_ELEMENTS, stream, data );
    }
    else
    {
        // disable this attribute
        data.uintVal = ST_INACTIVE_ATTRIBUTE;
        registerWriteBuffer.writeRegister( GPU_VERTEX_ATTRIBUTE_MAP, attrib, data );
    }

    GLOBAL_PROFILER_EXIT_REGION()

    return true;
}



bool HAL::commitVertexProgram( U32 memDesc, U32 programSize, U32 startPC )
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")

    /*

    GPURegData data;

    //_MemoryDescriptor* md = _findMD( memDesc );
    //if ( md == NULL )
    //{
    //    CG_ASSERT("Memory descriptor not found");
    //    return false;
    //}


     // PACTH to avoid:
     //   MemoryController:issueTransaction => GPU Memory operation out of range.
    data.uintVal = 0;
    registerWriteBuffer.writeRegister( GPU_PROGRAM_MEM_ADDR, 0, data );

    writeGPUAddrRegister(GPU_VERTEX_PROGRAM, 0, memDesc);
    //data.uintVal = md->firstAddress;
    //_sendcgoMetaStream( new cgoMetaStream( GPU_VERTEX_PROGRAM, 0, data ) );

    data.uintVal = programSize;
    registerWriteBuffer.writeRegister( GPU_VERTEX_PROGRAM_SIZE, 0, data );

    data.uintVal = startPC;
    registerWriteBuffer.writeRegister( GPU_VERTEX_PROGRAM_PC, 0, data );

    sendCommand( GPU_LOAD_VERTEX_PROGRAM );
    */


    if ( programSize % ShaderProgramSched::InstructionSize != 0 )
    {
        stringstream ss;
        ss << "program size: " << programSize << " is not multiple of instruction size: "
           << ShaderProgramSched::InstructionSize;
        CG_ASSERT(ss.str().c_str());
    }


//printf("HAL::commitVertexProgram => memDesc %d programSize %d startPC = %x\n", memDesc, programSize, startPC);
//_MemoryDescriptor *mdDesc = _findMD(memDesc);
//if (mdDesc != NULL)
//printf("HAL::commitVertexProgram => firstAddress %x lastAddress %x size %d\n", mdDesc->firstAddress, mdDesc->lastAddress, mdDesc->size);
//else
//printf("HAL::commitVertexProgram => Memory Descriptor not found\n");

    shSched.select(memDesc, programSize / ShaderProgramSched::InstructionSize, VERTEX_TARGET);

    GLOBAL_PROFILER_EXIT_REGION()

    return true;
}


bool HAL::commitFragmentProgram( U32 memDesc, U32 programSize, U32 startPC )
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")

    /*

    GPURegData data;

    //_MemoryDescriptor* md = _findMD( memDesc );
    //if ( md == NULL )
    //{
    //    CG_ASSERT("Memory descriptor not found");
    //    return false;
    //}


     //PACTH to avoid:
    //    MemoryController:issueTransaction => GPU Memory operation out of range.

    data.uintVal = 0;
    registerWriteBuffer.writeRegister( GPU_PROGRAM_MEM_ADDR, 0, data );

    writeGPUAddrRegister(GPU_FRAGMENT_PROGRAM, 0, memDesc);
    //data.uintVal = md->firstAddress;
    //_sendcgoMetaStream( new cgoMetaStream( GPU_FRAGMENT_PROGRAM, 0, data ) );

    data.uintVal = programSize;
    registerWriteBuffer.writeRegister( GPU_FRAGMENT_PROGRAM_SIZE, 0, data );

    data.uintVal = startPC;
    registerWriteBuffer.writeRegister( GPU_FRAGMENT_PROGRAM_PC, 0, data );

    sendCommand( GPU_LOAD_FRAGMENT_PROGRAM );
    */


    if ( programSize % ShaderProgramSched::InstructionSize != 0 )
    {
        stringstream ss;
        ss << "program size: " << programSize << " is not multiple of instruction size: "
           << ShaderProgramSched::InstructionSize;
        CG_ASSERT(ss.str().c_str());
    }

    shSched.select(memDesc, programSize / ShaderProgramSched::InstructionSize, FRAGMENT_TARGET);

    GLOBAL_PROFILER_EXIT_REGION()

    return true;
}

HAL::_MemoryDescriptor* HAL::_createMD( U32 firstAddress, U32 lastAddress, U32 size )
{
    //  Get a new memory descriptor id.
    U32 mdID = nextMemId++;

    //  Verify that the new MD doesn't already exist.
    map <U32, _MemoryDescriptor*>::iterator it;
    it = memoryDescriptors.find(mdID);

    if (it == memoryDescriptors.end())
    {
        _MemoryDescriptor *md = new _MemoryDescriptor;
        md->size = size;
        md->firstAddress = firstAddress;
        md->lastAddress = lastAddress;
        md->memId = mdID;

        // debug
        md->highAddressWritten = firstAddress;

        //  Add to the map of Memory Descriptors.
        memoryDescriptors[mdID] = md;

        return md;
    }
    else
    {
        //  Error, memory descriptor identifier already used!!
        return NULL;
    }

    /*U32 i = nextMemId++;
    for ( i; i != nextMemId - 2 ; i++ )
    {
        _MemoryDescriptor* md = mdList;
        for ( md; md != NULL; md = md->next )
        { // check all memory id's
            if ( md->memId == i ) // occupied
                break; // ( md != NULL )
        }
        if ( md == NULL ) { // identifier available
            md = new _MemoryDescriptor;
            md->size = size;
            md->firstAddress = firstAddress;
            md->lastAddress = lastAddress;
            md->memId = i;
            md->next = mdList;
            mdList = md;
            // debug
            md->highAddressWritten = firstAddress;
            return md;
        }
    }
    return NULL; // max id's used...*/
}


HAL::_MemoryDescriptor* HAL::_findMD( U32 memId )
{
    GLOBAL_PROFILER_ENTER_REGION("_findMD", "HAL", "_findMD");
    mdSearches++;

    map<U32, _MemoryDescriptor*>::iterator it;

    it = memoryDescriptors.find(memId);

    if (it != memoryDescriptors.end())
    {
        GLOBAL_PROFILER_EXIT_REGION()
        return it->second;
    }

    //_MemoryDescriptor* md = mdList;
    //for ( md; md != NULL; md = md->next ) {
    //    if ( md->memId == memId )
    //        return md;
    //}
    GLOBAL_PROFILER_EXIT_REGION()
    return NULL;
}

HAL::_MemoryDescriptor* HAL::_findMDByAddress( U32 physicalAddress )
{
    //_MemoryDescriptor* md = mdList;
    //for ( md; md != NULL; md = md->next ) {
    //    if ( md->firstAddress <= physicalAddress && physicalAddress <= md->lastAddress )
    //        return md;
    //}

    addressSearches++;

    bool found = false;

    map<U32, _MemoryDescriptor*>::iterator it;
    it = memoryDescriptors.begin();

    while (!found && it != memoryDescriptors.end())
    {
        found = (it->second->firstAddress <= physicalAddress) && (physicalAddress <= it->second->lastAddress);
        if (!found)
            it++;
    }

    if (found)
        return it->second;

    return NULL;
}


//  Release a memory descriptor and deallocate the associated memory.  
void HAL::_releaseMD( U32 memId )
{
    // Remove memId from Shader Program Schedulers (if memId points to a Program)
    //vshSched.remove(memId);
    //fshSched.remove(memId);
    shSched.remove(memId);

    U32 isGPUMem;
    U32 i;
    //_MemoryDescriptor* md = mdList;
    //_MemoryDescriptor* prev = mdList;

    ////  Search the memory descriptor.  
    //for (; (md != NULL) && (md->memId != memId); prev = md, md = md->next );

    _MemoryDescriptor *md;

    //  Search for the memory descriptor.
    map<U32, _MemoryDescriptor*>::iterator it;
    it = memoryDescriptors.find(memId);
    if (it != memoryDescriptors.end())
        md = it->second;
    else
        md = NULL;

    //  Check if the memory descriptor was found.  
    if ((md != NULL) && (md->memId == memId))
    {
        //  Update statistics.  
#ifdef _DRIVER_STATISTICS
            memoryDeallocations++;
#endif

        //  Determine the address space for the memory descriptor.  
        switch(md->firstAddress & ADDRESS_SPACE_MASK)
        {
            case GPU_ADDRESS_SPACE:

                //  Deallocate blocks.  
                for ( i = (md->firstAddress & SPACE_ADDRESS_MASK) / (BLOCK_SIZE*1024);
                    i <= (md->lastAddress & SPACE_ADDRESS_MASK) / (BLOCK_SIZE*1024); i++ )
                    gpuMap[i] = 0;

                //  Update number of allocated blocks in GPU memory.  
                gpuAllocBlocks -= ((md->lastAddress & SPACE_ADDRESS_MASK) / (BLOCK_SIZE*1024)) -
                    ((md->firstAddress & SPACE_ADDRESS_MASK) / (BLOCK_SIZE*1024)) + 1;

                break;

            case SYSTEM_ADDRESS_SPACE:

                //  Deallocate blocks.  
                for ( i = (md->firstAddress & SPACE_ADDRESS_MASK) / (BLOCK_SIZE*1024);
                    i <= (md->lastAddress & SPACE_ADDRESS_MASK) / (BLOCK_SIZE*1024); i++ )
                    systemMap[i] = 0;

                //  Update number of allocated blocks system GPU memory.  
                systemAllocBlocks -= ((md->lastAddress & SPACE_ADDRESS_MASK) / (BLOCK_SIZE*1024)) -
                    ((md->firstAddress & SPACE_ADDRESS_MASK) / (BLOCK_SIZE*1024)) + 1;

                break;

            default:
                CG_ASSERT("Unsupported GPU address space.");
                break;
        }

        //  Delete and remove memory descriptor from the map.
        memoryDescriptors.erase(it);
        delete md;

        //  destroy _MemoryDescriptor  
        //if ( md == mdList )
        //{
        //    mdList = md->next;
        //    delete md;
        //}
        //else
        //{ // last or in the middle ( prev exists )
        //    prev->next = md->next;
        //    delete md;
        //}
    }
    else
    {
        CG_ASSERT("Memory descriptor not found.");
    }
}


void HAL::releaseMemory( U32 md )
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")
    if ( md == 0 )
    {
        GLOBAL_PROFILER_EXIT_REGION()
        return ; // releasing NULL (ignored)
    }

    // Comment this code to have a exact memory footprint when using preload/normal write
    /*
    if ( preloadMemory )
    {
        // do inmediate release of memory
        _releaseMD(md);
        return ;
    }
    */

    pendentReleases.push_back(md); // deferred releasing (to support batch pipelining)
    //_releaseMD( md );
    GLOBAL_PROFILER_EXIT_REGION()
}


//  Search a memory map for consecutive blocks for the required memory.  
U32 HAL::searchBlocks(U32 &first, U08 *map, U32 memSize, U32 memRequired)
{
    GLOBAL_PROFILER_ENTER_REGION("searchBlock", "HAL", "searchBlocks");

    U32 i;
    //U32 first = lastBlock;
    U32 accum = 0;
    U32 blocks = 0;

    //  Search for empty blocks until enough have been r.  
    for (i = first = 0; (i < (memSize / BLOCK_SIZE)) && (accum < memRequired); i++ )
    {
        //  Check if the block is already used.  
        if (map[i] == 1)
        {
            //  Restart search of consecutive blocks.  
            accum = 0;
            blocks = 0;
            first = i + 1;
        }
        else
        {
            //  Block is empty.   map[i] == 0.  

            //  Update consecutive empty blocks counter.  
            accum += BLOCK_SIZE*1024;
            blocks++;
        }
    }

    //  Check if search was successful.  
    if (accum >= memRequired)
    {
        GLOBAL_PROFILER_EXIT_REGION()
        //  Return the address of the first of the consecutive blocks.  
        return blocks;
    }
    else
    {
        GLOBAL_PROFILER_EXIT_REGION()
        //  Return no consecutive blocks available.  
        return 0;
    }
}

U32 HAL::obtainMemory( U32 memRequired, MemoryRequestPolicy memRequestPolicy )
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")
    bool useGPUMem;
    U32 first;
    U32 blocks;
    U32 i;
    U32 firstAddress;
    U32 lastAddress;

    if ( !setGPUParametersCalled )
        CG_ASSERT("Before obtaining memory is required to call setGPUParameters");

    if ( memRequired == 0 )
        CG_ASSERT("0 bytes required ??? (programming error?)");

    if ( memRequestPolicy == GPUMemoryFirst )
    {
        blocks = searchBlocks(first, gpuMap, gpuMemory, memRequired);
        if ( blocks == 0 ) // memory couldn't be allocated in GPU local memory, try with system memory
        {
            blocks = searchBlocks(first, systemMap, systemMemory, memRequired);
            useGPUMem = false;
        }
        else
            useGPUMem = true;
    }
    else // SystemMemoryFirst
    {
        blocks = searchBlocks(first, systemMap, systemMemory, memRequired);
        if ( blocks == 0 ) // memory couldn't be allocated in system memory, try with GPU local memory
        {
            blocks = searchBlocks(first, gpuMap, gpuMemory, memRequired);
            useGPUMem = true;
        }
        else
            useGPUMem = false;
    }


    //  Check if a block was found.  
    if (blocks > 0)
    {
        //  Update statistics.  
        #ifdef _DRIVER_STATISTICS
            memoryAllocations++;
        #endif

        //  Check if reserve GPU memory.  
        if (useGPUMem)
        {
            //  Set the gpu memory blocks as allocated.  
            for (i = first; i < (first + blocks); i++)
                gpuMap[i] = 1;

            //  Update the number of allocated blocks in GPU memory.  
            gpuAllocBlocks += blocks;
        }
        else
        {
            //  Set the system memory blocks as allocated.  
            for (i = first; i < (first + blocks); i++)
                systemMap[i] = 1;

            //  Update the number of allocated blocks in system memory.  
            systemAllocBlocks += blocks;
        }

        //  Calculate the start address for the allocated memory.  
        firstAddress = first * BLOCK_SIZE * 1024;

        //  Calculate the last address for the allocated memory.  
        lastAddress =((first + blocks) * BLOCK_SIZE * 1024) - 1;

        //  Apply address space offsets.  
        if (useGPUMem)
        {
            firstAddress += GPU_ADDRESS_SPACE;
            lastAddress += GPU_ADDRESS_SPACE;
        }
        else
        {
            firstAddress += SYSTEM_ADDRESS_SPACE;
            lastAddress += SYSTEM_ADDRESS_SPACE;
        }

        //  Create the memory descriptor.  
        _MemoryDescriptor* md = _createMD(firstAddress, lastAddress, memRequired);

        //  Check if the memory descriptor was correctly allocated.  
        if ( md == NULL )
        {
            GLOBAL_PROFILER_EXIT_REGION()
            CG_ASSERT("No memory descriptors available");
            return 0;
        }

        //  Set next empty block search point.  
        lastBlock = first + blocks;

        GLOBAL_PROFILER_EXIT_REGION()
        return md->memId;
    }
    else
    {
        GLOBAL_PROFILER_EXIT_REGION()
        CG_ASSERT("No memory available");
        return 0;
    }

}


bool HAL::writeMemory( U32 md, const U08* data, U32 dataSize, bool isLocked )
{
    return writeMemory( md, 0, data, dataSize, isLocked );
}

/*
bool HAL::writeMemoryDebug(U32 md, U32 offset, const U08* data, U32 dataSize, const std::string& debugInfo)
{
    _MemoryDescriptor* memDesc = _findMD( md );
    if ( memDesc == NULL )
        CG_ASSERT("Memory descriptor does not exist");
    if ( !CHECK_MEMORY_ACCESS(memDesc,offset,dataSize) )
        CG_ASSERT("Access memory out of range");

    UPDATE_HIGH_ADDRESS_WRITTEN(memDesc,offset,dataSize);

    cgoMetaStream* agpt = new cgoMetaStream( memDesc->firstAddress, dataSize,(U08*)data, true );
    char buf[256];
    sprintf(buf, "%d", dataSize);
    agpt->setDebugInfo(string() + debugInfo + "  Size: " + buf);
    _sendcgoMetaStream(agpt);
    return true;
}

*/

bool HAL::writeMemory( U32 md, U32 offset, const U08* data, U32 dataSize, bool isLocked )
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")

    _MemoryDescriptor* memDesc = _findMD( md );
    if ( memDesc == NULL )
        CG_ASSERT("Memory descriptor does not exist");

    if ( !CHECK_MEMORY_ACCESS(memDesc,offset,dataSize) )
        CG_ASSERT("Access memory out of range");

    UPDATE_HIGH_ADDRESS_WRITTEN(memDesc,offset,dataSize);

    if ( preloadMemory )
    {
        _sendcgoMetaStream( new cgoMetaStream( memDesc->firstAddress + offset, dataSize,(U08*)data, md) );
        
        memPreloads++;
        memPreloadBytes += dataSize;
    }
    else
    {
        _sendcgoMetaStream( new cgoMetaStream( memDesc->firstAddress + offset, dataSize,(U08*)data, md, true, isLocked) );
        
        memWrites++;
        memWriteBytes += dataSize;
    }

    GLOBAL_PROFILER_EXIT_REGION()

    return true;
}

bool HAL::writeMemoryPreload(U32 md, U32 offset, const U08* data, U32 dataSize)
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")

    _MemoryDescriptor* memDesc = _findMD(md);
    
    if (memDesc == NULL)
        CG_ASSERT("Memory descriptor does not exist");

    if (!CHECK_MEMORY_ACCESS(memDesc,offset,dataSize))
        CG_ASSERT("Access memory out of range");

    UPDATE_HIGH_ADDRESS_WRITTEN(memDesc,offset,dataSize);

    //  Create META_STREAM_PRELOAD transaction.
    _sendcgoMetaStream(new cgoMetaStream(memDesc->firstAddress + offset, dataSize, (U08*) data, md));

    memPreloads++;
    memPreloadBytes += dataSize;
    
    GLOBAL_PROFILER_EXIT_REGION()

    return true;
}

void HAL::printMemoryUsage()
{
    printf("HAL => Memory usage : GPU %d blocks | System %d blocks\n", gpuAllocBlocks,
        systemAllocBlocks);
}

void HAL::dumpMemoryAllocation( bool contents )
{
    if ( !setGPUParametersCalled )
        CG_ASSERT("Before tracing memory is required to call setGPUParameters");

    U32 i;
    printf( "GPU memory mapping: \n" );
    printf( "%d Kbytes of available GPU memory\n", gpuMemory );
    printf( "%d KBytes per Block\n", BLOCK_SIZE );
    printf( "-------------------------------------\n" );
    for ( i = 0; i < gpuMemory / BLOCK_SIZE; i++ )
    {
        if ( gpuMap[i] == 0 )
            printf( "GPU Map %d : FREE\n", i );
        else {
            _MemoryDescriptor* md = _findMDByAddress( i*BLOCK_SIZE*1024 );
            printf( "Map %d : OCCUPIED --> MD:%d ", i, md->memId );
            if ( contents )
                printf( "high address written = %d\n", md->highAddressWritten );
            else
                printf( "\n" );
        }
    }
    printf( "-------------------------------------\n" );
    printf( "System memory mapping: \n" );
    printf( "%d Kbytes of available GPU memory\n", systemMemory );
    printf( "%d KBytes per Block\n", BLOCK_SIZE );
    printf( "-------------------------------------\n" );
    for ( i = 0; i <systemMemory / BLOCK_SIZE; i++ )
    {
        if ( systemMap[i] == 0 )
            printf( "System Map %d : FREE\n", i );
        else {
            _MemoryDescriptor* md = _findMDByAddress( i*BLOCK_SIZE*1024 );
            printf( "Map %d : OCCUPIED --> MD:%d ", i, md->memId );
            if ( contents )
                printf( "high address written = %d\n", md->highAddressWritten );
            else
                printf( "\n" );
        }
    }
    printf( "-------------------------------------\n" );
}



void HAL::dumpMD( U32 md )
{
    _MemoryDescriptor* memDesc = _findMD( md );
    if ( memDesc != NULL ) {
        printf( "Descriptor ID: %d\n", memDesc->memId );
        printf( "First Address: %d\n", memDesc->firstAddress );
        printf( "Last Address: %d\n", memDesc->lastAddress );
    }
    else
        printf( "Descriptor unused.\n" );
}


void HAL::dumpMDs()
{
    //_MemoryDescriptor* memDesc = mdList;
    //for ( memDesc ; memDesc != NULL; memDesc = memDesc->next ) {
    //    printf( "Descriptor ID: %d\n", memDesc->memId );
    //    printf( "First Address: %d\n", memDesc->firstAddress );
    //    printf( "Last Address: %d\n", memDesc->lastAddress );
    //}

    map<U32, _MemoryDescriptor*>::iterator it;
    it = memoryDescriptors.begin();
    while (it != memoryDescriptors.end())
    {
        printf( "Descriptor ID: %d\n", it->second->memId );
        printf( "First Address: %d\n", it->second->firstAddress );
        printf( "Last Address: %d\n", it->second->lastAddress );
    }
}


void HAL::dumpMetaStreamBuffer()
{
    if ( metaStreamCount == 0 ) {
        printf( "MetaStreamBuffer is empty\n" );
        return ;
    }
    printf( "MetaStreamBuffer contents (cgoMetaStream objects):\n" );
    int i = out;
    int c = 1;
    while ( i != in ) {
        printf( "%d:", c );
        switch ( metaStreamBuffer[i]->GetMetaStreamType() ) {
            case META_STREAM_WRITE :
                printf( "META_STREAM_WRITE\n" );
                break;
            case META_STREAM_READ :
                printf( "META_STREAM_READ\n" );
                break;
            case META_STREAM_REG_WRITE :
                printf( "META_STREAM_REG_WRITE\n" );
                break;
            case META_STREAM_REG_READ :
                printf( "META_STREAM_REG_READ\n" );
                break;
            case META_STREAM_COMMAND :
                printf( "META_STREAM_COMMAND\n" );
                break;
        }
        c++;
        INC_MOD(i);
    }
}

void HAL::dumpStatistics()
{
    printf("---\n" );
    printf("metaStreamGenerated : %d\n", metaStreamGenerated );
    printf("memoryAllocations : %d\n", memoryAllocations );
    printf("memoryDeallocations : %d\n", memoryDeallocations );
    printf("mdSearches : %d\n", mdSearches);
    printf("addressSearches : %d\n", addressSearches);
    printf("memPreloads : %d\n", memPreloads);
    printf("memPreloadBytes : %d\n", memPreloadBytes);
    printf("memWrites : %d\n", memWrites);
    printf("memWriteBytes : %d\n", memWriteBytes);
}


U32 HAL::getTextureUnits() const
{
    return MAX_TEXTURES;
}


U32 HAL::getMaxMipmaps() const
{
    return MAX_TEXTURE_SIZE;
}


void HAL::setResolution(U32 width, U32 height)
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")
    hRes = width;
    vRes = height;
    setResolutionCalled = true;
    GPURegData data;
    data.uintVal = width;
    driver->writeGPURegister( GPU_DISPLAY_X_RES, data );
    data.uintVal = height;
    driver->writeGPURegister( GPU_DISPLAY_Y_RES, data );
    GLOBAL_PROFILER_EXIT_REGION()
}

void HAL::getResolution(U32& width, U32& height) const
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")
    width = hRes;
    height = vRes;
    GLOBAL_PROFILER_EXIT_REGION()
}

bool HAL::isResolutionDefined() const
{
    return setResolutionCalled;
}


void HAL::VertexAttribute::dump() const
{
    std::cout << "Stream: " << stream << std::endl;
    std::cout << "Attrib ID: " << attrib << std::endl;
    std::cout << "Enabled: " << enabled << std::endl;
    std::cout << "Memory descriptor: " << md << std::endl;
    std::cout << "Offset: " << offset << std::endl;
    std::cout << "Stride: " << stride << std::endl;
    std::cout << "ComponentsType: " << componentsType << std::endl;
    std::cout << "# components: " << components << std::endl;
}

U32 HAL::getMaxVertexAttribs() const
{
    return MAX_VERTEX_ATTRIBUTES;
}

U32 HAL::getMaxAddrRegisters() const
{
    // Information extracted from bmUnifiedShader.h, line 77:  "VS2_ADDRESS_NUM_REGS = 2" 
    return 2;
}

void HAL::setPreloadMemory(bool enable)
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")
    preloadMemory = enable;
    if ( !preloadMemory )
    {
        // clear ProgramShaderSched's since commands (GPU_LOAD_VERTEX_PROGRAM & GPU_LOAD_FRAGMENT_PROGRAM
        // have been ignored and their contents are wrong
        //vshSched.clear();
        //fshSched.clear();
        shSched.clear();
    }
    GLOBAL_PROFILER_EXIT_REGION()
}

bool HAL::getPreloadMemory() const
{
    return preloadMemory;
}

void HAL::translateShaderProgram(U08 *inCode, U32 inSize, U08 *outCode, U32 &outSize,
                                 bool isVertexProgram, U32 &maxLiveTempRegs, MicroTriangleRasterSettings settings)
{
    GLOBAL_PROFILER_ENTER_REGION("HAL", "", "")
    if (!enableShaderProgramTransformations) //  Check if shader translation is disabled.
    {
        //  If disabled just copy the original program.
        CG_ASSERT_COND((inSize <= outSize), "Output shader program buffer size is too small.");
        memcpy(outCode, inCode, inSize);
        outSize = inSize;
        GLOBAL_PROFILER_EXIT_REGION()
        return;
    }

    vector<cgoShaderInstr *> inputProgram;
    vector<cgoShaderInstr *> programTemp1;
    vector<cgoShaderInstr *> programTemp2;
    vector<cgoShaderInstr *> programOptimized;
    vector<cgoShaderInstr *> programFinal;

    bool verbose = false;
    bool optimization = true;
    bool setWaitPoints = true;

    U32 numTemps;
    bool hasJumps;
    
    ShaderOptimization::decodeProgram(inCode, inSize, inputProgram, numTemps, hasJumps);

    //  Disable optimizations if the shader has jumps (not supported yet).
    optimization = optimization && !hasJumps;
    
    if (hasJumps)
        printf("HAL::translateShaderProgram => Jump instructions found in the program.  Disabling optimizations.\n");
    
    if (verbose)
    {
        printf("Starting new shader program translation.\n");
        printf(" >>> vertex program = %s | convert to LDA = %s | convert to Scalar = %s\n\n",
            isVertexProgram ? "Yes" : "No", convertShaderProgramToLDA ? "Yes" : "No", convertShaderProgramToSOA ? "Yes" : "No");
        printf("Input Program : \n");
        printf("----------------------------------------\n");

        ShaderOptimization::printProgram(inputProgram);

        printf("\n");
    }

    if (convertShaderProgramToLDA && isVertexProgram)
    {
        ShaderOptimization::attribute2lda(inputProgram, programTemp1);

        if (verbose)
        {
            printf("Convert input registers to LDA : \n");
            printf("----------------------------------------\n");

            ShaderOptimization::printProgram(programTemp1);

            printf("\n");
        }
    }
    else
        ShaderOptimization::copyProgram(inputProgram, programTemp1);

    ShaderOptimization::deleteProgram(inputProgram);

    if (convertShaderProgramToSOA)
    {
        ShaderOptimization::aos2soa(programTemp1, programTemp2);
        ShaderOptimization::deleteProgram(programTemp1);
        ShaderOptimization::copyProgram(programTemp2, programTemp1);
        ShaderOptimization::deleteProgram(programTemp2);

        if (verbose)
        {
            printf("SIMD4 to Scalar conversion : \n");
            printf("----------------------------------------\n");

            ShaderOptimization::printProgram(programTemp1);

            printf("\n");
        }
    }

    if (optimization)
        ShaderOptimization::optimize(programTemp1, programOptimized, maxLiveTempRegs, false, convertShaderProgramToSOA, verbose);
    else
    {
        ShaderOptimization::copyProgram(programTemp1, programOptimized);
        maxLiveTempRegs = numTemps;
    }
    
    ShaderOptimization::deleteProgram(programTemp1);

    if (verbose)
    {
        printf("Optimized program : \n");
        printf("----------------------------------------\n");

        ShaderOptimization::printProgram(programOptimized);

        printf("\n");
    }

    if (setWaitPoints)
    {
        ShaderOptimization::assignWaitPoints(programOptimized, programFinal);

        if (verbose)
        {
            printf("Program with wait points : \n");
            printf("----------------------------------------\n");

            ShaderOptimization::printProgram(programFinal);

            printf("\n");
        }
    }
    else
    {
        ShaderOptimization::copyProgram(programOptimized, programFinal);
    }

    ShaderOptimization::deleteProgram(programOptimized);

    ShaderOptimization::encodeProgram(programFinal, outCode, outSize);

    ShaderOptimization::deleteProgram(programFinal);

    GLOBAL_PROFILER_EXIT_REGION()
}

U32 HAL::assembleShaderProgram(U08 *program, U08 *code, U32 size)
{
    ShaderOptimization::assembleProgram(program, code, size);
    
    return size;
}

U32 HAL::disassembleShaderProgram(U08 *code, U32 codeSize, U08 *program, U32 size, bool &disableEarlyZ)
{
    ShaderOptimization::disassembleProgram(code, codeSize, program, size, disableEarlyZ);
    
    return size;
}

void HAL::dumpRegisterStatus (int frame, int batch)
{
    registerWriteBuffer.dumpRegisterStatus(frame, batch);
}

void HAL::MortonTableBuilder()
{
    for(U32 i = 0; i < 256; i++)
    {
        U32 t1 = i & 0x0F;
        U32 t2 = (i >> 4) & 0x0F;
        U32 m = 0;

        for(U32 nextBit = 0; nextBit < 4; nextBit++)
        {
            m += ((t1 & 0x01) << (2 * nextBit)) + ((t2 & 0x01) << (2 * nextBit + 1));

            t1 = t1 >> 1;
            t2 = t2 >> 1;
        }

        _mortonTable[i] = m;
    }
}

U08* HAL::getDataInMortonOrder( U08* originalData, U32 width, U32 height, U32 depth, TextureCompression format, U32 texelSize, U32& mortonDataSize)
{

    U08* mortonData;
    //  Check for compressed formats and compute the compressed block size.
    U32 s3tcBlockSz;
    switch ( format )
    {
        case GPU_S3TC_DXT1_RGB:
        case GPU_S3TC_DXT1_RGBA:
        case GPU_LATC1:
        case GPU_LATC1_SIGNED:
            s3tcBlockSz = 8;
            break;
        case GPU_S3TC_DXT3_RGBA:
        case GPU_S3TC_DXT5_RGBA:
        case GPU_LATC2:
        case GPU_LATC2_SIGNED:
            s3tcBlockSz = 16;
            break;
        default: //GPU_NO_TEXTURE_COMPRESSION
            s3tcBlockSz = 0;
    }

    U32 w2;

    //  Check if compressed texture.
    if (s3tcBlockSz != 0)
    {
        // Compressed texture

        //  Compute the size of the mipmap data in morton order.
        //  NOTE: The width and height of the mipmap must be clamped to 1 block (4x4).
        U32 w2 = (U32) ceil(logTwo(max(width >> 2, U32(1))));
        mortonDataSize = s3tcBlockSz * (_texel2address(w2, blocksz - 2, sblocksz,
                                                                      max((width >> 2), U32(1)) - 1,
                                                                      max((height >> 2), U32(1)) - 1) + 1);

        //  Allocate the memory buffer for the mipmap data in morton order.
        mortonData = new U08[mortonDataSize];

        // Convert mipmap data to morton order.
        //  NOTE: The width and height of the mipmap must be clamped to 1 block (4x4).
        for( U32 i = 0; i < (max(height >> 2, U32(1))); ++i )
        {
            for( U32 j = 0; j < (max(width >> 2, U32(1))); ++j )
            {
                U32 a = _texel2address(w2, blocksz - 2, sblocksz, j, i);
                for(U32 k = 0; k < (s3tcBlockSz >> 2); ++k )
                    ((U32 *) mortonData)[(a * (s3tcBlockSz >> 2)) + k] = ((U32 *) originalData)[(i * max((width >> 2), U32(1)) + j) * (s3tcBlockSz >> 2) + k];
            }
        }


        return mortonData;
    }
    else
    {
        // Uncompressed texture.

        //  Compute the size of the mipmap data in morton order.
        w2 = (U32)ceil(logTwo(width));
        U32 mortonDataSliceSize = texelSize*(_texel2address(w2, blocksz, sblocksz, width-1, height-1) + 1);
        mortonDataSize = mortonDataSliceSize * depth;
        //  Allocate the memory buffer for the mipmap data in morton order.
        mortonData = new U08[mortonDataSize];
        U08* tmpMortonData = mortonData;
        switch ( texelSize )
        {
            case 1:
                {
                    for ( U32 k = 0; k < depth; ++k)
                    {
                        for ( U32 i = 0; i < height; ++i )
                        {
                            for( U32 j = 0; j < width; ++j )
                            {
                                U32 a = _texel2address(w2, blocksz, sblocksz, j, i);
                                ((U08 *) tmpMortonData)[a] = ((U08 *) originalData)[(width * height * k) + i * width + j];
                            }
                        }
                        tmpMortonData += mortonDataSliceSize;
                    }
                }
                break;
            case 2:
                {
                    for ( U32 k = 0; k < depth; ++k)
                    {
                        for ( U32 i = 0; i < height; ++i )
                        {
                            for( U32 j = 0; j < width; ++j )
                            {
                                U32 a = _texel2address(w2, blocksz, sblocksz, j, i);
                                ((U16 *) tmpMortonData)[a] = ((U16 *) originalData)[(width * height * k) + i * width + j];
                            }
                        }
                        tmpMortonData += mortonDataSliceSize;
                    }
                }
                break;
            case 4:
                {
                    for ( U32 k = 0; k < depth; ++k)
                    {
                        for( U32 i = 0; i < height; ++i )
                        {
                            for( U32 j = 0; j < width; ++j )
                            {
                                U32 a = _texel2address(w2, blocksz, sblocksz, j, i);
                                ((U32 *) tmpMortonData)[a] = ((U32 *) originalData)[(width * height * k) + i * width + j];
                            }
                        }
                        tmpMortonData += mortonDataSliceSize;
                    }
                }
                break;
            case 8:
                {
                    for ( U32 k = 0; k < depth; ++k)
                    {
                        for( U32 i = 0; i < height; ++i )
                        {
                            for( U32 j = 0; j < width; ++j )
                            {
                                U32 a = _texel2address(w2, blocksz, sblocksz, j, i);
                                ((U64 *) tmpMortonData)[a] = ((U64 *) originalData)[(width * height * k) + i * width + j];
                            }
                        }
                        tmpMortonData += mortonDataSliceSize;
                    }
                }
                break;
            case 16:
                {
                    for ( U32 k = 0; k < depth; ++k)
                    {
                        for( U32 i = 0; i < height; ++i )
                        {
                            for( U32 j = 0; j < width; ++j )
                            {
                                U32 a = _texel2address(w2, blocksz, sblocksz, j, i);
                                ((U64 *) tmpMortonData)[a * 2] = ((U64 *) originalData)[((width * height * k) + i * width + j) * 2];
                                ((U64 *) tmpMortonData)[a * 2 + 1] = ((U64 *) originalData)[((width * height * k) + i * width + j) * 2 + 1];
                            }
                        }
                        tmpMortonData += mortonDataSliceSize;
                    }
                }
                break;
            default:
            {
                stringstream ss;
                ss << "Only morton transformations with texel size 1, 4, 8 or 16 bytes supported. texel size = "
                   << texelSize;
                CG_ASSERT(ss.str().c_str());
            }
        }

        return mortonData;
    }
}

U32 HAL::_mortonFast(U32 size, U32 i, U32 j)
{
    U32 address;
    U32 t1 = i;
    U32 t2 = j;

    switch(size)
    {
        case 0:
            return 0;
        case 1:
            return _mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)] & 0x03;
        case 2:
            return _mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)] & 0x0F;
        case 3:
            return _mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)] & 0x3F;
        case 4:
            return _mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)];
        case 5:
            address = _mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)];
            address += (_mortonTable[(((t2 >> 4) & 0x0F) << 4) | ((t1 >> 4) & 0x0F)] & 0x03) << 8;
            return address;
        case 6:
            address = _mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)];
            address += (_mortonTable[(((t2 >> 4) & 0x0F) << 4) | ((t1 >> 4) & 0x0F)] & 0x0F) << 8;
            return address;
        case 7:
            address = _mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)];
            address += (_mortonTable[(((t2 >> 4) & 0x0F) << 4) | ((t1 >> 4) & 0x0F)] & 0x3F) << 8;
            return address;
        case 8:
            address = _mortonTable[((t2 & 0x0F) << 4) | (t1 & 0x0F)];
            address += _mortonTable[(((t2 >> 4) & 0x0F) << 4) | ((t1 >> 4) & 0x0F)] << 8;
            return address;
        default:
            CG_ASSERT("Fast morton not supported for this tile size");
            break;
    }

    return 0;
}
U32 HAL::_texel2address(U32 width, U32 blockSz, U32 sBlockSz, U32 i, U32 j)
{
    U32 address;
    U32 texelAddr;
    U32 blockAddr;
    U32 sBlockAddr;

    //  Calculate the address of the texel inside the block using Morton order.  
    //texelAddr = morton(blockSz, i, j);
    texelAddr = _mortonFast(blockSz, i, j);

    //  Calculate the address of the block inside the superblock using Morton order.  
    //blockAddr = morton(sBlockSz, i >> blockSz, j >> blockSz);
    blockAddr = _mortonFast(sBlockSz, i >> blockSz, j >> blockSz);

    //  Calculate thte address of the superblock inside the cache.  
    sBlockAddr = ((j >> (sBlockSz + blockSz)) << max(S32(width - (sBlockSz + blockSz)), S32(0))) + (i >> (sBlockSz + blockSz));

    address = (((sBlockAddr << (2 * sBlockSz)) + blockAddr) << (2 * blockSz)) + texelAddr;

    return address;
}

F64 HAL::ceil(F64 x)
{
    return (x - std::floor(x) > 0)?std::floor(x) + 1:std::floor(x);
}

F64 HAL::logTwo(F64 x)
{
    return std::log(x)/std::log(2.0);
}

