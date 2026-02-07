/**************************************************************************
 *
 * GPU behaviorModel implementation file.
 *  This file contains the implementation of functions for the CG1 GPU behaviorModel.
 *
 */
#include "bmGpuTop.h"
#include "GlobalProfiler.h"
#include "ImageSaver.h"
#include "bmClipper.h"
#include <functional>
#include <iostream>
#include <cstring>

using namespace std;

//#define DUMP_RT_AFTER_DRAW

namespace cg1gpu
{

//  Constructor.
bmoGpuTop::bmoGpuTop(cgsArchConfig ArchConf, cgoTraceDriverBase *TraceDriver) :
    ArchConf(ArchConf), TraceDriver(TraceDriver), 
    cacheDXT1RGB(*this, 1024, 64, 0x003C, DXT1_SPACE_SHIFT, bmoTextureProcessor::decompressDXT1RGB),
    cacheDXT1RGBA(*this, 1024, 64, 0x003C, DXT1_SPACE_SHIFT, bmoTextureProcessor::decompressDXT1RGBA),
    cacheDXT3RGBA(*this, 1024, 64, 0x003C, DXT3_DXT5_SPACE_SHIFT, bmoTextureProcessor::decompressDXT3RGBA),
    cacheDXT5RGBA(*this, 1024, 64, 0x003C, DXT3_DXT5_SPACE_SHIFT, bmoTextureProcessor::decompressDXT5RGBA),
    cacheLATC1(*this, 1024, 64, 0x003F, LATC1_LATC2_SPACE_SHIFT, bmoTextureProcessor::decompressLATC1),
    cacheLATC1_SIGNED(*this, 1024, 64, 0x003F, LATC1_LATC2_SPACE_SHIFT, bmoTextureProcessor::decompressLATC1Signed),
    cacheLATC2(*this, 1024, 64, 0x003E, LATC1_LATC2_SPACE_SHIFT, bmoTextureProcessor::decompressLATC2),
    cacheLATC2_SIGNED(*this, 1024, 64, 0x003E, LATC1_LATC2_SPACE_SHIFT, bmoTextureProcessor::decompressLATC2Signed)
{
    //  Create and initialize a rasterizer behaviorModel for Rasterizer.
    bmRaster = new bmoRasterizer(
        1,                              //  Active triangles.  
        MAX_FRAGMENT_ATTRIBUTES,        //  Attributes per fragment.  
        ArchConf.ras.scanWidth,             //  Scan tile width in fragments.  
        ArchConf.ras.scanHeight,            //  Scan tile height in fragments.  
        ArchConf.ras.overScanWidth,         //  Over scan tile width in scan tiles.  
        ArchConf.ras.overScanHeight,        //  Over scan tile height in scan tiles.  
        ArchConf.ras.genWidth,              //  Generation tile width in fragments.  
        ArchConf.ras.genHeight,             //  Generation tile width in fragments.  
        ArchConf.ras.useBBOptimization,     //  Use the Bounding cmoMduBase optimization pass (micropolygon rasterizer).  
        ArchConf.ras.subPixelPrecision      //  Precision bits for the decimal part of subpixel operations (micropolygon rasterizer).  
        );
    CG_ASSERT_COND((bmRaster != NULL), "Error creating rasterizer behaviorModel object.");

    //  Create texture behaviorModel.
    bmTexture = new bmoTextureProcessor(
        STAMP_FRAGMENTS,                //  Fragments per stamp.  
        ArchConf.ush.texBlockDim,          //  Texture block dimension (texels): 2^n x 2^n.  
        ArchConf.ush.texSuperBlockDim,         //  Texture superblock dimension (blocks): 2^m x 2^m.  
        ArchConf.ush.anisoAlgo,             //  Anisotropy algorithm selected.  
        ArchConf.ush.forceMaxAniso,         //  Force the maximum anisotropy from the configuration file for all textures.  
        ArchConf.ush.maxAnisotropy,         //  Maximum anisotropy allowed for any texture.  
        ArchConf.ush.triPrecision,          //  Trilinear precision.  
        ArchConf.ush.briThreshold,          //  Brilinear threshold.  
        ArchConf.ush.anisoRoundPrec,        //  Aniso ratio rounding precision.  
        ArchConf.ush.anisoRoundThres,       //  Aniso ratio rounding threshold.  
        ArchConf.ush.anisoRatioMultOf2,     //  Aniso ratio must be multiple of two.  
        ArchConf.ras.overScanWidth,         //  Over scan tile width (scan tiles).  
        ArchConf.ras.overScanHeight,        //  Over scan tile height (scan tiles).  
        ArchConf.ras.scanWidth,             //  Scan tile width (pixels).  
        ArchConf.ras.scanHeight,            //  Scan tile height (pixels).  
        ArchConf.ras.genWidth,              //  Generation tile width (pixels).  
        ArchConf.ras.genHeight              //  Generation tile height (pixels).  
        );
    CG_ASSERT_COND((bmTexture != NULL), "Error creating texture behaviorModel object.");

    //  Create and initialize shader behaviorModel.
    bmShader = new bmoUnifiedShader(
        "bmoUnifiedShader",                                    //  Shader name.
        UNIFIED,                                        //  Shader model.
        CG_BEHV_MODEL,
        STAMP_FRAGMENTS,                                //  Threads supported by the shader.
        true,                                           //  Store decoded instructions.
        STAMP_FRAGMENTS,                                //  Fragments per stamp for texture accesses.
        ArchConf.ras.subPixelPrecision                      //  subpixel precision for shader fixed point operations.
        );
    CG_ASSERT_COND( (bmShader != NULL), "Error creating shader behaviorModel object.");    
    bmShader->SetTP(bmTexture);


    //  Create fragment operations behaviorModel.
    bmFragOp = new bmoFragmentOperator(
        STAMP_FRAGMENTS                 //  Fragments per stamp.
        );
    CG_ASSERT_COND( (bmFragOp != NULL), "Error creating fragment operations behaviorModel object.");

    //  Allocate memory.
    gpuMemory = new U08[ArchConf.mem.memSize * 1024 * 1024];
    sysMemory = new U08[ArchConf.mem.mappedMemSize * 1024 * 1024];
    //  Check allocation.
    CG_ASSERT_COND((gpuMemory != NULL), "Error allocating gpu memory.");
    CG_ASSERT_COND((sysMemory != NULL), "Error allocating system memory.");

    //  Reset content of the gpu memory.
    for(U32 dw = 0; dw < ((ArchConf.mem.memSize * 1024 * 1024) >> 2); dw++)
        ((U32 *) gpuMemory)[dw] = 0xDEADCAFE;
    //  Reset content of the system mapped memory.
    for(U32 dw = 0; dw < ((ArchConf.mem.mappedMemSize * 1024 * 1024) >> 2); dw++)
        ((U32 *) sysMemory)[dw] = 0xDEADCAFE;
    //  Set frame counter as start frame.
    frameCounter = ArchConf.sim.startFrame;
    //  Reset the counter of draw calls processed.
    batchCounter = 0;
    //  Reset the validation mode flag.
    validationMode = false;
    //  Reset skip draw call mode flag.
    skipBatchMode = false;
    
    //  Initialize debug log variables.
    traceLog = false;
    traceBatch = false;
    traceVertex = false;
    tracePixel = false;
    traceVShader = false;
    traceFShader = false;
    traceTexture = false;
    watchBatch = 0;
    watchPixelX = 0;
    watchPixelY = 0;
    watchIndex = 0;
}

//
//  Implementation of the ShadedVertex container class.
//

bmoGpuTop::ShadedVertex::ShadedVertex(Vec4FP32 *attrib)
{
    for(U32 a = 0; a < MAX_VERTEX_ATTRIBUTES; a++)
    {
        attributes[a][0] = attrib[a][0];
        attributes[a][1] = attrib[a][1];
        attributes[a][2] = attrib[a][2];
        attributes[a][3] = attrib[a][3];
    }
}

Vec4FP32 *bmoGpuTop::ShadedVertex::getAttributes()
{
    return attributes;
}

//
//  Implementation of the ShadedFragment container class.
//


bmoGpuTop::ShadedFragment::ShadedFragment(Fragment *fr, bool _culled)
{
    fragment = fr;
    culled = _culled;
}

Fragment *bmoGpuTop::ShadedFragment::getFragment()
{
    return fragment;
}


Vec4FP32 *bmoGpuTop::ShadedFragment::getAttributes()
{
    return attributes;
}

bool bmoGpuTop::ShadedFragment::isCulled()
{
    return culled;
}

void bmoGpuTop::ShadedFragment::setAsCulled()
{
    culled = true;
}


//
//  Implementation of the CompressedTextureCache helper class.
//
bmoGpuTop::CompressedTextureCache::CompressedTextureCache(bmoGpuTop &bm, U32 blocks, U32 blockSize, U64 blockMask, U32 ratioShift,
                                                            void (*decompFunc) (U08 *, U08 *, U32)) :
    bm(bm)
{
    decompressedBlockSize = blockSize;
    maxCachedBlocks = blocks;
    compressionRatioShift = ratioShift;
    decompressedBlockMask = blockMask;
    decompressionFunction = decompFunc;

    //  Allocate space for the decompressed texture data.
    decompressedData = new U08[decompressedBlockSize * maxCachedBlocks];

    //  Clear compressed texture cache data.
    cachedBlocks = 0;
    blockCache.clear();
}

void bmoGpuTop::CompressedTextureCache::readData(U64 address, U08 *data, U32 size)
{
    //  Search block in the block cache.
    U64 blockAddress = address & ~(decompressedBlockMask);
    map<U64, U08 *>::iterator it;
    it = blockCache.find(blockAddress);

    //  Check if decompressed block is in the cache.
    if (it != blockCache.end())
    {
        //  Copy decompressed data.
        for(U32 b = 0; b < size; b++)
            data[b] = (it->second)[(address & decompressedBlockMask) + b];
    }
    else
    {
        //  Calculate address of the compressed data in memory.
        U32 comprAddress = U32((blockAddress >> compressionRatioShift) & 0xffffffff);

        //  Check if block cache is full.
        if (cachedBlocks == maxCachedBlocks)
        {
            //  Clear the cache.
            cachedBlocks = 0;
            blockCache.clear();
        }

        U08 *memory = bm.selectMemorySpace(comprAddress);
        comprAddress = comprAddress & SPACE_ADDRESS_MASK;
        
        //  Decompress into the next free cached block.
        decompressionFunction(&memory[comprAddress], &decompressedData[cachedBlocks * decompressedBlockSize],
            decompressedBlockSize >> compressionRatioShift);

        //  Copy decompressed data.
        for(U32 b = 0; b < size; b++)
            data[b] = decompressedData[cachedBlocks * decompressedBlockSize + (address & decompressedBlockMask) + b];

        //  Add to the block cache.
        blockCache.insert(make_pair(blockAddress, &decompressedData[cachedBlocks * decompressedBlockSize]));
        cachedBlocks++;
    }
}

void bmoGpuTop::CompressedTextureCache::clear()
{
    //  Clear the cache.
    cachedBlocks = 0;
    blockCache.clear();
}
//  Emulates the Command Processor.
void bmoGpuTop::emulateCommandProcessor(cgoMetaStream *CurMetaStream)
{
    char filename[1024];
    bool rtEnabled;
    //  Process the current MetaStream.
    switch(CurMetaStream->GetMetaStreamType())
    {
        case META_STREAM_WRITE:
            {
                U32 address = CurMetaStream->getAddress(); //  Get address and size of the write.
                U32 size = CurMetaStream->getSize();
                CG_INFO("META_STREAM_WRITE address %08x size %d", address, size);
                if ((address & ADDRESS_SPACE_MASK) == GPU_ADDRESS_SPACE) //  Check memory space for the write.
                {
                    address = address & SPACE_ADDRESS_MASK; //  Get address inside the memory space.
                    CG_ASSERT_COND( ((address + size) < (ArchConf.mem.memSize * 1024 * 1024)), "META_STREAM_WRITE out of memory."); //  Check address overflow.
                    memcpy(&gpuMemory[address], CurMetaStream->getData(), size); //  Perform the write.
                }
                else if ((address & ADDRESS_SPACE_MASK) == SYSTEM_ADDRESS_SPACE)
                {
                    address = address & SPACE_ADDRESS_MASK; //  Get address inside the memory space.
                    CG_ASSERT_COND( ((address + size) < (ArchConf.mem.mappedMemSize * 1024 * 1024)), "META_STREAM_WRITE out of memory."); //  Check address overflow.
                    memcpy(&sysMemory[address], CurMetaStream->getData(), size); //  Perform the write.
                }
            }
            delete CurMetaStream;
            break;
        case META_STREAM_PRELOAD:
            {
                U32 address = CurMetaStream->getAddress(); //  Get address and size of the write.
                U32 size = CurMetaStream->getSize();
                CG_INFO("META_STREAM_PRELOAD address %08x size %d", address, size);
                if ((address & ADDRESS_SPACE_MASK) == GPU_ADDRESS_SPACE) //  Check memory space for the write.
                {
                    address = address & SPACE_ADDRESS_MASK; //  Get address inside the memory space.
                    CG_ASSERT_COND(((address + size) < (ArchConf.mem.memSize * 1024 * 1024)), "META_STREAM_PRELOAD out of memory."); //  Check address overflow.
                    memcpy(&gpuMemory[address], CurMetaStream->getData(), size); //  Perform the write.
                }
                if ((address & ADDRESS_SPACE_MASK) == SYSTEM_ADDRESS_SPACE)
                {
                    address = address & SPACE_ADDRESS_MASK; //  Get address inside the memory space.
                    CG_ASSERT_COND( ((address + size) < (ArchConf.mem.mappedMemSize * 1024 * 1024)), "META_STREAM_PRELOAD out of memory."); //  Check address overflow.
                    memcpy(&sysMemory[address], CurMetaStream->getData(), size); //  Perform the write.
                }
            }
            delete CurMetaStream;
            break;

        case META_STREAM_READ:
            CG_ASSERT("META_STREAM_READ not implemented.");
            break;

        case META_STREAM_REG_WRITE:
            processRegisterWrite(CurMetaStream->getGPURegister(),
                                 CurMetaStream->getGPUSubRegister(),
                                 CurMetaStream->getGPURegData());
            delete CurMetaStream;
            break;

        case META_STREAM_REG_READ:
            CG_ASSERT("META_STREAM_REG_READ not implemented.");
            break;

        case META_STREAM_COMMAND:
            switch(CurMetaStream->getGPUCommand())
            {
                case GPU_RESET:
                    CG_INFO( "GPU_RESET command received.");
                    resetState();
                    delete CurMetaStream;
                    break;

                case GPU_DRAW:
                    CG_INFO( "GPU_DRAW command received.");
                    //  Found command to process.
                //if (batchCounter == watchBatch)
                //{
                    draw();

#ifdef DUMP_RT_AFTER_DRAW
                    CG_INFO("Rendering frame %d batch %d trace position %d", frameCounter, batchCounter, TraceDriver->getTracePosition());
                    rtEnabled = false;
                    for(U32 rt = 0; rt < MAX_RENDER_TARGETS; rt++)
                    {
                        //  Check if color write is enabled in the current batch.
                        if (state.rtEnable[rt] && (state.colorMaskR[rt] || state.colorMaskG[rt] || state.colorMaskB[rt] || state.colorMaskA[rt]))
                        {
                            sprintf(filename, "frame%04d-batch%05d-pos%08d-rt%02d.bm", frameCounter, batchCounter, TraceDriver->getTracePosition(), rt);
                            dumpFrame(filename, rt, false);
                            
                            rtEnabled = true;
                        }
                    }
                    
                    if (!rtEnabled)
                    {
                        sprintf(filename, "depth%04d-batch%05d-pos%08d.bm", frameCounter, batchCounter, TraceDriver->getTracePosition());
                        dumpDepthBuffer(filename);
                    }
#endif
                //}
                
                    CG1_BMDL_TRACE(
                        if (traceLog || (traceBatch && (batchCounter == watchBatch)))
                        {
                            sprintf(filename, "alpha%04d-batch%05d-pos%08d.bm", frameCounter, batchCounter, TraceDriver->getTracePosition());
                            dumpFrame(filename, 0, true);

                            sprintf(filename, "depth%04d-batch%05d-pos%08d.bm", frameCounter, batchCounter, TraceDriver->getTracePosition());
                            dumpDepthBuffer(filename);

                            sprintf(filename, "stencil%04d-batch%05d-pos%08d.bm", frameCounter, batchCounter, TraceDriver->getTracePosition());
                            dumpStencilBuffer(filename);
                        }
                    )
                    
                    //sprintf(filename, "profile.frame%04d-batch%05d", frameCounter, batchCounter);
                    //GLOBAL_PROFILER_GENERATE_REPORT(filename)
                    batchCounter++;
                    delete CurMetaStream;
                    break;

                case GPU_SWAPBUFFERS:
                    CG_INFO("GPU_SWAPBUFFERS command received.");
                    //  Create current frame filename.
                    sprintf(filename, "frame%04d.bm", frameCounter);
                    dumpFrame(filename, 0);
                    //sprintf(filename, "depth%04.bm", frameCounter);
                    //dumpDepthBuffer(filename);
                    frameCounter++;
                    batchCounter = 0;
                    //  Clean compressed texture caches.
                    cacheDXT1RGB.clear();
                    cacheDXT1RGBA.clear();
                    cacheDXT3RGBA.clear();
                    cacheDXT5RGBA.clear();
                    delete CurMetaStream;
                    break;

                case GPU_BLIT:
                    emulateBlitter();
                    delete CurMetaStream;
                    break;

                case GPU_CLEARZSTENCILBUFFER:
                    CG_INFO("GPU_CLEARZSTENCILBUFFER command received");
                    clearZStencilBuffer();
                    delete CurMetaStream;
                    break;

                case GPU_CLEARCOLORBUFFER:
                    CG_INFO("GPU_CLEARCOLORBUFFER command received");
                    clearColorBuffer();
                    delete CurMetaStream;
                    break;

                case GPU_LOAD_VERTEX_PROGRAM:
                    CG_INFO("GPU_LOAD_VERTEX_PROGRAM command received");
                    loadVertexProgram();
                    delete CurMetaStream;
                    break;

                case GPU_LOAD_FRAGMENT_PROGRAM:
                    CG_INFO("GPU_LOAD_FRAGMENT_PROGRAM command received");
                    loadFragmentProgram();
                    delete CurMetaStream;
                    break;

                case GPU_LOAD_SHADER_PROGRAM:
                    CG_INFO("GPU_LOAD_SHADER_PROGRAM command received");
                    loadShaderProgram();
                    delete CurMetaStream;
                    break;

                case GPU_FLUSHZSTENCIL:
                    CG_INFO("GPU_FLUSHZSTENCIL command ignored.");
                    //  Ignore command.
                    delete CurMetaStream;
                    break;

                case GPU_FLUSHCOLOR:
                    CG_INFO("GPU_FLUSHCOLOR command ignored.");
                    //  Ignore command.
                    delete CurMetaStream;
                    break;

                case GPU_SAVE_COLOR_STATE:
                    CG_INFO("GPU_SAVE_COLOR_STATE command ignored.");
                    //  Ignore command.
                    delete CurMetaStream;
                    break;

                case GPU_RESTORE_COLOR_STATE:
                    CG_INFO("GPU_RESTORE_COLOR_STATE command ignored.");
                    //  Ignore command.
                    delete CurMetaStream;
                    break;

                case GPU_SAVE_ZSTENCIL_STATE:
                    CG_INFO("GPU_SAVE_ZSTENCIL_STATE command ignored.");
                    //  Ignore command.
                    delete CurMetaStream;
                    break;

                case GPU_RESTORE_ZSTENCIL_STATE:
                    CG_INFO("GPU_RESTORE_ZSTENCIL_STATE command ignored.");
                    //  Ignore command.
                    delete CurMetaStream;
                    break;

                case GPU_RESET_COLOR_STATE:
                    CG_INFO("GPU_RESET_COLOR_STATE command ignored.");
                    //  Ignore command.
                    delete CurMetaStream;
                    break;

                case GPU_RESET_ZSTENCIL_STATE:
                    CG_INFO("GPU_RESET_ZSTENCIL_STATE command ignored.");
                    //  Ignore command.
                    delete CurMetaStream;
                    break;

                case GPU_CLEARBUFFERS:

                    CG_ASSERT("GPU_CLEARBUFFERS command not implemented.");
                    break;

                case GPU_CLEARZBUFFER:

                    CG_ASSERT("GPU_CLEARZBUFFER command not implemented.");
                    break;

                default:
                    CG_ASSERT("GPU command not supported.");
                    break;
            }
            break;

        case META_STREAM_INIT_END:
            CG_INFO("META_STREAM_INIT_END ignored.");
            //  Ignore this transaction.
            delete CurMetaStream;
            break;

        case META_STREAM_EVENT:
            CG_INFO("META_STREAM_EVENT ignored.");
            //  Ignore this transaction.
            delete CurMetaStream;
            break;

        default:
            CG_ASSERT("Unsupported agp command.");
            break;
    }
}

//  Process a GPU register write.  
void bmoGpuTop::processRegisterWrite(GPURegister gpuReg, U32 gpuSubReg, GPURegData gpuData)
{
    U32 textUnit;
    U32 mipmap;
    U32 cubemap;

    //  Write request to a GPU register:
    //   *  Check that the MetaStream can be processed.
    //   *  Write value to GPU register.
    //   *  Issue the state change to other GPU units.
    switch (gpuReg)
    {
        //  GPU state registers.
        case GPU_STATUS: //  Read Only register.
            CG_ASSERT("Error writing to GPU status register not allowed.");
            break;
        //  GPU display registers.
        case GPU_DISPLAY_X_RES:
            CG_INFO("Write GPU_DISPLAY_X_RES = %d.", gpuData.uintVal); 
            //  Check x resolution range.
            GPU_ASSERT_COND( (gpuData.uintVal > MAX_DISPLAY_RES_X), ( "Horizontal display resolution not supported."); )
            //  Set GPU display x resolution register.
            state.displayResX = gpuData.uintVal;
            break;

        case GPU_DISPLAY_Y_RES:
            CG_INFO("Write GPU_DISPLAY_Y_RES = %d.", gpuData.uintVal);
            //  Check y resolution range.
            GPU_ASSERT_COND( (gpuData.uintVal > MAX_DISPLAY_RES_Y), ( "Vertical display resolution not supported."); )
            //  Set GPU display y resolution register.
            state.displayResY = gpuData.uintVal;
            break;

        case GPU_D3D9_PIXEL_COORDINATES:
            CG_INFO("Write GPU_D3D9_PIXEL_COORDINATES = %s.", gpuData.booleanVal ? "TRUE" : "FALSE"); 
            //  Set GPU use D3D9 pixel coordinates convention register.
            state.d3d9PixelCoordinates = gpuData.booleanVal; 
            break;

        case GPU_VIEWPORT_INI_X:
            CG_INFO("Write GPU_VIEWPORT_INI_X = %d.", gpuData.intVal); 
            //  Check viewport x range.
            CG_ASSERT_COND(!((gpuData.intVal < MIN_VIEWPORT_X) || (gpuData.intVal > MAX_VIEWPORT_X)), "Out of range viewport initial x position.");
            //  Set GPU viewport initial x register.
            state.viewportIniX = gpuData.intVal;

            break;

        case GPU_VIEWPORT_INI_Y:
            CG_INFO("Write GPU_VIEWPORT_INI_Y = %d.", gpuData.intVal);
            //  Check viewport y range.
            CG_ASSERT_COND(!((gpuData.intVal < MIN_VIEWPORT_Y) || (gpuData.intVal > MAX_VIEWPORT_Y)), "Out of range viewport initial y position.");
            //  Set GPU viewport initial y register.
            state.viewportIniY = gpuData.intVal;

            break;

        case GPU_VIEWPORT_HEIGHT:
            CG_INFO("Write GPU_VIEWPORT_HEIGHT = %d.", gpuData.intVal);
            //  Check viewport y range.  
            GPU_ASSERT(
                if(((gpuData.intVal + state.viewportIniY) < MIN_VIEWPORT_Y) ||
                    ((gpuData.intVal + state.viewportIniY) > MAX_VIEWPORT_Y))
                    CG_ASSERT("Out of range viewport final y position.");
            )

            //  Set GPU viewport width register.
            state.viewportHeight = gpuData.uintVal;

            break;

        case GPU_VIEWPORT_WIDTH:
            CG_INFO("Write GPU_VIEWPORT_WIDTH = %d.", gpuData.intVal);
            //  Check viewport x range.  
            GPU_ASSERT(
                if(((gpuData.intVal + state.viewportIniX) < MIN_VIEWPORT_X) ||
                    ((gpuData.intVal + state.viewportIniX) > MAX_VIEWPORT_X))
                    CG_ASSERT("Out of range viewport final x position.");
            )

            //  Set GPU viewport width register.
            state.viewportWidth = gpuData.uintVal;

            break;

        case GPU_DEPTH_RANGE_NEAR:
            CG_INFO("Write GPU_DEPTHRANGE_NEAR = %f.", gpuData.f32Val);
            //  Check depth near range (clamped to [0, 1]).  
            CG_ASSERT_COND(!((gpuData.f32Val < 0.0) || (gpuData.f32Val > 1.0)), "Near depth range must be clamped to 0..1.");
            //  Set GPU near depth range register.
            state.nearRange = gpuData.f32Val;

            break;

        case GPU_DEPTH_RANGE_FAR:

            GPU_DEBUG(
                CG_INFO("Write GPU_DEPTHRANGE_FAR = %f.", gpuData.f32Val);
            )

            //  Check depth far range (clamped to [0, 1]).  
            CG_ASSERT_COND(!((gpuData.f32Val < 0.0) || (gpuData.f32Val > 1.0)), "Far depth range must be clamped to 0..1.");
            //  Set GPU far depth range register.
            state.farRange = gpuData.f32Val;

            break;

        case GPU_D3D9_DEPTH_RANGE:

            GPU_DEBUG(
                CG_INFO("Write GPU_D3D9_DEPTH_RANGE = %s", gpuData.booleanVal ? "T" : "F");
            )

            //  Set GPU use d39 depth range in clip space register.
            state.d3d9DepthRange = gpuData.booleanVal;

            break;

        case GPU_COLOR_BUFFER_CLEAR:

            GPU_DEBUG(
                CG_INFO("Write GPU_COLOR_BUFFER_CLEAR = {%f, %f, %f, %f}.",
                    gpuData.qfVal[0], gpuData.qfVal[1], gpuData.qfVal[2], gpuData.qfVal[3]);
            )

            //  Set GPU color buffer clear value register.
            state.colorBufferClear[0] = gpuData.qfVal[0];
            state.colorBufferClear[1] = gpuData.qfVal[1];
            state.colorBufferClear[2] = gpuData.qfVal[2];
            state.colorBufferClear[3] = gpuData.qfVal[3];

            break;

        case GPU_BLIT_INI_X:

            GPU_DEBUG(
                CG_INFO("Write GPU_BLIT_INI_X = %d.", gpuData.uintVal);
            )

            //  Set GPU blit initial x coordinate register.
            state.blitIniX = gpuData.uintVal;

            break;

        case GPU_BLIT_INI_Y:

            GPU_DEBUG(
                CG_INFO("Write GPU_BLIT_INI_Y = %d.", gpuData.uintVal);
            )

            //  Set GPU blit initial y coordinate register.
            state.blitIniY = gpuData.uintVal;

            break;

        case GPU_BLIT_X_OFFSET:

            GPU_DEBUG(
                CG_INFO("Write GPU_BLIT_X_OFFSET = %d.", gpuData.uintVal);
            )

            //  Set GPU blit x offset coordinate register.
            state.blitXOffset = gpuData.uintVal;

            break;

        case GPU_BLIT_Y_OFFSET:

            GPU_DEBUG(
                CG_INFO("Write GPU_BLIT_Y_OFFSET = %d.", gpuData.uintVal);
            )

            //  Set GPU blit x offset coordinate register.
            state.blitYOffset = gpuData.uintVal;

            break;

        case GPU_BLIT_WIDTH:

            GPU_DEBUG(
                CG_INFO("Write GPU_BLIT_WIDTH = %d.", gpuData.uintVal);
            )

            //  Set GPU blit width register.
            state.blitWidth = gpuData.uintVal;

            break;

        case GPU_BLIT_HEIGHT:

            GPU_DEBUG(
                CG_INFO("Write GPU_BLIT_HEIGHT = %d.", gpuData.uintVal);
            )

            //  Set GPU blit height register.
            state.blitHeight = gpuData.uintVal;

            break;

        case GPU_BLIT_DST_ADDRESS:

            GPU_DEBUG(
                CG_INFO("Write GPU_BLIT_DST_ADDRESS = %08x.", gpuData.uintVal);
            )

            //  Set GPU blit GPU memory destination address register.
            state.blitDestinationAddress = gpuData.uintVal;

            break;

        case GPU_BLIT_DST_TX_WIDTH2:

            GPU_DEBUG(
                CG_INFO("Write GPU_BLIT_DST_TX_WIDTH2 = %d.", gpuData.txFormat);
            )

            //  Set GPU blit GPU memory destination address register.
            state.blitTextureWidth2 = gpuData.uintVal;

            break;

        case GPU_BLIT_DST_TX_FORMAT:

            GPU_DEBUG(
                CG_INFO("Write GPU_BLIT_DST_TX_FORMAT = %d.", gpuData.txFormat);
            )

            //  Set GPU blit GPU memory destination address register.
            state.blitDestinationTextureFormat = gpuData.txFormat;

            break;

        case GPU_BLIT_DST_TX_BLOCK:

            GPU_DEBUG(
                CG_INFO("Write GPU_BLIT_DST_TX_BLOCK = ", gpuData.txFormat);
                switch(gpuData.txFormat)
                {
                    case GPU_ALPHA8:
                        CG_INFO("GPU_ALPHA8");
                        break;
                    case GPU_ALPHA12:
                        CG_INFO("GPU_ALPHA12");
                        break;

                    case GPU_ALPHA16:
                        CG_INFO("GPU_ALPHA16");
                        break;

                    case GPU_DEPTH_COMPONENT16:
                        CG_INFO("GPU_DEPTH_COMPONENT16");
                        break;

                    case GPU_DEPTH_COMPONENT24:
                        CG_INFO("GPU_DEPTH_COMPONENT24");
                        break;

                    case GPU_DEPTH_COMPONENT32:
                        CG_INFO("GPU_DEPTH_COMPONENT32");
                        break;

                    case GPU_LUMINANCE8:
                        CG_INFO("GPU_LUMINANCE8");
                        break;

                    case GPU_LUMINANCE8_SIGNED:
                        CG_INFO("GPU_LUMINANCE8_SIGNED");
                        break;

                    case GPU_LUMINANCE12:
                        CG_INFO("GPU_LUMINANCE12");
                        break;

                    case GPU_LUMINANCE16:
                        CG_INFO("GPU_LUMINANCE16");
                        break;

                    case GPU_LUMINANCE4_ALPHA4:
                        CG_INFO("GPU_LUMINANCE4_ALPHA4");
                        break;

                    case GPU_LUMINANCE6_ALPHA2:
                        CG_INFO("GPU_LUMINANCE6_ALPHA2");
                        break;

                    case GPU_LUMINANCE8_ALPHA8:
                        CG_INFO("GPU_LUMINANCE8_ALPHA8");
                        break;

                    case GPU_LUMINANCE8_ALPHA8_SIGNED:
                        CG_INFO("GPU_LUMINANCE8_ALPHA8_SIGNED");
                        break;

                    case GPU_LUMINANCE12_ALPHA4:
                        CG_INFO("GPU_LUMINANCE12_ALPHA4");
                        break;

                    case GPU_LUMINANCE12_ALPHA12:
                        CG_INFO("GPU_LUMINANCE12_ALPHA12");
                        break;

                    case GPU_LUMINANCE16_ALPHA16:
                        CG_INFO("GPU_LUMINANCE16_ALPHA16");
                        break;

                    case GPU_INTENSITY8:
                        CG_INFO("GPU_INTENSITY8");
                        break;

                    case GPU_INTENSITY12:
                        CG_INFO("GPU_INTENSITY12");
                        break;

                    case GPU_INTENSITY16:
                        CG_INFO("GPU_INTENSITY16");
                        break;
                    case GPU_RGB332:
                        CG_INFO("GPU_RGB332");
                        break;

                    case GPU_RGB444:
                        CG_INFO("GPU_RGB444");
                        break;

                    case GPU_RGB555:
                        CG_INFO("GPU_RGB555");
                        break;

                    case GPU_RGB565:
                        CG_INFO("GPU_RGB565");
                        break;

                    case GPU_RGB888:
                        CG_INFO("GPU_RGB888");
                        break;

                    case GPU_RGB101010:
                        CG_INFO("GPU_RGB101010");
                        break;
                    case GPU_RGB121212:
                        CG_INFO("GPU_RGB121212");
                        break;

                    case GPU_RGBA2222:
                        CG_INFO("GPU_RGBA2222");
                        break;

                    case GPU_RGBA4444:
                        CG_INFO("GPU_RGBA4444");
                        break;

                    case GPU_RGBA5551:
                        CG_INFO("GPU_RGBA5551");
                        break;

                    case GPU_RGBA8888:
                        CG_INFO("GPU_RGBA8888");
                        break;

                    case GPU_RGBA1010102:
                        CG_INFO("GPU_RGB1010102");
                        break;

                    case GPU_R16:
                        CG_INFO("GPU_R16");
                        break;

                    case GPU_RG16:
                        CG_INFO("GPU_RG16");
                        break;

                    case GPU_RGBA16:
                        CG_INFO("GPU_RGBA16");
                        break;

                    case GPU_R16F:
                        CG_INFO("GPU_R16F");
                        break;

                    case GPU_RG16F:
                        CG_INFO("GPU_RG16F");
                        break;

                    case GPU_RGBA16F:
                        CG_INFO("GPU_RGBA16F");
                        break;

                    case GPU_R32F:
                        CG_INFO("GPU_R32F");
                        break;

                    case GPU_RG32F:
                        CG_INFO("GPU_RG32F");
                        break;

                    case GPU_RGBA32F:
                        CG_INFO("GPU_RGBA32F");
                        break;

                    default:
                        CG_INFO("unknown format with code %0X", gpuSubReg);
                        break;
                }
            )

            //  Set GPU blit GPU memory destination address register.
            state.blitDestinationTextureBlocking = gpuData.txBlocking;

            break;

        case GPU_Z_BUFFER_CLEAR:

            GPU_DEBUG(
                CG_INFO("Write GPU_Z_BUFFER_CLEAR = %08x.", gpuData.uintVal);
            )

            //  Set GPU Z buffer clear value register.
            state.zBufferClear = gpuData.uintVal;

            break;

        case GPU_Z_BUFFER_BIT_PRECISSION:

            GPU_DEBUG(
                CG_INFO("Write GPU_Z_BUFFER_BIT_PRECISSION = %d.", gpuData.uintVal);
            )

            //  Check z buffer bit precission.
            GPU_ASSERT(
                if ((gpuData.uintVal != 16) && (gpuData.uintVal != 24) &&
                    (gpuData.uintVal != 32))
                    CG_ASSERT("Not supported depth buffer bit precission.");
            )

            //  Set GPU Z buffer bit precission register.
            state.zBufferBitPrecission = gpuData.uintVal;

            break;

        case GPU_STENCIL_BUFFER_CLEAR:

            GPU_DEBUG(
                CG_INFO("Write GPU_STENCIL_BUFFER_CLEAR = %02x.", gpuData.uintVal);
            )

            //  Set GPU Z buffer clear value register.
            state.stencilBufferClear = gpuData.uintVal;

            break;

        //  GPU memory registers.
        case GPU_FRONTBUFFER_ADDR:

            GPU_DEBUG(
                CG_INFO("Write GPU_FRONTBUFFER_ADDR = %08x.", gpuData.uintVal);
            )

            //  Check address range.

            //  Set GPU front buffer address in GPU memory register.
            state.frontBufferBaseAddr = gpuData.uintVal;

            break;

        case GPU_BACKBUFFER_ADDR:

            GPU_DEBUG(
                CG_INFO("Write GPU_BACKBUFFER_ADDR = %08x.", gpuData.uintVal);
            )

            //  Check address range.

            //  Set GPU back buffer address in GPU memory register.
            state.backBufferBaseAddr = gpuData.uintVal;

            //  Aliased to render target 0.
            state.rtAddress[0] = gpuData.uintVal;

            break;

        case GPU_ZSTENCILBUFFER_ADDR:

            GPU_DEBUG(
                CG_INFO("Write GPU_ZSTENCILBUFFER_ADDR = %08x.", gpuData.uintVal);
            )

            //  Check address range.

            //  Set GPU Z/Stencil buffer address in GPU memory register.
            state.zStencilBufferBaseAddr = gpuData.uintVal;

            break;

        case GPU_COLOR_STATE_BUFFER_MEM_ADDR:

            GPU_DEBUG(
                CG_INFO("Write GPU_COLOR_STATE_BUFFER_MEM_ADDR = %08x.", gpuData.uintVal);
            )

            //  Check address range.

            //  Set GPU color block state buffer memory address register.
            state.colorStateBufferAddr = gpuData.uintVal;

            break;

        case GPU_ZSTENCIL_STATE_BUFFER_MEM_ADDR:

            GPU_DEBUG(
                CG_INFO("Write GPU_ZSTENCIL_STATE_BUFFER_MEM_ADDR = %08x.", gpuData.uintVal);
            )

            //  Check address range.

            //  Set GPU Z/Stencil block state buffer address in GPU memory register.
            state.zstencilStateBufferAddr = gpuData.uintVal;

            break;

        case GPU_TEXTURE_MEM_ADDR:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_MEM_ADDR = %08x.", gpuData.uintVal);
            )

            //  Check address range.

            //  Set GPU texture memory base address in GPU memory register.
            state.textureMemoryBaseAddr = gpuData.uintVal;

            break;

        case GPU_PROGRAM_MEM_ADDR:

            GPU_DEBUG(
                CG_INFO("Write GPU_PROGRAM_MEM_ADDR = %08x.", gpuData.uintVal);
            )

            //  Check address range.

            //  Set GPU program memory base address in GPU memory register.
            state.programMemoryBaseAddr = gpuData.uintVal;

            break;

        //  GPU vertex shader.
        case GPU_VERTEX_PROGRAM:

            GPU_DEBUG(
                CG_INFO("Write GPU_VERTEX_PROGRAM = %08x.", gpuData.uintVal);
            )

            //  Check address range.

            //  Set vertex program address register.
            state.vertexProgramAddr = gpuData.uintVal;

            break;

        case GPU_VERTEX_PROGRAM_PC:

            GPU_DEBUG(
                CG_INFO("Write GPU_VERTEX_PROGRAM_PC = %04x.", gpuData.uintVal);
            )

            //  Check vertex program PC range.

            //  Set vertex program start PC register.
            state.vertexProgramStartPC = gpuData.uintVal;

            break;

        case GPU_VERTEX_PROGRAM_SIZE:

            GPU_DEBUG(
                CG_INFO("Write GPU_VERTEX_PROGRAM_SIZE = %d.", gpuData.uintVal);
            )

            //  Set vertex program to load size (instructions).
            state.vertexProgramSize = gpuData.uintVal;

            break;

        case GPU_VERTEX_THREAD_RESOURCES:

            GPU_DEBUG(
                CG_INFO("Write GPU_VERTEX_THREAD_RESOURCES = %d.", gpuData.uintVal);
            )

            //  Set vertex program to load size (instructions).
            state.vertexThreadResources = gpuData.uintVal;

            break;

        case GPU_VERTEX_CONSTANT:

            GPU_DEBUG(
                CG_INFO("Write GPU_VERTEX_CONSTANT[%d] = {%f, %f, %f, %f}.", gpuSubReg,
                    gpuData.qfVal[0], gpuData.qfVal[1], gpuData.qfVal[2], gpuData.qfVal[3]);
            )

            //  Check constant range.
            CG_ASSERT_COND(!(gpuSubReg >= MAX_VERTEX_CONSTANTS), "Out of range vertex constant register.");
            //  Set vertex constant register.
            state.vConstants[gpuSubReg].setComponents(gpuData.qfVal[0],
                gpuData.qfVal[1], gpuData.qfVal[2], gpuData.qfVal[3]);

            //  Load vertex constant in the shader behaviorModel.
            bmShader->loadShaderState(0, cg1gpu::PARAM, gpuSubReg + VERTEX_PARTITION * UNIFIED_CONSTANT_NUM_REGS, state.vConstants[gpuSubReg]);

            break;

        case GPU_VERTEX_OUTPUT_ATTRIBUTE:

            GPU_DEBUG(
                CG_INFO("Write GPU_VERTEX_OUTPUT_ATTRIBUTE[%d] = %s.",
                    gpuSubReg, gpuData.booleanVal?"ENABLED":"DISABLED");
            )

            //  Check vertex attribute identifier range.
            CG_ASSERT_COND(!(gpuSubReg >= MAX_VERTEX_ATTRIBUTES), "Out of range vertex attribute identifier.");
            //  Set GPU vertex attribute mapping register.
            state.outputAttribute[gpuSubReg] = gpuData.booleanVal;

            break;

        //  GPU vertex stream buffer registers.
        case GPU_VERTEX_ATTRIBUTE_MAP:

            GPU_DEBUG(
                CG_INFO("Write GPU_VERTEX_ATTRIBUTE_MAP[%d] = %d.", gpuSubReg, gpuData.uintVal);
            )

            //  Check vertex attribute identifier range.
            CG_ASSERT_COND(!(gpuSubReg >= MAX_VERTEX_ATTRIBUTES), "Out of range vertex attribute identifier.");
            //  Set GPU vertex attribute mapping register.
            state.attributeMap[gpuSubReg] = gpuData.uintVal;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_VERTEX_ATTRIBUTE_DEFAULT_VALUE:

            GPU_DEBUG(
                CG_INFO("Write GPU_VERTEX_ATTRIBUTE_DEFAULT_VALUE[%d] = {%f, %f, %f, %f}.", gpuSubReg,
                    gpuData.qfVal[0], gpuData.qfVal[1], gpuData.qfVal[2], gpuData.qfVal[3]);
            )

            //  Check vertex attribute identifier range.
            CG_ASSERT_COND(!(gpuSubReg >= MAX_VERTEX_ATTRIBUTES), "Out of range vertex attribute identifier.");
            //  Set GPU vertex attribute default value.
            state.attrDefValue[gpuSubReg][0] = gpuData.qfVal[0];
            state.attrDefValue[gpuSubReg][1] = gpuData.qfVal[1];
            state.attrDefValue[gpuSubReg][2] = gpuData.qfVal[2];
            state.attrDefValue[gpuSubReg][3] = gpuData.qfVal[3];

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_STREAM_ADDRESS:

            GPU_DEBUG(
                CG_INFO("Write GPU_STREAM_ADDRESS[%d] = %08x.", gpuSubReg, gpuData.uintVal);
            )

            //  Check stream identifier range.
            CG_ASSERT_COND(!(gpuSubReg >= MAX_STREAM_BUFFERS), "Out of range stream buffer identifier.");
            //  Set GPU stream buffer address register.
            state.streamAddress[gpuSubReg] = gpuData.uintVal;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_STREAM_STRIDE:

            GPU_DEBUG(
                CG_INFO("Write GPU_STREAM_STRIDE[%d] = %d.", gpuSubReg, gpuData.uintVal);
            )

            //  Check stream identifier range.
            CG_ASSERT_COND(!(gpuSubReg >= MAX_STREAM_BUFFERS), "Out of range stream buffer identifier.");
            //  Set GPU stream buffer stride register.
            state.streamStride[gpuSubReg] = gpuData.uintVal;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_STREAM_DATA:

            GPU_DEBUG(
                CG_INFO("Write GPU_STREAM_DATA[%d] = ", gpuSubReg);
                switch(gpuData.streamData)
                {
                    case SD_UNORM8:
                        CG_INFO("SD_UNORM8.");
                        break;
                    case SD_SNORM8:
                        CG_INFO("SD_SNORM8.");
                        break;
                    case SD_UNORM16:
                        CG_INFO("SD_UNORM16.");
                        break;
                    case SD_SNORM16:
                        CG_INFO("SD_SNORM16.");
                        break;
                    case SD_UNORM32:
                        CG_INFO("SD_UNORM32.");
                        break;
                    case SD_SNORM32:
                        CG_INFO("SD_SNORM32.");
                        break;
                    case SD_FLOAT16:
                        CG_INFO("SD_FLOAT16.");
                        break;
                    case SD_FLOAT32:
                        CG_INFO("SD_FLOAT32.");
                        break;
                    case SD_UINT8:
                        CG_INFO("SD_UINT8.");
                        break;
                    case SD_SINT8:
                        CG_INFO("SD_SINT8.");
                        break;
                    case SD_UINT16:
                        CG_INFO("SD_UINT16.");
                        break;
                    case SD_SINT16:
                        CG_INFO("SD_SINT16.");
                        break;
                    case SD_UINT32:
                        CG_INFO("SD_UINT32.");
                        break;
                    case SD_SINT32:
                        CG_INFO("SD_SINT32.");
                        break;
                    default:
                        CG_ASSERT("Undefined format.");
                        break;
                }
            )

            //  Check stream identifier range.
            CG_ASSERT_COND(!(gpuSubReg >= MAX_STREAM_BUFFERS), "Out of range stream buffer identifier.");
            //  Set GPU stream buffer data type register.
            state.streamData[gpuSubReg] = gpuData.streamData;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_STREAM_ELEMENTS:

            GPU_DEBUG(
                CG_INFO("Write GPU_STREAM_ELEMENTS[%d] = %d.", gpuSubReg, gpuData.uintVal);
            )

            //  Check stream identifier range.
            CG_ASSERT_COND(!(gpuSubReg >= MAX_STREAM_BUFFERS), "Out of range stream buffer identifier.");
            //  Set GPU stream buffer number of elements per entry register.
            state.streamElements[gpuSubReg] = gpuData.uintVal;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_STREAM_FREQUENCY:

            GPU_DEBUG(
                CG_INFO("Write GPU_STREAM_FREQUENCY[%d] = %d.", gpuSubReg, gpuData.uintVal);
            )

            //  Check stream identifier range.
            CG_ASSERT_COND(!(gpuSubReg >= MAX_STREAM_BUFFERS), "Out of range stream buffer identifier.");
            //  Set GPU stream buffer read frequency register.
            state.streamFrequency[gpuSubReg] = gpuData.uintVal;

            //  Write register in the Texture Behavior Model.
            //bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_STREAM_START:

            GPU_DEBUG(
                CG_INFO("Write GPU_STREAM_START = %d.", gpuData.uintVal);
            )

            //  Set GPU stream start position register.
            state.streamStart = gpuData.uintVal;

            break;

        case GPU_STREAM_COUNT:

            GPU_DEBUG(
                CG_INFO("Write GPU_STREAM_COUNT = %d.", gpuData.uintVal);
            )

            //  Set GPU stream count register.
            state.streamCount = gpuData.uintVal;

            break;

        case GPU_STREAM_INSTANCES:

            GPU_DEBUG(
                CG_INFO("Write GPU_STREAM_INSTANCES = %d.", gpuData.uintVal);
            )

            //  Set GPU stream instances register.
            state.streamInstances = gpuData.uintVal;

            break;

        case GPU_INDEX_MODE:

            GPU_DEBUG(
                CG_INFO("Write GPU_INDEX_MODE = %s.", gpuData.booleanVal ? "ENABLED" : "DISABLED");
            )

            //  Set GPU indexed batching mode register.
            state.indexedMode = gpuData.booleanVal;

            break;

        case GPU_INDEX_STREAM:

            GPU_DEBUG(
                CG_INFO("Write GPU_INDEX_STREAM = %d.", gpuData.uintVal);
            )

            //  Check stream identifier range.
            CG_ASSERT_COND(!(gpuData.uintVal >= MAX_STREAM_BUFFERS), "Out of range stream buffer identifier.");
            //  Set GPU stream buffer for index buffer register.
            state.indexStream = gpuData.uintVal;

            break;

        case GPU_D3D9_COLOR_STREAM:

            GPU_DEBUG(
                CG_INFO("Write GPU_D3D9_COLOR_STREAM[%d] = %s.", gpuSubReg, gpuData.booleanVal ? "T" : "F");
            )

            //  Check stream identifier range.
            CG_ASSERT_COND(!(gpuSubReg >= MAX_STREAM_BUFFERS), "Out of range stream buffer identifier.");
            //  Set GPU D3D9 color stream register.
            state.d3d9ColorStream[gpuSubReg] = gpuData.booleanVal;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_ATTRIBUTE_LOAD_BYPASS:

            GPU_DEBUG(
                CG_INFO("Write GPU_ATTRIBUTE_LOAD_BYPASS = %s.", gpuData.booleanVal ? "T" : "F");
            )

            //  Set attribute load bypass register.
            state.attributeLoadBypass = gpuData.booleanVal;

            break;

        //  GPU primitive assembly registers.

        case GPU_PRIMITIVE:

            GPU_DEBUG(
                CG_INFO("Write GPU_PRIMITIVE = ");
                switch(gpuData.primitive)
                {
                    case cg1gpu::TRIANGLE:
                        CG_INFO("TRIANGLE.");
                        break;
                    case cg1gpu::TRIANGLE_STRIP:
                        CG_INFO("TRIANGLE_STRIP.");
                        break;
                    case cg1gpu::TRIANGLE_FAN:
                        CG_INFO("TRIANGLE_FAN.");
                        break;
                    case cg1gpu::QUAD:
                        CG_INFO("QUAD.");
                        break;
                    case cg1gpu::QUAD_STRIP:
                        CG_INFO("QUAD_STRIP.");
                        break;
                    case cg1gpu::LINE:
                        CG_INFO("LINE.");
                        break;
                    case cg1gpu::LINE_STRIP:
                        CG_INFO("LINE_STRIP.");
                        break;
                    case cg1gpu::LINE_FAN:
                        CG_INFO("LINE_FAN.");
                        break;
                    case cg1gpu::POINT:
                        CG_INFO("POINT.");
                        break;
                    default:
                        CG_ASSERT("Unsupported primitive mode.");
                        break;
                }
            )

            //  Set GPU primitive mode register.
            state.primitiveMode = gpuData.primitive;

            break;


        //  GPU clipping and culling registers.

        case GPU_FRUSTUM_CLIPPING:

            GPU_DEBUG(
                CG_INFO("Write GPU_FRUSTUM_CLIPPING = %s.", gpuData.booleanVal ? "T":"F");
            )

            //  Set GPU frustum clipping flag register.
            state.frustumClipping = gpuData.booleanVal;

            break;

        case GPU_USER_CLIP:

            GPU_DEBUG(
                CG_INFO("Write GPU_USER_CLIP[%d] = {%f, %f, %f, %f}.", gpuSubReg,
                    gpuData.qfVal[0], gpuData.qfVal[1], gpuData.qfVal[2], gpuData.qfVal[3]);
            )

            //  Check user clip plane identifier range.
            CG_ASSERT_COND(!(gpuSubReg >= MAX_USER_CLIP_PLANES), "Out of range user clip plane identifier.");
            //  Set GPU user clip plane.
            state.userClip[gpuSubReg].setComponents(gpuData.qfVal[0],
                gpuData.qfVal[1], gpuData.qfVal[2], gpuData.qfVal[3]);

            break;

        case GPU_USER_CLIP_PLANE:

            GPU_DEBUG(
                CG_INFO("Write GPU_USER_CLIP_PLANE = %s.", gpuData.booleanVal ? "T" : "F");
            )

            //  Set GPU user clip planes enable register.
            state.userClipPlanes = gpuData.booleanVal;

            break;

        case GPU_FACEMODE:

            GPU_DEBUG(
                CG_INFO("Write GPU_FACEMODE = ");
                switch(gpuData.culling)
                {
                    case GPU_CW:
                        CG_INFO("CW.");
                        break;

                    case GPU_CCW:
                        CG_INFO("CCW.");
                        break;
                }
            )

            //  Set GPU face mode (vertex order for front facing triangles) register.
            state.faceMode = gpuData.faceMode;

            break;

        case GPU_CULLING:

            GPU_DEBUG(
                CG_INFO("Write GPU_CULLING = ");
                switch(gpuData.culling)
                {
                    case NONE:
                        CG_INFO("NONE.");
                        break;

                    case FRONT:
                        CG_INFO("FRONT.");
                        break;

                    case BACK:
                        CG_INFO("BACK.");
                        break;

                    case FRONT_AND_BACK:
                        CG_INFO("FRONT_AND_BACK.");
                        break;

                }
            )

            //  Set GPU culling mode (none, front, back, front/back) register.
            state.cullMode = gpuData.culling;

            break;

        //  Hierarchical Z registers.
        case GPU_HIERARCHICALZ:

            GPU_DEBUG(
                CG_INFO("Write GPU_HIERARCHICALZ = %s.", gpuData.booleanVal ? "T" : "F");
            )

            //  Set GPU hierarchical Z enable flag register.
            state.hzTest = gpuData.booleanVal;

            break;

        //  Hierarchical Z registers.
        case GPU_EARLYZ:

            GPU_DEBUG(
                CG_INFO("Write GPU_EARLYZ = %s.", gpuData.booleanVal ? "T" : "F");
            )

            //  Set GPU early Z test enable flag register.
            state.earlyZ = gpuData.booleanVal;

            break;

        //  GPU rasterization registers.

        case GPU_SCISSOR_TEST:

            GPU_DEBUG(
                CG_INFO("Write GPU_SCISSOR_TEST = %s.", gpuData.booleanVal ? "T" : "F");
            )

            //  Set GPU scissor test enable register.
            state.scissorTest = gpuData.booleanVal;

            break;

        case GPU_SCISSOR_INI_X:

            GPU_DEBUG(
                CG_INFO("Write GPU_SCISSOR_INI_X = %d.", gpuData.intVal);
            )

            //  Set GPU scissor initial x register.
            state.scissorIniX = gpuData.intVal;

            break;

        case GPU_SCISSOR_INI_Y:

            GPU_DEBUG(
                CG_INFO("Write GPU_SCISSOR_INI_Y = %d.", gpuData.intVal);
            )

            //  Set GPU scissor initial y register.
            state.scissorIniY = gpuData.intVal;

            break;

        case GPU_SCISSOR_WIDTH:

            GPU_DEBUG(
                CG_INFO("Write GPU_SCISSOR_WIDTH = %d.", gpuData.uintVal);
            )

            //  Set GPU scissor width register.
            state.scissorWidth = gpuData.uintVal;

            break;

        case GPU_SCISSOR_HEIGHT:

            GPU_DEBUG(
                CG_INFO("Write GPU_SCISSOR_HEIGHT = %d.", gpuData.uintVal);
            )

            //  Set GPU scissor height register.
            state.scissorHeight = gpuData.uintVal;

            break;

        case GPU_DEPTH_SLOPE_FACTOR:

            GPU_DEBUG(
                CG_INFO("Write GPU_DEPTH_SLOPE_FACTOR = %f.", gpuData.f32Val);
            )

            //  Set GPU depth slope factor.
            state.slopeFactor = gpuData.f32Val;

            break;

        case GPU_DEPTH_UNIT_OFFSET:

            GPU_DEBUG(
                CG_INFO("Write GPU_DEPTH_UNIT_OFFSET = %f.", gpuData.f32Val);
            )

            //  Set GPU depth unit offset.
            state.unitOffset = gpuData.f32Val;

            break;

        case GPU_D3D9_RASTERIZATION_RULES:

            GPU_DEBUG(
                CG_INFO("Write GPU_D3D9_RASTERIZATION_RULES = %s", gpuData.booleanVal ? "T" : "F");
            )

            //  Set GPU use d39 rasterization rules.
            state.d3d9RasterizationRules = gpuData.booleanVal;

            break;

        case GPU_TWOSIDED_LIGHTING:

            GPU_DEBUG(
                CG_INFO("Write GPU_TWOSIDED_LIGHTING = %s.", gpuData.booleanVal ? "T" : "F");
            )

            //  Set GPU two sided lighting color selection register.
            state.twoSidedLighting = gpuData.booleanVal;

            break;

        case GPU_MULTISAMPLING:

            GPU_DEBUG(
                CG_INFO("Write GPU_MULTISAMPLING = %s.", gpuData.booleanVal ? "T" : "F");
            )

            //  Set multisampling (MSAA) enabling register.
            state.multiSampling = gpuData.booleanVal;

            break;

        case GPU_MSAA_SAMPLES:

            GPU_DEBUG(
                CG_INFO("Write GPU_MSAA_SAMPLES = %d.", gpuData.uintVal);
            )

            //  Set GPU number of z samples to generate per fragment for MSAA register.
            state.msaaSamples = gpuData.uintVal;

            break;

        case GPU_INTERPOLATION:

            GPU_DEBUG(
                CG_INFO("Write GPU_INTERPOLATION = %s.", gpuData.booleanVal ? "T" : "F");
            )

            //  Set GPU fragments attribute interpolation mode (GL flat/smoth) register.
            state.interpolation[gpuSubReg] = gpuData.booleanVal;

            break;

        case GPU_FRAGMENT_INPUT_ATTRIBUTES:

            GPU_DEBUG(
                CG_INFO("Write GPU_FRAGMENT_INPUT_ATTRIBUTES[%d] = %s.",
                    gpuSubReg, gpuData.booleanVal?"ENABLED":"DISABLED");
            )

            //  Check fragment attribute range.
            CG_ASSERT_COND(!(gpuSubReg >= MAX_FRAGMENT_ATTRIBUTES), "Fragment attribute identifier out of range.");
            //  Set fragment input attribute active flag GPU register.
            state.fragmentInputAttributes[gpuSubReg] = gpuData.booleanVal;

            break;

        //  GPU fragment registers.
        case GPU_FRAGMENT_PROGRAM:

            GPU_DEBUG(
                CG_INFO("Write GPU_FRAGMENT_PROGRAM = %08x.", gpuData.uintVal);
            )

            //  Check address range.

            //  Set fragment program address register.
            state.fragProgramAddr = gpuData.uintVal;

            break;

        case GPU_FRAGMENT_PROGRAM_PC:

            GPU_DEBUG(
                CG_INFO("Write GPU_FRAGMENT_PROGRAM_PC = %04x.", gpuData.uintVal);
            )

            //  Check fragment program PC range.

            //  Set fragment program start PC register.
            state.fragProgramStartPC = gpuData.uintVal;

            break;

        case GPU_FRAGMENT_PROGRAM_SIZE:

            GPU_DEBUG(
                CG_INFO("Write GPU_FRAGMENT_PROGRAM_SIZE = %d.", gpuData.uintVal);
            )

            //  Check fragment program size.

            //  Set fragment program to load size (instructions).
            state.fragProgramSize = gpuData.uintVal;

            break;

        case GPU_FRAGMENT_THREAD_RESOURCES:

            GPU_DEBUG(
                CG_INFO("Write GPU_FRAGMENT_THREAD_RESOURCES = %d.", gpuData.uintVal);
            )

            //  Set per fragment thread resource usage.
            state.fragThreadResources = gpuData.uintVal;

            break;

        case GPU_FRAGMENT_CONSTANT:

            GPU_DEBUG(
                CG_INFO("Write GPU_FRAGMENT_CONSTANT[%d] = {%f, %f, %f, %f}.", gpuSubReg,
                    gpuData.qfVal[0], gpuData.qfVal[1], gpuData.qfVal[2], gpuData.qfVal[3]);
            )

            //  Check constant range.
            CG_ASSERT_COND(!(gpuSubReg >= MAX_FRAGMENT_CONSTANTS), "Out of range fragment constant register.");
            //  Set fragment constant register.
            state.fConstants[gpuSubReg].setComponents(gpuData.qfVal[0],
                gpuData.qfVal[1], gpuData.qfVal[2], gpuData.qfVal[3]);

            //  Load fragment constant in the shader behaviorModel.
            bmShader->loadShaderState(0, cg1gpu::PARAM, gpuSubReg + FRAGMENT_PARTITION * UNIFIED_CONSTANT_NUM_REGS, state.fConstants[gpuSubReg]);

            break;

        case GPU_MODIFY_FRAGMENT_DEPTH:

            GPU_DEBUG(
                CG_INFO("Write GPU_MODIFY_FRAGMENT_DEPTH = %s.", gpuData.booleanVal ? "T" : "F");
            )

            //  Set fragment constant register.
            state.modifyDepth = gpuData.booleanVal;

            break;

        //  GPU Shader Program registers.
        case GPU_SHADER_PROGRAM_ADDRESS:

            //  Check address range.

            //  Set shader program address register.  */
            state.programAddress = gpuData.uintVal;

            GPU_DEBUG(
                CG_INFO("Write GPU_SHADER_PROGRAM_ADDDRESS = %x.", gpuData.uintVal);
            )

            break;

        case GPU_SHADER_PROGRAM_SIZE:

            //  Check shader program size.

            //  Set shader program size (bytes).
            state.programSize = gpuData.uintVal;

            GPU_DEBUG(
                CG_INFO("Write GPU_SHADER_PROGRAM_SIZE = %x.", gpuData.uintVal);
            )

            break;

        case GPU_SHADER_PROGRAM_LOAD_PC:

            //  Check fragment program PC range.

            //  Set shader program load PC register.
            state.programLoadPC = gpuData.uintVal;

            GPU_DEBUG(
                CG_INFO("Write GPU_SHADER_PROGRAM_LOAD_PC = %x.", gpuData.uintVal);
            )

            break;


        case GPU_SHADER_PROGRAM_PC:

            {

                //  Check fragment program PC range.

                //  Set shader program PC register for the corresponding shader target.
                state.programStartPC[gpuSubReg] = gpuData.uintVal;

                //  Get shader target for which the per-thread resource register is changed.
                ShaderTarget shTarget;
                switch(gpuSubReg)
                {
                    case VERTEX_TARGET:
                        shTarget = VERTEX_TARGET;
                        break;
                    case FRAGMENT_TARGET:
                        shTarget = FRAGMENT_TARGET;
                        break;
                    case TRIANGLE_TARGET:
                        shTarget = TRIANGLE_TARGET;
                        break;
                    case MICROTRIANGLE_TARGET:
                        shTarget = MICROTRIANGLE_TARGET;
                        break;
                    default:
                        CG_ASSERT("Undefined shader target.");
                        break;
                }

                GPU_DEBUG(
                    CG_INFO("Write GPU_SHADER_PROGRAM_LOAD_PC[");

                    switch(gpuSubReg)
                    {
                        case VERTEX_TARGET:
                            CG_INFO("VERTEX_TARGET");
                            break;
                        case FRAGMENT_TARGET:
                            CG_INFO("FRAGMENT_TARGET");
                            break;
                        case TRIANGLE_TARGET:
                            CG_INFO("TRIANGLE_TARGET");
                            break;
                        case MICROTRIANGLE_TARGET:
                            CG_INFO("MICROTRIANGLE_TARGET");
                            break;
                        default:
                            CG_ASSERT("Undefined shader target.");
                            break;
                    }

                    CG_INFO("] = %x.", gpuData.uintVal);
                )
            }

            break;

        case GPU_SHADER_THREAD_RESOURCES:

            {
                //  Set per fragment thread resource usage register for the corresponding shader target.
                state.programResources[gpuSubReg] = gpuData.uintVal;

                //  Get shader target for which the per-thread resource register is changed.
                ShaderTarget shTarget;
                switch(gpuSubReg)
                {
                    case VERTEX_TARGET:
                        shTarget = VERTEX_TARGET;
                        break;
                    case FRAGMENT_TARGET:
                        shTarget = FRAGMENT_TARGET;
                        break;
                    case TRIANGLE_TARGET:
                        shTarget = TRIANGLE_TARGET;
                        break;
                    case MICROTRIANGLE_TARGET:
                        shTarget = MICROTRIANGLE_TARGET;
                        break;
                    default:
                        CG_ASSERT("Undefined shader target.");
                        break;
                }

                GPU_DEBUG(
                    CG_INFO("WriteGPU_SHADER_THREAD_RESOURCES[");

                    switch(gpuSubReg)
                    {
                        case VERTEX_TARGET:
                            CG_INFO("VERTEX_TARGET");
                            break;
                        case FRAGMENT_TARGET:
                            CG_INFO("FRAGMENT_TARGET");
                            break;
                        case TRIANGLE_TARGET:
                            CG_INFO("TRIANGLE_TARGET");
                            break;
                        case MICROTRIANGLE_TARGET:
                            CG_INFO("MICROTRIANGLE_TARGET");
                            break;
                        default:
                            CG_ASSERT("Undefined shader target.");
                            break;
                    }

                    CG_INFO("] = %x.", gpuData.uintVal);
                )
            }

            break;


        //  GPU Texture Unit registers.
        case GPU_TEXTURE_ENABLE:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_ENABLE[%d] = %s.", gpuSubReg, gpuData.booleanVal ? "T" : "F");
            )

            //  Write texture enable register.
            state.textureEnabled[gpuSubReg] = gpuData.booleanVal;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_MODE:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_MODE[%d] = ", gpuSubReg);
                switch(gpuData.txMode)
                {
                    case GPU_TEXTURE1D:
                        CG_INFO("TEXTURE1D");
                        break;

                    case GPU_TEXTURE2D:
                        CG_INFO("TEXTURE2D");
                        break;

                    case GPU_TEXTURE3D:
                        CG_INFO("TEXTURE3D");
                        break;

                    case GPU_TEXTURECUBEMAP:
                        CG_INFO("TEXTURECUBEMAP");
                        break;

                    default:
                        CG_ASSERT("Unsupported texture mode.");
                }
            )

            //  Write texture mode register.
            state.textureMode[gpuSubReg] = gpuData.txMode;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_ADDRESS:

            //  WARNING:  As the current simulator only support an index per register we must
            //  decode the texture unit, mipmap and cubemap image.  To do so the cubemap images of
            //  a mipmap are stored sequentally and then the mipmaps for a texture unit
            //  are stored sequentially and then the mipmap addresses of the other texture units
            //  are stored sequentially.

            //  Calculate texture unit and mipmap level.
            textUnit = gpuSubReg / (MAX_TEXTURE_SIZE * CUBEMAP_IMAGES);
            mipmap = GPU_MOD(gpuSubReg / CUBEMAP_IMAGES, MAX_TEXTURE_SIZE);
            cubemap = GPU_MOD(gpuSubReg, CUBEMAP_IMAGES);

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_ADDRESS[%d][%d][%d] = %08x.", textUnit, mipmap, cubemap, gpuData.uintVal);
            )

            //  Write texture address register (per cubemap image, mipmap and texture unit).
            state.textureAddress[textUnit][mipmap][cubemap] = gpuData.uintVal;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_WIDTH:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_WIDTH[%d] = %d.", gpuSubReg, gpuData.uintVal);
            )

            //  Write texture width (first mipmap).
            state.textureWidth[gpuSubReg] = gpuData.uintVal;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_HEIGHT:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_HEIGHT[%d] = %d.", gpuSubReg, gpuData.uintVal);
            )

            //  Write texture height (first mipmap).
            state.textureHeight[gpuSubReg] = gpuData.uintVal;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_DEPTH:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_DEPTH[%d] = %d.", gpuSubReg, gpuData.uintVal);
            )

            //  Write texture width (first mipmap).
            state.textureDepth[gpuSubReg] = gpuData.uintVal;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_WIDTH2:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_WIDTH2[%d] = %d.", gpuSubReg, gpuData.uintVal);
            )

            //  Write texture width (log of 2 of the first mipmap).
            state.textureWidth2[gpuSubReg] = gpuData.uintVal;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_HEIGHT2:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_HEIGHT2[%d] = %d.", gpuSubReg, gpuData.uintVal);
            )

            //  Write texture height (log 2 of the first mipmap).
            state.textureHeight2[gpuSubReg] = gpuData.uintVal;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_DEPTH2:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_DEPTH2[%d] = %d.", gpuSubReg, gpuData.uintVal);
            )

            //  Write texture depth (log of 2 of the first mipmap).
            state.textureDepth2[gpuSubReg] = gpuData.uintVal;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_BORDER:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_BORDER[%d] = %d.", gpuSubReg, gpuData.uintVal);
            )

            //  Write texture border register.
            state.textureBorder[gpuSubReg] = gpuData.uintVal;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;


        case GPU_TEXTURE_FORMAT:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_FORMAT[%d] = ", gpuSubReg);
                switch(gpuData.txFormat)
                {
                    case GPU_ALPHA8:
                        CG_INFO("GPU_ALPHA8");
                        break;
                    case GPU_ALPHA12:
                        CG_INFO("GPU_ALPHA12");
                        break;

                    case GPU_ALPHA16:
                        CG_INFO("GPU_ALPHA16");
                        break;

                    case GPU_DEPTH_COMPONENT16:
                        CG_INFO("GPU_DEPTH_COMPONENT16");
                        break;

                    case GPU_DEPTH_COMPONENT24:
                        CG_INFO("GPU_DEPTH_COMPONENT24");
                        break;

                    case GPU_DEPTH_COMPONENT32:
                        CG_INFO("GPU_DEPTH_COMPONENT32");
                        break;

                    case GPU_LUMINANCE8:
                        CG_INFO("GPU_LUMINANCE8");
                        break;

                    case GPU_LUMINANCE12:
                        CG_INFO("GPU_LUMINANCE12");
                        break;

                    case GPU_LUMINANCE16:
                        CG_INFO("GPU_LUMINANCE16");
                        break;

                    case GPU_LUMINANCE4_ALPHA4:
                        CG_INFO("GPU_LUMINANCE4_ALPHA4");
                        break;

                    case GPU_LUMINANCE6_ALPHA2:
                        CG_INFO("GPU_LUMINANCE6_ALPHA2");
                        break;

                    case GPU_LUMINANCE8_ALPHA8:
                        CG_INFO("GPU_LUMINANCE8_ALPHA8");
                        break;

                    case GPU_LUMINANCE12_ALPHA4:
                        CG_INFO("GPU_LUMINANCE12_ALPHA4");
                        break;

                    case GPU_LUMINANCE12_ALPHA12:
                        CG_INFO("GPU_LUMINANCE12_ALPHA12");
                        break;

                    case GPU_LUMINANCE16_ALPHA16:
                        CG_INFO("GPU_LUMINANCE16_ALPHA16");
                        break;

                    case GPU_INTENSITY8:
                        CG_INFO("GPU_INTENSITY8");
                        break;

                    case GPU_INTENSITY12:
                        CG_INFO("GPU_INTENSITY12");
                        break;

                    case GPU_INTENSITY16:
                        CG_INFO("GPU_INTENSITY16");
                        break;
                    case GPU_RGB332:
                        CG_INFO("GPU_RGB332");
                        break;

                    case GPU_RGB444:
                        CG_INFO("GPU_RGB444");
                        break;

                    case GPU_RGB555:
                        CG_INFO("GPU_RGB555");
                        break;

                    case GPU_RGB565:
                        CG_INFO("GPU_RGB565");
                        break;

                    case GPU_RGB888:
                        CG_INFO("GPU_RGB888");
                        break;

                    case GPU_RGB101010:
                        CG_INFO("GPU_RGB101010");
                        break;
                    case GPU_RGB121212:
                        CG_INFO("GPU_RGB121212");
                        break;

                    case GPU_RGBA2222:
                        CG_INFO("GPU_RGBA2222");
                        break;

                    case GPU_RGBA4444:
                        CG_INFO("GPU_RGBA4444");
                        break;

                    case GPU_RGBA5551:
                        CG_INFO("GPU_RGBA5551");
                        break;

                    case GPU_RGBA8888:
                        CG_INFO("GPU_RGBA8888");
                        break;

                    case GPU_RGBA1010102:
                        CG_INFO("GPU_RGB1010102");
                        break;

                    case GPU_R16:
                        CG_INFO("GPU_R16");
                        break;

                    case GPU_RG16:
                        CG_INFO("GPU_RG16");
                        break;

                    case GPU_RGBA16:
                        CG_INFO("GPU_RGBA16");
                        break;

                    case GPU_R16F:
                        CG_INFO("GPU_R16F");
                        break;

                    case GPU_RG16F:
                        CG_INFO("GPU_RG16F");
                        break;

                    case GPU_RGBA16F:
                        CG_INFO("GPU_RGBA16F");
                        break;

                    case GPU_R32F:
                        CG_INFO("GPU_R32F");
                        break;

                    case GPU_RG32F:
                        CG_INFO("GPU_RG32F");
                        break;

                    case GPU_RGBA32F:
                        CG_INFO("GPU_RGBA32F");
                        break;

                    default:
                        CG_INFO("unknown format with code %0X", gpuData.txFormat);
                        break;
                }
            )

            //  Write texture format register.
            state.textureFormat[gpuSubReg] = gpuData.txFormat;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_REVERSE:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_REVERSE[%d] = %s.", gpuSubReg, gpuData.booleanVal ? "T" : "F");
            )

            //  Write texture reverse register.  */
            state.textureReverse[gpuSubReg] = gpuData.booleanVal;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_D3D9_COLOR_CONV:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_D3D9_COLOR_CONV[%d] = %s.", gpuSubReg, gpuData.booleanVal ? "T" : "F");
            )

            //  Write texture D3D9 color order conversion register.
            state.textD3D9ColorConv[gpuSubReg] = gpuData.booleanVal;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_D3D9_V_INV:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_D3D9_V_INV[%d] = %s.", gpuSubReg, gpuData.booleanVal ? "T" : "F");
            )

            //  Write texture D3D9 v coordinate inversion register.
            state.textD3D9VInvert[gpuSubReg] = gpuData.booleanVal;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_COMPRESSION:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_COMPRESSION[%d] = ", gpuSubReg);
                switch(gpuData.txCompression)
                {
                    case GPU_NO_TEXTURE_COMPRESSION:
                        CG_INFO("GPU_NO_TEXTURE_COMPRESSION.");
                        break;

                    case GPU_S3TC_DXT1_RGB:
                        CG_INFO("GPU_S3TC_DXT1_RGB.");
                        break;

                    case GPU_S3TC_DXT1_RGBA:
                        CG_INFO("GPU_S3TC_DXT1_RGBA.");
                        break;

                    case GPU_S3TC_DXT3_RGBA:
                        CG_INFO("GPU_S3TC_DXT3_RGBA.");
                        break;

                    case GPU_S3TC_DXT5_RGBA:
                        CG_INFO("GPU_S3TC_DXT5_RGBA.");
                        break;

                    case GPU_LATC1:
                        CG_INFO("GPU_LATC1.");
                        break;

                    case GPU_LATC1_SIGNED:
                        CG_INFO("GPU_LATC1_SIGNED.");
                        break;

                    case GPU_LATC2:
                        CG_INFO("GPU_LATC2.");
                        break;

                    case GPU_LATC2_SIGNED:
                        CG_INFO("GPU_LATC2_SIGNED.");
                        break;

                    default:
                        CG_ASSERT("Unsupported texture compression mode.");
                        break;
                }
            )

            //  Write texture compression register.
            state.textureCompr[gpuSubReg] = gpuData.txCompression;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_BLOCKING:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_BLOCKING[%d] = ", gpuSubReg);
                switch(gpuData.txBlocking)
                {
                    case GPU_TXBLOCK_TEXTURE:
                        CG_INFO("GPU_TXBLOCK_TEXTURE.");
                        break;

                    case GPU_TXBLOCK_FRAMEBUFFER:
                        CG_INFO("GPU_TXBLOCK_FRAMEBUFFER.");
                        break;

                    default:
                        CG_ASSERT("Unsupported texture blocking mode.");
                        break;
                }
            )

            //  Write texture blocking mode register.
            state.textureBlocking[gpuSubReg] = gpuData.txBlocking;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_BORDER_COLOR:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_COLOR[%d] = {%f, %f, %f, %f}.", gpuSubReg,
                    gpuData.qfVal[0], gpuData.qfVal[1], gpuData.qfVal[2], gpuData.qfVal[3]);
            )

            //  Write texture border color register.
            state.textBorderColor[gpuSubReg][0] = gpuData.qfVal[0];
            state.textBorderColor[gpuSubReg][1] = gpuData.qfVal[1];
            state.textBorderColor[gpuSubReg][2] = gpuData.qfVal[2];
            state.textBorderColor[gpuSubReg][3] = gpuData.qfVal[3];

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_WRAP_S:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_WRAP_S[%d] = ", gpuSubReg);
                switch(gpuData.txClamp)
                {
                    case GPU_TEXT_CLAMP:
                        CG_INFO("CLAMP");
                        break;

                    case GPU_TEXT_CLAMP_TO_EDGE:
                        CG_INFO("CLAMP_TO_EDGE");
                        break;

                    case GPU_TEXT_REPEAT:
                        CG_INFO("REPEAT");
                        break;

                    case GPU_TEXT_CLAMP_TO_BORDER:
                        CG_INFO("CLAMP_TO_BORDER");
                        break;

                    case GPU_TEXT_MIRRORED_REPEAT:
                        CG_INFO("MIRRORED_REPEAT");
                        break;

                    default:
                        CG_ASSERT("Unsupported texture clamp mode.");
                        break;
                }
            )

            //  Write texture wrap in s dimension register.
            state.textureWrapS[gpuSubReg] = gpuData.txClamp;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_WRAP_T:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_WRAP_T[%d] = ", gpuSubReg);
                switch(gpuData.txClamp)
                {
                    case GPU_TEXT_CLAMP:
                        CG_INFO("CLAMP");
                        break;

                    case GPU_TEXT_CLAMP_TO_EDGE:
                        CG_INFO("CLAMP_TO_EDGE");
                        break;

                    case GPU_TEXT_REPEAT:
                        CG_INFO("REPEAT");
                        break;

                    case GPU_TEXT_CLAMP_TO_BORDER:
                        CG_INFO("CLAMP_TO_BORDER");
                        break;

                    case GPU_TEXT_MIRRORED_REPEAT:
                        CG_INFO("MIRRORED_REPEAT");
                        break;

                    default:
                        CG_ASSERT("Unsupported texture clamp mode.");
                        break;
                }
            )

            //  Write texture wrap in t dimension register.
            state.textureWrapT[gpuSubReg] = gpuData.txClamp;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_WRAP_R:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_WRAP_R[%d] = ", gpuSubReg);
                switch(gpuData.txClamp)
                {
                    case GPU_TEXT_CLAMP:
                        CG_INFO("CLAMP");
                        break;

                    case GPU_TEXT_CLAMP_TO_EDGE:
                        CG_INFO("CLAMP_TO_EDGE");
                        break;

                    case GPU_TEXT_REPEAT:
                        CG_INFO("REPEAT");
                        break;

                    case GPU_TEXT_CLAMP_TO_BORDER:
                        CG_INFO("CLAMP_TO_BORDER");
                        break;

                    case GPU_TEXT_MIRRORED_REPEAT:
                        CG_INFO("MIRRORED_REPEAT");
                        break;

                    default:
                        CG_ASSERT("Unsupported texture clamp mode.");
                        break;
                }
            )

            //  Write texture wrap in r dimension register.
            state.textureWrapR[gpuSubReg] = gpuData.txClamp;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_NON_NORMALIZED:

            GPU_DEBUG(CG_INFO("Write GPU_TEXTURE_NON_NORMALIZED[%d] = ", gpuSubReg, gpuData.booleanVal ? "T": "F"); )

            //  Write texture non-normalized coordinates register.
            state.textureNonNormalized[gpuSubReg] = gpuData.booleanVal;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_MIN_FILTER:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_MIN_FILTER[%d] = ", gpuSubReg);
                switch(gpuData.txFilter)
                {
                    case GPU_NEAREST:
                        CG_INFO("NEAREST");
                        break;

                    case GPU_LINEAR:
                        CG_INFO("LINEAR");
                        break;

                    case GPU_NEAREST_MIPMAP_NEAREST:
                        CG_INFO("NEAREST_MIPMAP_NEAREST");
                        break;

                    case GPU_NEAREST_MIPMAP_LINEAR:
                        CG_INFO("NEAREST_MIPMAP_LINEAR");
                        break;

                    case GPU_LINEAR_MIPMAP_NEAREST:
                        CG_INFO("LINEAR_MIPMAP_NEAREST");
                        break;

                    case GPU_LINEAR_MIPMAP_LINEAR:
                        CG_INFO("LINEAR_MIPMAP_LINEAR");
                        break;

                    default:
                        CG_ASSERT("Unsupported texture minification filter mode.");
                        break;
                }
            )

            //  Write texture minification filter register.
            state.textureMinFilter[gpuSubReg] = gpuData.txFilter;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_MAG_FILTER:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_MAG_FILTER[%d] = ", gpuSubReg);
                switch(gpuData.txFilter)
                {
                    case GPU_NEAREST:
                        CG_INFO("NEAREST");
                        break;

                    case GPU_LINEAR:
                        CG_INFO("LINEAR");
                        break;

                    case GPU_NEAREST_MIPMAP_NEAREST:
                        CG_INFO("NEAREST_MIPMAP_NEAREST");
                        break;

                    case GPU_NEAREST_MIPMAP_LINEAR:
                        CG_INFO("NEAREST_MIPMAP_LINEAR");
                        break;

                    case GPU_LINEAR_MIPMAP_NEAREST:
                        CG_INFO("LINEAR_MIPMAP_NEAREST");
                        break;

                    case GPU_LINEAR_MIPMAP_LINEAR:
                        CG_INFO("LINEAR_MIPMAP_LINEAR");
                        break;

                    default:
                        CG_ASSERT("Unsupported texture minification filter mode.");
                        break;
                }
            )

            //  Write texture magnification filter register.
            state.textureMagFilter[gpuSubReg] = gpuData.txFilter;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_ENABLE_COMPARISON:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_ENABLE_COMPARISON[%d] = %s.", gpuSubReg, gpuData.booleanVal ? "T" : "F");
            )

            //  Write texture enable comparison (PCF) register.
            state.textureEnableComparison[gpuSubReg] = gpuData.booleanVal;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_COMPARISON_FUNCTION:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_COMPARISON_FUNCTION[%d] = ", gpuSubReg);
                switch(gpuData.compare)
                {
                    case GPU_NEVER:
                        CG_INFO("NEVER.");
                        break;

                    case GPU_ALWAYS:
                        CG_INFO("ALWAYS.");
                        break;

                    case GPU_LESS:
                        CG_INFO("LESS.");
                        break;

                    case GPU_LEQUAL:
                        CG_INFO("LEQUAL.");
                        break;

                    case GPU_EQUAL:
                        CG_INFO("EQUAL.");
                        break;

                    case GPU_GEQUAL:
                        CG_INFO("GEQUAL.");
                        break;

                    case GPU_GREATER:
                        CG_INFO("GREATER.");
                        break;

                    case GPU_NOTEQUAL:
                        CG_INFO("NOTEQUAL.");

                    default:
                        CG_ASSERT("Undefined compare function mode");
                        break;
                }
            )

            //  Write texture comparison function (PCF) register.
            state.textureComparisonFunction[gpuSubReg] = gpuData.compare;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_SRGB:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_SRGB[%d] = %s.", gpuSubReg, gpuData.booleanVal ? "T" : "F");
            )

            //  Write texture SRGB to linear conversion register.
            state.textureSRGB[gpuSubReg] = gpuData.booleanVal;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_MIN_LOD:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_MIN_LOD[%d] = %f.", gpuSubReg, gpuData.uintVal);
            )

            //  Write texture minimum lod register.
            state.textureMinLOD[gpuSubReg] = gpuData.f32Val;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_MAX_LOD:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_MAX_LOD[%d] = %f.", gpuSubReg, gpuData.uintVal);
            )

            //  Write texture maximum lod register.
            state.textureMaxLOD[gpuSubReg] = gpuData.f32Val;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_LOD_BIAS:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_LOD_BIAS[%d] = %f.", gpuSubReg, gpuData.f32Val);
            )

            //  Write texture lod bias register.
            state.textureLODBias[gpuSubReg] = gpuData.f32Val;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_MIN_LEVEL:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_MIN_LEVEL[%d] = %d.", gpuSubReg, gpuData.uintVal);
            )

            //  Write texture minimum mipmap level register.
            state.textureMinLevel[gpuSubReg] = gpuData.uintVal;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_MAX_LEVEL:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_MAX_LEVEL[%d] = %d.", gpuSubReg, gpuData.uintVal);
            )

            //  Write texture maximum mipmap level register.
            state.textureMaxLevel[gpuSubReg] = gpuData.uintVal;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXT_UNIT_LOD_BIAS:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXT_UNIT_LOD_BIAS[%d] = %f.", gpuSubReg, gpuData.f32Val);
            )

            //  Write texture unit lod bias register.
            state.textureUnitLODBias[gpuSubReg] = gpuData.f32Val;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        case GPU_TEXTURE_MAX_ANISOTROPY:

            GPU_DEBUG(
                CG_INFO("Write GPU_TEXTURE_MAX_ANISOTROPY[%d] = %d.", gpuSubReg, gpuData.uintVal);
            )

            //  Write texture unit max anisotropy register.
            state.maxAnisotropy[gpuSubReg] = gpuData.uintVal;

            //  Write register in the Texture Behavior Model.
            bmTexture->writeRegister(gpuReg, gpuSubReg, gpuData);

            break;

        //  GPU Stencil test registers.
        case GPU_STENCIL_TEST:

            GPU_DEBUG(
                CG_INFO("Write GPU_STENCIL_TEST = %s.", gpuData.booleanVal ? "T" : "F");
            )

            //  Set stencil test enable flag.
            state.stencilTest = gpuData.booleanVal;

            break;

        case GPU_STENCIL_FUNCTION:

            GPU_DEBUG(
                CG_INFO("Write GPU_STENCIL_FUNCTION = ");
                switch(gpuData.compare)
                {
                    case GPU_NEVER:
                        CG_INFO("NEVER.");
                        break;

                    case GPU_ALWAYS:
                        CG_INFO("ALWAYS.");
                        break;

                    case GPU_LESS:
                        CG_INFO("LESS.");
                        break;

                    case GPU_LEQUAL:
                        CG_INFO("LEQUAL.");
                        break;

                    case GPU_EQUAL:
                        CG_INFO("EQUAL.");
                        break;

                    case GPU_GEQUAL:
                        CG_INFO("GEQUAL.");
                        break;

                    case GPU_GREATER:
                        CG_INFO("GREATER.");
                        break;

                    case GPU_NOTEQUAL:
                        CG_INFO("NOTEQUAL.");

                    default:
                        CG_ASSERT("Undefined compare function mode");
                        break;
                }
            )

            //  Set stencil test compare function.
            state.stencilFunction = gpuData.compare;

            break;

        case GPU_STENCIL_REFERENCE:

            GPU_DEBUG(
                CG_INFO("Write GPU_STENCIL_REFERENCE = %02x.", gpuData.uintVal);
            )

            //  Set stencil test reference value.
            state.stencilReference = U08(gpuData.uintVal);

            break;

        case GPU_STENCIL_COMPARE_MASK:

            GPU_DEBUG(
                CG_INFO("Write GPU_STENCIL_COMPARE_MASK = %02x.", gpuData.uintVal);
            )

            //  Set stencil test compare mask.
            state.stencilTestMask = U08(gpuData.uintVal);

            break;

        case GPU_STENCIL_UPDATE_MASK:

            GPU_DEBUG(
                CG_INFO("Write GPU_UPDATE_MASK = %02x.", gpuData.uintVal);
            )

            //  Set stencil test update mask.
            state.stencilUpdateMask = U08(gpuData.uintVal);

            break;

        case GPU_STENCIL_FAIL_UPDATE:

            GPU_DEBUG(
                CG_INFO("Write GPU_STENCIL_FAIL_UPDATE = ");
                switch(gpuData.stencilUpdate)
                {
                    case STENCIL_KEEP:
                        CG_INFO("KEEP.");
                        break;

                    case STENCIL_ZERO:
                        CG_INFO("ZERO.");
                        break;

                    case STENCIL_REPLACE:
                        CG_INFO("REPLACE.");
                        break;

                    case STENCIL_INCR:
                        CG_INFO("INCREMENT.");
                        break;

                    case STENCIL_DECR:
                        CG_INFO("DECREMENT.");
                        break;

                    case STENCIL_INVERT:
                        CG_INFO("INVERT.");
                        break;

                    case STENCIL_INCR_WRAP:
                        CG_INFO("INCREMENT AND WRAP.");
                        break;

                    case STENCIL_DECR_WRAP:
                        CG_INFO("DECREMENT AND WRAP.");
                        break;

                    default:
                        CG_ASSERT("Undefined stencil update function");
                        break;
                }
            )

            //  Set stencil fail stencil update function.
            state.stencilFail = gpuData.stencilUpdate;

            break;

        case GPU_DEPTH_FAIL_UPDATE:

            GPU_DEBUG(
                CG_INFO("Write GPU_DEPTH_FAIL_UPDATE = ");
                switch(gpuData.stencilUpdate)
                {
                    case STENCIL_KEEP:
                        CG_INFO("KEEP.");
                        break;

                    case STENCIL_ZERO:
                        CG_INFO("ZERO.");
                        break;

                    case STENCIL_REPLACE:
                        CG_INFO("REPLACE.");
                        break;

                    case STENCIL_INCR:
                        CG_INFO("INCREMENT.");
                        break;

                    case STENCIL_DECR:
                        CG_INFO("DECREMENT.");
                        break;

                    case STENCIL_INVERT:
                        CG_INFO("INVERT.");
                        break;

                    case STENCIL_INCR_WRAP:
                        CG_INFO("INCREMENT AND WRAP.");
                        break;

                    case STENCIL_DECR_WRAP:
                        CG_INFO("DECREMENT AND WRAP.");
                        break;

                    default:
                        CG_ASSERT("Undefined stencil update function");
                        break;
                }
            )

            //  Set stencil update function for depth fail.
            state.depthFail = gpuData.stencilUpdate;

            break;

        case GPU_DEPTH_PASS_UPDATE:

            GPU_DEBUG(
                CG_INFO("Write GPU_DEPTH_PASS_UPDATE = ");
                switch(gpuData.stencilUpdate)
                {
                    case STENCIL_KEEP:
                        CG_INFO("KEEP.");
                        break;

                    case STENCIL_ZERO:
                        CG_INFO("ZERO.");
                        break;

                    case STENCIL_REPLACE:
                        CG_INFO("REPLACE.");
                        break;

                    case STENCIL_INCR:
                        CG_INFO("INCREMENT.");
                        break;

                    case STENCIL_DECR:
                        CG_INFO("DECREMENT.");
                        break;

                    case STENCIL_INVERT:
                        CG_INFO("INVERT.");
                        break;

                    case STENCIL_INCR_WRAP:
                        CG_INFO("INCREMENT AND WRAP.");
                        break;

                    case STENCIL_DECR_WRAP:
                        CG_INFO("DECREMENT AND WRAP.");
                        break;

                    default:
                        CG_ASSERT("Undefined stencil update function");
                        break;
                }
            )

            //  Set stencil update function for depth pass.
            state.depthPass = gpuData.stencilUpdate;

            break;

        //  GPU Depth test registers.
        case GPU_DEPTH_TEST:

            GPU_DEBUG(
                CG_INFO("Write GPU_DEPTH_TEST = %s", gpuData.booleanVal ? "T" : "F");
            )

            //  Set depth test enable flag.
            state.depthTest = gpuData.booleanVal;

            break;

        case GPU_DEPTH_FUNCTION:

            GPU_DEBUG(
                CG_INFO("Write GPU_DEPTH_FUNCION = ");
                switch(gpuData.compare)
                {
                    case GPU_NEVER:
                        CG_INFO("NEVER.");
                        break;

                    case GPU_ALWAYS:
                        CG_INFO("ALWAYS.");
                        break;

                    case GPU_LESS:
                        CG_INFO("LESS.");
                        break;

                    case GPU_LEQUAL:
                        CG_INFO("LEQUAL.");
                        break;

                    case GPU_EQUAL:
                        CG_INFO("EQUAL.");
                        break;

                    case GPU_GEQUAL:
                        CG_INFO("GEQUAL.");
                        break;

                    case GPU_GREATER:
                        CG_INFO("GREATER.");
                        break;

                    case GPU_NOTEQUAL:
                        CG_INFO("NOTEQUAL.");

                    default:
                        CG_ASSERT("Undefined compare function mode");
                        break;
                }
            )

            //  Set depth test compare function.
            state.depthFunction = gpuData.compare;

            break;

        case GPU_DEPTH_MASK:

            GPU_DEBUG(
                CG_INFO("Write GPU_DEPTH_MASK = %s.", gpuData.booleanVal ? "T" : "F");
            )

            //  Set depth test update mask.
            state.depthMask = gpuData.booleanVal;

            break;

        case GPU_ZSTENCIL_COMPRESSION:

            GPU_DEBUG(
                CG_INFO("Write GPU_ZSTENCIL_COMPRESSION = %s.", gpuData.booleanVal ? "T" : "F");
            )

            //  Set Z/Stencil compression enable/disable register.
            state.zStencilCompression = gpuData.booleanVal;

            break;

        //  GPU Color Buffer and Render Target registers.

        case GPU_COLOR_BUFFER_FORMAT:

            GPU_DEBUG(
                CG_INFO("Write GPU_COLOR_BUFFER_FORMAT = ");
                switch(gpuData.txFormat)
                {
                    case GPU_RGBA8888:
                        CG_INFO(" GPU_RGBA8888.");
                        break;

                    case GPU_RG16F:
                        CG_INFO(" GPU_RG16F.");
                        break;

                    case GPU_R32F:
                        CG_INFO(" GPU_R32F.");
                        break;

                    case GPU_RGBA16:
                        CG_INFO(" GPU_RGBA16.");
                        break;

                    case GPU_RGBA16F:
                        CG_INFO(" GPU_RGBA16F.");
                        break;

                    default:
                        CG_ASSERT("Unsupported color buffer format.");
                        break;
                }
            )

            //  Set color buffer format register.
            state.colorBufferFormat = gpuData.txFormat;

            //  Aliased to render target 0.
            state.rtFormat[0] = gpuData.txFormat;

            break;

        case GPU_COLOR_COMPRESSION:

            GPU_DEBUG(
                CG_INFO("Write GPU_COLOR_COMPRESSION = %s.", gpuData.booleanVal ? "T" : "F");
            )

            //  Set color compression enable/disable register.
            state.colorCompression = gpuData.booleanVal;

            break;

        case GPU_COLOR_SRGB_WRITE:
            
            GPU_DEBUG(
                CG_INFO("Write GPU_COLOR_SRGB_WRITE = %s", gpuData.booleanVal ? "T" : "F");
            )
            
            //  Set sRGB conversion on color write register.
            state.colorSRGBWrite = gpuData.booleanVal;
            
            break;
            
        case GPU_RENDER_TARGET_ENABLE:

            GPU_DEBUG(
                CG_INFO("Write GPU_RENDER_TARGET_ENABLE[%d] = %s.", gpuSubReg, gpuData.booleanVal ? "T" : "F");
            )

            //  Set render target enable/disable register.
            state.rtEnable[gpuSubReg] = gpuData.booleanVal;

            break;

        case GPU_RENDER_TARGET_FORMAT:

            GPU_DEBUG(
                CG_INFO("Write GPU_RENDER_TARGET_FORMAT[%d] = ", gpuSubReg);
                switch(gpuData.txFormat)
                {
                    case GPU_RGBA8888:
                        CG_INFO(" GPU_RGBA8888.");
                        break;

                    case GPU_RG16F:
                        CG_INFO(" GPU_RG16F.");
                        break;

                    case GPU_R32F:
                        CG_INFO(" GPU_R32F.");
                        break;

                    case GPU_RGBA16:
                        CG_INFO(" GPU_RGBA16.");
                        break;

                    case GPU_RGBA16F:
                        CG_INFO(" GPU_RGBA16F.");
                        break;

                    default:
                        CG_ASSERT("Unsupported color buffer format.");
                        break;
                }
            )

            //  Set render target format register.
            state.rtFormat[gpuSubReg] = gpuData.txFormat;
            
            //  Render target 0 is aliased to the back buffer.
            if (gpuSubReg == 0)
                state.colorBufferFormat = gpuData.txFormat;            

            break;

        case GPU_RENDER_TARGET_ADDRESS:

            GPU_DEBUG(
                CG_INFO("Write GPU_RENDER_TARGET_ADDRESS[%d] = %08x.", gpuSubReg, gpuData.uintVal);
            )

            //  Set render target base address register.
            state.rtAddress[gpuSubReg] = gpuData.uintVal;
            
            //  Back buffer is aliased to render target 0.
            if (gpuSubReg == 0)            
                state.backBufferBaseAddr = gpuData.uintVal;

            break;

        //  GPU Color Blend registers.
        case GPU_COLOR_BLEND:

            GPU_DEBUG(
                CG_INFO("Write GPU_COLOR_BLEND[%d] = %s.", gpuSubReg, gpuData.booleanVal ? "T" : "F");
            )

            //  Set color blend enable flag.
            state.colorBlend[gpuSubReg] = gpuData.booleanVal;

            break;

        case GPU_BLEND_EQUATION:

            GPU_DEBUG(
                CG_INFO("Write GPU_BLEND_EQUATION[%d] = ", gpuSubReg);
                switch(gpuData.blendEquation)
                {
                    case BLEND_FUNC_ADD:
                        CG_INFO("FUNC_ADD.");
                        break;

                    case BLEND_FUNC_SUBTRACT:
                        CG_INFO("FUNC_SUBTRACT.");
                        break;

                    case BLEND_FUNC_REVERSE_SUBTRACT:
                        CG_INFO("FUNC_REVERSE_SUBTRACT.");
                        break;

                    case BLEND_MIN:
                        CG_INFO("MIN.");
                        break;

                    case BLEND_MAX:
                        CG_INFO("MAX.");
                        break;

                    default:
                        CG_ASSERT("Unsupported blend equation mode.");
                        break;
                }
           )

            //  Set color blend equation.
            state.blendEquation[gpuSubReg] = gpuData.blendEquation;

            break;


        case GPU_BLEND_SRC_RGB:

            GPU_DEBUG(
                CG_INFO("Write GPU_BLEND_SRC_RGB[%d] = ", gpuSubReg);
                switch(gpuData.blendFunction)
                {
                    case BLEND_ZERO:
                        CG_INFO("ZERO.");
                        break;

                    case BLEND_ONE:
                        CG_INFO("ONE.");
                        break;

                    case BLEND_SRC_COLOR:
                        CG_INFO("SRC_COLOR.");
                        break;

                    case BLEND_ONE_MINUS_SRC_COLOR:
                        CG_INFO("ONE_MINUS_SRC_COLOR.");
                        break;

                    case BLEND_DST_COLOR:
                        CG_INFO("DST_COLOR.");
                        break;

                    case BLEND_ONE_MINUS_DST_COLOR:
                        CG_INFO("ONE_MINUS_DST_COLOR.");
                        break;

                    case BLEND_SRC_ALPHA:
                        CG_INFO("SRC_ALPHA.");
                        break;

                    case BLEND_ONE_MINUS_SRC_ALPHA:
                        CG_INFO("ONE_MINUS_SRC_ALPHA.");
                        break;

                    case BLEND_DST_ALPHA:
                        CG_INFO("DST_ALPHA.");
                        break;

                    case BLEND_ONE_MINUS_DST_ALPHA:
                        CG_INFO("ONE_MINUS_DST_ALPHA.");
                        break;

                    case BLEND_CONSTANT_COLOR:
                        CG_INFO("CONSTANT_COLOR.");
                        break;

                    case BLEND_ONE_MINUS_CONSTANT_COLOR:
                        CG_INFO("ONE_MINUS_CONSTANT_COLOR.");
                        break;

                    case BLEND_CONSTANT_ALPHA:
                        CG_INFO("CONSTANT_ALPHA.");
                        break;

                    case BLEND_ONE_MINUS_CONSTANT_ALPHA:
                        CG_INFO("ONE_MINUS_CONSTANT_ALPHA.");
                        break;

                    case BLEND_SRC_ALPHA_SATURATE:
                        CG_INFO("ALPHA_SATURATE.");
                        break;

                    default:
                        CG_ASSERT("Unsupported blend weight function.");
                        break;
                }
            )

            //  Set color blend source RGB weight factor.
            state.blendSourceRGB[gpuSubReg] = gpuData.blendFunction;

            break;

        case GPU_BLEND_DST_RGB:

            GPU_DEBUG(
                CG_INFO("Write GPU_BLEND_DST_RGB[%d] = ", gpuSubReg);
                switch(gpuData.blendFunction)
                {
                    case BLEND_ZERO:
                        CG_INFO("ZERO.");
                        break;

                    case BLEND_ONE:
                        CG_INFO("ONE.");
                        break;

                    case BLEND_SRC_COLOR:
                        CG_INFO("SRC_COLOR.");
                        break;

                    case BLEND_ONE_MINUS_SRC_COLOR:
                        CG_INFO("ONE_MINUS_SRC_COLOR.");
                        break;

                    case BLEND_DST_COLOR:
                        CG_INFO("DST_COLOR.");
                        break;

                    case BLEND_ONE_MINUS_DST_COLOR:
                        CG_INFO("ONE_MINUS_DST_COLOR.");
                        break;

                    case BLEND_SRC_ALPHA:
                        CG_INFO("SRC_ALPHA.");
                        break;

                    case BLEND_ONE_MINUS_SRC_ALPHA:
                        CG_INFO("ONE_MINUS_SRC_ALPHA.");
                        break;

                    case BLEND_DST_ALPHA:
                        CG_INFO("DST_ALPHA.");
                        break;

                    case BLEND_ONE_MINUS_DST_ALPHA:
                        CG_INFO("ONE_MINUS_DST_ALPHA.");
                        break;

                    case BLEND_CONSTANT_COLOR:
                        CG_INFO("CONSTANT_COLOR.");
                        break;

                    case BLEND_ONE_MINUS_CONSTANT_COLOR:
                        CG_INFO("ONE_MINUS_CONSTANT_COLOR.");
                        break;

                    case BLEND_CONSTANT_ALPHA:
                        CG_INFO("CONSTANT_ALPHA.");
                        break;

                    case BLEND_ONE_MINUS_CONSTANT_ALPHA:
                        CG_INFO("ONE_MINUS_CONSTANT_ALPHA.");
                        break;

                    case BLEND_SRC_ALPHA_SATURATE:
                        CG_INFO("ALPHA_SATURATE.");
                        break;

                    default:
                        CG_ASSERT("Unsupported blend weight function.");
                        break;
                }
            )

            //  Set color blend destination RGB weight factor.
            state.blendDestinationRGB[gpuSubReg] = gpuData.blendFunction;

            break;

        case GPU_BLEND_SRC_ALPHA:

            GPU_DEBUG(
                CG_INFO("Write GPU_BLEND_SRC_ALPHA[%d] = ", gpuSubReg);
                switch(gpuData.blendFunction)
                {
                    case BLEND_ZERO:
                        CG_INFO("ZERO.");
                        break;

                    case BLEND_ONE:
                        CG_INFO("ONE.");
                        break;

                    case BLEND_SRC_COLOR:
                        CG_INFO("SRC_COLOR.");
                        break;

                    case BLEND_ONE_MINUS_SRC_COLOR:
                        CG_INFO("ONE_MINUS_SRC_COLOR.");
                        break;

                    case BLEND_DST_COLOR:
                        CG_INFO("DST_COLOR.");
                        break;

                    case BLEND_ONE_MINUS_DST_COLOR:
                        CG_INFO("ONE_MINUS_DST_COLOR.");
                        break;

                    case BLEND_SRC_ALPHA:
                        CG_INFO("SRC_ALPHA.");
                        break;

                    case BLEND_ONE_MINUS_SRC_ALPHA:
                        CG_INFO("ONE_MINUS_SRC_ALPHA.");
                        break;

                    case BLEND_DST_ALPHA:
                        CG_INFO("DST_ALPHA.");
                        break;

                    case BLEND_ONE_MINUS_DST_ALPHA:
                        CG_INFO("ONE_MINUS_DST_ALPHA.");
                        break;

                    case BLEND_CONSTANT_COLOR:
                        CG_INFO("CONSTANT_COLOR.");
                        break;

                    case BLEND_ONE_MINUS_CONSTANT_COLOR:
                        CG_INFO("ONE_MINUS_CONSTANT_COLOR.");
                        break;

                    case BLEND_CONSTANT_ALPHA:
                        CG_INFO("CONSTANT_ALPHA.");
                        break;

                    case BLEND_ONE_MINUS_CONSTANT_ALPHA:
                        CG_INFO("ONE_MINUS_CONSTANT_ALPHA.");
                        break;

                    case BLEND_SRC_ALPHA_SATURATE:
                        CG_INFO("ALPHA_SATURATE.");
                        break;

                    default:
                        CG_ASSERT("Unsupported blend weight function.");
                        break;
                }
            )

            //  Set color blend source ALPHA weight factor.
            state.blendSourceAlpha[gpuSubReg] = gpuData.blendFunction;

            break;

        case GPU_BLEND_DST_ALPHA:

            GPU_DEBUG(
                CG_INFO("Write GPU_BLEND_DST_ALPHA[%d] = ", gpuSubReg);
                switch(gpuData.blendFunction)
                {
                    case BLEND_ZERO:
                        CG_INFO("ZERO.");
                        break;

                    case BLEND_ONE:
                        CG_INFO("ONE.");
                        break;

                    case BLEND_SRC_COLOR:
                        CG_INFO("SRC_COLOR.");
                        break;

                    case BLEND_ONE_MINUS_SRC_COLOR:
                        CG_INFO("ONE_MINUS_SRC_COLOR.");
                        break;

                    case BLEND_DST_COLOR:
                        CG_INFO("DST_COLOR.");
                        break;

                    case BLEND_ONE_MINUS_DST_COLOR:
                        CG_INFO("ONE_MINUS_DST_COLOR.");
                        break;

                    case BLEND_SRC_ALPHA:
                        CG_INFO("SRC_ALPHA.");
                        break;

                    case BLEND_ONE_MINUS_SRC_ALPHA:
                        CG_INFO("ONE_MINUS_SRC_ALPHA.");
                        break;

                    case BLEND_DST_ALPHA:
                        CG_INFO("DST_ALPHA.");
                        break;

                    case BLEND_ONE_MINUS_DST_ALPHA:
                        CG_INFO("ONE_MINUS_DST_ALPHA.");
                        break;

                    case BLEND_CONSTANT_COLOR:
                        CG_INFO("CONSTANT_COLOR.");
                        break;

                    case BLEND_ONE_MINUS_CONSTANT_COLOR:
                        CG_INFO("ONE_MINUS_CONSTANT_COLOR.");
                        break;

                    case BLEND_CONSTANT_ALPHA:
                        CG_INFO("CONSTANT_ALPHA.");
                        break;

                    case BLEND_ONE_MINUS_CONSTANT_ALPHA:
                        CG_INFO("ONE_MINUS_CONSTANT_ALPHA.");
                        break;

                    case BLEND_SRC_ALPHA_SATURATE:
                        CG_INFO("ALPHA_SATURATE.");
                        break;

                    default:
                        CG_ASSERT("Unsupported blend weight function.");
                        break;
                }
            )

            //  Set color blend destination Alpha weight factor.
            state.blendDestinationAlpha[gpuSubReg] = gpuData.blendFunction;

            break;

        case GPU_BLEND_COLOR:

            GPU_DEBUG(
                CG_INFO("Write GPU_BLEND_COLOR[%d] = {%f, %f, %f, %f}.", gpuSubReg,
                    gpuData.qfVal[0], gpuData.qfVal[1], gpuData.qfVal[2], gpuData.qfVal[3]);
            )

            //  Set color blend constant color.
            state.blendColor[gpuSubReg][0] = gpuData.qfVal[0];
            state.blendColor[gpuSubReg][1] = gpuData.qfVal[1];
            state.blendColor[gpuSubReg][2] = gpuData.qfVal[2];
            state.blendColor[gpuSubReg][3] = gpuData.qfVal[3];

            break;

        case GPU_COLOR_MASK_R:

            GPU_DEBUG(
                CG_INFO("Write GPU_WRITE_MASK_R[%d] = %s.", gpuSubReg, gpuData.booleanVal ? "T" : "F");
            )

            //  Set color mask for red component.
            state.colorMaskR[gpuSubReg] = gpuData.booleanVal;

            break;

        case GPU_COLOR_MASK_G:

            GPU_DEBUG(
                CG_INFO("Write GPU_WRITE_MASK_G[%d] = %s.", gpuSubReg, gpuData.booleanVal ? "T": "F");
            )

            //  Set color mask for green component.
            state.colorMaskG[gpuSubReg] = gpuData.booleanVal;

            break;

        case GPU_COLOR_MASK_B:

            GPU_DEBUG(
                CG_INFO("Write GPU_WRITE_MASK_B[%d] = %s.", gpuSubReg, gpuData.booleanVal ? "T" : "F");
            )

            //  Set color mask for blue component.
            state.colorMaskB[gpuSubReg] = gpuData.booleanVal;

            break;

        case GPU_COLOR_MASK_A:

            GPU_DEBUG(
                CG_INFO("Write GPU_WRITE_MASK_A[%d] = %s.", gpuSubReg, gpuData.booleanVal ? "T" : "F");
            )

            //  Set color mask for alpha component.
            state.colorMaskA[gpuSubReg] = gpuData.booleanVal;

            break;

        //  GPU Color Logical Operation registers.
        case GPU_LOGICAL_OPERATION:

            GPU_DEBUG(
                CG_INFO("Write GPU_LOGICAL_OPERATION = %s.", gpuData.booleanVal ? "T" : "F");
            )

            //  Set color logical operation enable flag.
            state.logicalOperation = gpuData.booleanVal;

            break;

        case GPU_LOGICOP_FUNCTION:

            GPU_DEBUG(
                CG_INFO("Write GPU_LOGICOP_FUNCTION = ");
                switch(gpuData.logicOp)
                {
                    case LOGICOP_CLEAR:
                        CG_INFO("CLEAR.");
                        break;

                    case LOGICOP_AND:
                        CG_INFO("AND.");
                        break;

                    case LOGICOP_AND_REVERSE:
                        CG_INFO("AND_REVERSE.");
                        break;

                    case LOGICOP_COPY:
                        CG_INFO("COPY.");
                        break;

                    case LOGICOP_AND_INVERTED:
                        CG_INFO("AND_INVERTED.");
                        break;

                    case LOGICOP_NOOP:
                        CG_INFO("NOOP.");
                        break;

                    case LOGICOP_XOR:
                        CG_INFO("XOR.");
                        break;

                    case LOGICOP_OR:
                        CG_INFO("OR.");
                        break;

                    case LOGICOP_NOR:
                        CG_INFO("NOR.");
                        break;

                    case LOGICOP_EQUIV:
                        CG_INFO("EQUIV.");
                        break;

                    case LOGICOP_INVERT:
                        CG_INFO("INVERT.");
                        break;

                    case LOGICOP_OR_REVERSE:
                        CG_INFO("OR_REVERSE.");
                        break;

                    case LOGICOP_COPY_INVERTED:
                        CG_INFO("COPY_INVERTED.");
                        break;

                    case LOGICOP_OR_INVERTED:
                        CG_INFO("OR_INVERTED.");
                        break;

                    case LOGICOP_NAND:
                        CG_INFO("NAND.");
                        break;

                    case LOGICOP_SET:
                        CG_INFO("SET.");
                        break;

                    default:
                        CG_ASSERT("Unsupported logical operation function.");
                        break;
                }
            )

            //  Set color mask for red component.
            state.logicOpFunction = gpuData.logicOp;

            break;

        case GPU_MCV2_2ND_INTERLEAVING_START_ADDR:

            GPU_DEBUG(
                CG_INFO("Write GPU_MCV2_2ND_INTERLEAVING_START_ADDR = %08x.", gpuData.uintVal);
            )

            state.mcSecondInterleavingStartAddr = gpuData.uintVal;

            break;

        default:

            CG_ASSERT("Undefined GPU register identifier.");

            break;

    }
}

void bmoGpuTop::clearColorBuffer()
{
    GLOBAL_PROFILER_ENTER_REGION("clearColorBuffer", "", "clearColorBuffer")

    if (!skipBatchMode)
    {
        U08 clearColor8bit[4 * 4];
        U08 clearColor16bit[4 * 8];
        U08 clearColor32bit[4 * 16];
        Vec4FP32 clearColor[4];

        for(U32 p = 0; p < 4; p++)
        {
            for(U32 c = 0; c < 4; c++)
            {
                clearColor[p][c] = state.colorBufferClear[c];
                ((F32 *) clearColor32bit)[p * 4 + c] = state.colorBufferClear[c];
            }
        }

        colorRGBA32FToRGBA8(clearColor, clearColor8bit);
        colorRGBA32FToRGBA16F(clearColor, clearColor16bit);

        U32 samples = state.multiSampling ? state.msaaSamples : 1;
        U32 bytesPixel;

        //switch(state.colorBufferFormat)
        switch(state.rtFormat[0])
        {
            case GPU_RGBA8888:
            case GPU_RG16F:
            case GPU_R32F:
                bytesPixel = 4;
                break;
            case GPU_RGBA16:
            case GPU_RGBA16F:
                bytesPixel = 8;
                break;
        }

        pixelMapper[0].setupDisplay(state.displayResX, state.displayResY, STAMP_WIDTH, STAMP_WIDTH,
                                 ArchConf.ras.genWidth / STAMP_WIDTH, ArchConf.ras.genHeight / STAMP_HEIGHT,
                                 ArchConf.ras.scanWidth / ArchConf.ras.genWidth, ArchConf.ras.scanHeight / ArchConf.ras.genHeight,
                                 ArchConf.ras.overScanWidth, ArchConf.ras.overScanHeight,
                                 samples, bytesPixel);

        //U08 *memory = selectMemorySpace(state.backBufferBaseAddr);
        U08 *memory = selectMemorySpace(state.rtAddress[0]);

        for(U32 y = 0; y < state.displayResY; y += 2)
        {
            for(U32 x = 0; x < state.displayResX; x += 2)
            {
                U32 address = pixelMapper[0].computeAddress(x, y);

                //address += state.backBufferBaseAddr;
                address += state.rtAddress[0];

                //  Get address inside the memory space.
                address = address & SPACE_ADDRESS_MASK;

                //  Check if multisampling is enabled.
                if (!state.multiSampling)
                {
                    //  Write clear color.
                    //switch(state.colorBufferFormat)
                    switch(state.rtFormat[0])
                    {
                        case GPU_RGBA8888:

                            *((U64 *) &memory[address + 0]) = *((U64 *) &clearColor8bit[0]);
                            *((U64 *) &memory[address + 8]) = *((U64 *) &clearColor8bit[8]);
                            break;

                        case GPU_RG16F:

                            *((U32 *) &memory[address +  0]) = *((U32 *) &clearColor16bit[0]);
                            *((U32 *) &memory[address +  4]) = *((U32 *) &clearColor16bit[2]);
                            *((U32 *) &memory[address +  8]) = *((U32 *) &clearColor16bit[0]);
                            *((U32 *) &memory[address + 12]) = *((U32 *) &clearColor16bit[2]);
                            break;

                        case GPU_R32F:

                            *((U32 *) &memory[address +  0]) = *((U32 *) &clearColor32bit[0]);
                            *((U32 *) &memory[address +  4]) = *((U32 *) &clearColor32bit[0]);
                            *((U32 *) &memory[address +  8]) = *((U32 *) &clearColor32bit[0]);
                            *((U32 *) &memory[address + 12]) = *((U32 *) &clearColor32bit[0]);
                            break;

                        case GPU_RGBA16:

                            *((U64 *) &memory[address +  0]) = *((U64 *) &clearColor16bit[24]);
                            *((U64 *) &memory[address +  8]) = *((U64 *) &clearColor16bit[16]);
                            *((U64 *) &memory[address + 16]) = *((U64 *) &clearColor16bit[8]);
                            *((U64 *) &memory[address + 24]) = *((U64 *) &clearColor16bit[0]);
                            break;

                        case GPU_RGBA16F:

                            *((U64 *) &memory[address +  0]) = *((U64 *) &clearColor16bit[0]);
                            *((U64 *) &memory[address +  8]) = *((U64 *) &clearColor16bit[8]);
                            *((U64 *) &memory[address + 16]) = *((U64 *) &clearColor16bit[16]);
                            *((U64 *) &memory[address + 24]) = *((U64 *) &clearColor16bit[24]);
                            break;
                    }
                }
                else
                {
                    for(U32 s = 0; s < (STAMP_FRAGMENTS * state.msaaSamples); s++)
                    {
                        //  Write clear color.
                        //switch(state.colorBufferFormat)
                        switch(state.rtFormat[0])
                        {
                            case GPU_RGBA8888:

                                *((U32 *) &memory[address + s * 4]) = *((U32 *) &clearColor8bit[0]);
                                break;

                            case GPU_RG16F:

                                *((U32 *) &memory[address + s * 4]) = *((U32 *) &clearColor16bit[0]);
                                break;

                            case GPU_R32F:

                                *((U32 *) &memory[address + s * 4]) = *((U32 *) &clearColor32bit[0]);
                                break;

                            case GPU_RGBA16:
                            case GPU_RGBA16F:

                                *((U64 *) &memory[address + s * 8]) = *((U64 *) &clearColor16bit[0]);
                                break;
                        }
                    }
                }
            }
        }
    }
    
    GLOBAL_PROFILER_EXIT_REGION()
}


//  Converts color data from RGBA8 format to RGBA32F format.
void bmoGpuTop::colorRGBA8ToRGBA32F(U08 *in, Vec4FP32 *out)
{
    U32 i;

    //  Convert all pixels in the stamp.
    for(i = 0; i < STAMP_FRAGMENTS; i++)
    {
        //  Convert 8 bit normalized color components to 32 bit float point color components.
        out[i][0] = F32(in[i * 4]) * (1.0f / 255.0f);
        out[i][1] = F32(in[i * 4 + 1]) * (1.0f / 255.0f);
        out[i][2] = F32(in[i * 4 + 2]) * (1.0f / 255.0f);
        out[i][3] = F32(in[i * 4 + 3]) * (1.0f / 255.0f);
    }
}

//  Converts color data from RGBA16 format to RGBA32F format.
void bmoGpuTop::colorRGBA16ToRGBA32F(U08 *in, Vec4FP32 *out)
{
    U32 i;

    //  Convert all pixels in the stamp.
    for(i = 0; i < STAMP_FRAGMENTS; i++)
    {
        //  Convert 16 bit normalized color components to 32 bit float point color components.
        out[i][0] = F32(((U16 *) in)[i * 4 + 0]) * (1.0f / 65535.0f);
        out[i][1] = F32(((U16 *) in)[i * 4 + 1]) * (1.0f / 65535.0f);
        out[i][2] = F32(((U16 *) in)[i * 4 + 2]) * (1.0f / 65535.0f);
        out[i][3] = F32(((U16 *) in)[i * 4 + 3]) * (1.0f / 65535.0f);
    }
}

//  Converts color data in RGB32F format to RGBA8 format.
void bmoGpuTop::colorRGBA32FToRGBA8(Vec4FP32 *in, U08 *out)
{
    U32 i;

    //  Convert all pixels in the stamp.
    for(i = 0; i < STAMP_FRAGMENTS; i++)
    {
        //  Convert 32bit float point color components to 8 bit normalized color components.
        out[i * 4] = U08(255.0f * GPU_CLAMP(in[i][0], 0.0f, 1.0f));
        out[i * 4 + 1] = U08(255.0f * GPU_CLAMP(in[i][1], 0.0f, 1.0f));
        out[i * 4 + 2] = U08(255.0f * GPU_CLAMP(in[i][2], 0.0f, 1.0f));
        out[i * 4 + 3] = U08(255.0f * GPU_CLAMP(in[i][3], 0.0f, 1.0f));
    }
}

//  Converts color data in RGB32F format to RGBA16 format.
void bmoGpuTop::colorRGBA32FToRGBA16(Vec4FP32 *in, U08 *out)
{
    U32 i;

    //  Convert all pixels in the stamp.
    for(i = 0; i < STAMP_FRAGMENTS; i++)
    {
        //  Convert 32bit float point color components to 16 bit normalized color components.
        ((U16 *) out)[i * 4 + 0] = U16(65535.0f * GPU_CLAMP(in[i][0], 0.0f, 1.0f));
        ((U16 *) out)[i * 4 + 1] = U16(65535.0f * GPU_CLAMP(in[i][1], 0.0f, 1.0f));
        ((U16 *) out)[i * 4 + 2] = U16(65535.0f * GPU_CLAMP(in[i][2], 0.0f, 1.0f));
        ((U16 *) out)[i * 4 + 3] = U16(65535.0f * GPU_CLAMP(in[i][3], 0.0f, 1.0f));
    }
}

//  Converts color data from RGBA16F format to RGBA32F format.
void bmoGpuTop::colorRGBA16FToRGBA32F(U08 *in, Vec4FP32 *out)
{
    U32 i;

    //  Convert all pixels in the stamp.
    for(i = 0; i < STAMP_FRAGMENTS; i++)
    {
        //  Convert 16 bit float point color components to 32 bit float point color components.
        out[i][0] = GPUMath::convertFP16ToFP32(((U16 *) in)[i * 4]);
        out[i][1] = GPUMath::convertFP16ToFP32(((U16 *) in)[i * 4 + 1]);
        out[i][2] = GPUMath::convertFP16ToFP32(((U16 *) in)[i * 4 + 2]);
        out[i][3] = GPUMath::convertFP16ToFP32(((U16 *) in)[i * 4 + 3]);
    }
}

//  Converts color data from RG16F format to RGBA32F format.
void bmoGpuTop::colorRG16FToRGBA32F(U08 *in, Vec4FP32 *out)
{
    U32 i;

    //  Convert all pixels in the stamp.
    for(i = 0; i < STAMP_FRAGMENTS; i++)
    {
        //  Convert 16 bit float point color components to 32 bit float point color components.
        out[i][0] = GPUMath::convertFP16ToFP32(((U16 *) in)[i * 2]);
        out[i][1] = GPUMath::convertFP16ToFP32(((U16 *) in)[i * 2 + 1]);
        out[i][2] = 0.0f;
        out[i][3] = 1.0f;
    }
}

//  Converts color data in RGB32F format to RG16F format.
void bmoGpuTop::colorRGBA32FToRG16F(Vec4FP32 *in, U08 *out)
{
    U32 i;

    //  Convert all pixels in the stamp.
    for(i = 0; i < STAMP_FRAGMENTS; i++)
    {
        //  Convert 32 bit float point color components to 16 bit float point color components.
        ((U16 *) out)[i * 2]     = GPUMath::convertFP32ToFP16(in[i][0]);
        ((U16 *) out)[i * 2 + 1] = GPUMath::convertFP32ToFP16(in[i][1]);
    }
}

//  Converts color data from R32F format to RGBA32F format.
void bmoGpuTop::colorR32FToRGBA32F(U08 *in, Vec4FP32 *out)
{
    U32 i;

    //  Convert all pixels in the stamp.
    for(i = 0; i < STAMP_FRAGMENTS; i++)
    {
        //  Convert 16 bit float point color components to 32 bit float point color components.
        out[i][0] = ((F32 *) in)[i];
        out[i][1] = 0.0f;
        out[i][2] = 0.0f;
        out[i][3] = 1.0f;
    }
}

//  Converts color data in RGB32F format to R32F format.
void bmoGpuTop::colorRGBA32FToR32F(Vec4FP32 *in, U08 *out)
{
    U32 i;

    //  Convert all pixels in the stamp.
    for(i = 0; i < STAMP_FRAGMENTS; i++)
    {
        //  Convert 32 bit float point color components to 16 bit float point color components.
        ((F32 *) out)[i] = in[i][0];
    }
}

//  Converts color data in RGB32F format to RGBA8 format.  */
void bmoGpuTop::colorRGBA32FToRGBA16F(Vec4FP32 *in, U08 *out)
{
    U32 i;

    //  Convert all pixels in the stamp.
    for(i = 0; i < STAMP_FRAGMENTS; i++)
    {
        //  Convert 32 bit float point color components to 16 bit float point color components.
        ((U16 *) out)[i * 4]     = GPUMath::convertFP32ToFP16(in[i][0]);
        ((U16 *) out)[i * 4 + 1] = GPUMath::convertFP32ToFP16(in[i][1]);
        ((U16 *) out)[i * 4 + 2] = GPUMath::convertFP32ToFP16(in[i][2]);
        ((U16 *) out)[i * 4 + 3] = GPUMath::convertFP32ToFP16(in[i][3]);
    }
}

#define GAMMA(x) F32(GPU_POWER(F64(x), F64(1.0f / 2.2f)))
#define LINEAR(x) F32(GPU_POWER(F64(x), F64(2.2f)))
//#define GAMMA(x) (x)

//  Converts color data from linear space to sRGB space.
void bmoGpuTop::colorLinearToSRGB(Vec4FP32 *in)
{
    for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
    {
        //  NOTE: Alpha shouldn't be affected by the color space conversion.

        //  Convert from linear space to sRGB space.
        in[f][0] = GAMMA(in[f][0]);
        in[f][1] = GAMMA(in[f][1]);
        in[f][2] = GAMMA(in[f][2]);
    }
}

//  Converts color data from sRGB space to linear space.
void bmoGpuTop::colorSRGBToLinear(Vec4FP32 *in)
{
    for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
    {
        //  NOTE: Alpha shouldn't be affected by the color space conversion.

        //  Convert from sRGB space to linear space.
        in[f][0] = LINEAR(in[f][0]);
        in[f][1] = LINEAR(in[f][1]);
        in[f][2] = LINEAR(in[f][2]);
    }
}

//  Writes the current color buffer as a ppm file.
void bmoGpuTop::dumpFrame(char *filename, U32 rt, bool dumpAlpha)
{
    U32 address;
    S32 x,y;

    static U08 *data = 0;
    static U32 dataSize = 0;
    
    //  Check if the buffer is large enough.
    if (dataSize < (state.displayResX * state.displayResY * 4))
    {
        //  Check if the old buffer must be deleted.
        if (data != NULL)
            delete[] data;
            
        //  Set the new buffer size.            
        dataSize = state.displayResX * state.displayResY * 4;
        
        //  Create the new buffer.
        data = new U08[dataSize];
    }

/*#else

    FILE *fout;

    //  Add file extension.
    char filenameAux[256];
    sprintf(filenameAux, "%s.ppm", filename);
    
    //  Open/Create the file for the current frame.
    fout = fopen(filenameAux, "wb");

    //  Check if the file was correctly created.
    CG_ASSERT_COND(!(fout == NULL), "Error creating frame color output file.");
    //  Write file header.

    //  Write magic number.
    fprintf(fout, "P6");

    //  Write frame size.
    fprintf(fout, "%d %d", state.displayResX, state.displayResY);

    //  Write color component maximum value.
    fprintf(fout, "255");

#endif*/

    U32 samples = state.multiSampling ? state.msaaSamples : 1;
    U32 bytesPixel;

    switch(state.rtFormat[rt])
    {
        case GPU_RGBA8888:
        case GPU_RG16F:
        case GPU_R32F:
            bytesPixel = 4;
            break;
        case GPU_RGBA16:
        case GPU_RGBA16F:
            bytesPixel = 8;
            break;
    }

    pixelMapper[rt].setupDisplay(state.displayResX, state.displayResY, STAMP_WIDTH, STAMP_WIDTH,
                                ArchConf.ras.genWidth / STAMP_WIDTH, ArchConf.ras.genHeight / STAMP_HEIGHT,
                                ArchConf.ras.scanWidth / ArchConf.ras.genWidth, ArchConf.ras.scanHeight / ArchConf.ras.genHeight,
                                ArchConf.ras.overScanWidth, ArchConf.ras.overScanHeight,
                                samples, bytesPixel);

    U08 red;
    U08 green;
    U08 blue;
    U08 alpha;

    U08 *memory = selectMemorySpace(state.rtAddress[rt]);

    //  Check if multisampling is enabled.
    if (!state.multiSampling)
    {
        S32 top = state.d3d9PixelCoordinates ? 0 : (state.displayResY - 1);
        S32 bottom = state.d3d9PixelCoordinates ? (state.displayResY - 1) : 0;
        S32 nextLine = state.d3d9PixelCoordinates ? +1 : -1;

        //  Do this for the whole picture now.
        for (y = top; y != (bottom + nextLine); y = y + nextLine)
        {
            for (x = 0; x < S32(state.displayResX); x++)
            {
                //  Calculate address from pixel position.
                address = pixelMapper[rt].computeAddress(x, y);
                address += state.rtAddress[rt];

                //  Get address inside the memory space.
                address = address & SPACE_ADDRESS_MASK;

                //  Convert data from the color color buffer to 8-bit PPM format.
                switch(state.rtFormat[rt])
                {
                    case GPU_RGBA8888:
                        red   = memory[address];
                        green = memory[address + 1];
                        blue  = memory[address + 2];
                        alpha = memory[address + 3];
                        break;

                    case GPU_RG16F:
                        red   = U08(GPU_MIN(GPU_MAX(GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + 0])), 0.0f), 1.0f) * 255.0f);
                        green = U08(GPU_MIN(GPU_MAX(GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + 2])), 0.0f), 1.0f) * 255.0f);
                        blue  = 0;
                        alpha = 0;
                        break;

                    case GPU_R32F:
                        red   = U08(GPU_MIN(GPU_MAX(*((F32 *) &memory[address]), 0.0f), 1.0f) * 255.0f);
                        green = 0;
                        blue  = 0;
                        alpha = 0;
                        break;

                    case GPU_RGBA16:
                        red   = U08(GPU_MIN(GPU_MAX((F32(*((U16 *) &memory[address + 0])) / 65535.0f), 0.0f), 1.0f) * 255.0f);
                        green = U08(GPU_MIN(GPU_MAX((F32(*((U16 *) &memory[address + 2])) / 65535.0f), 0.0f), 1.0f) * 255.0f);
                        blue  = U08(GPU_MIN(GPU_MAX((F32(*((U16 *) &memory[address + 4])) / 65535.0f), 0.0f), 1.0f) * 255.0f);
                        alpha = U08(GPU_MIN(GPU_MAX((F32(*((U16 *) &memory[address + 6])) / 65535.0f), 0.0f), 1.0f) * 255.0f);
                        break;

                    case GPU_RGBA16F:
                        red   = U08(GPU_MIN(GPU_MAX(GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + 0])), 0.0f), 1.0f) * 255.0f);
                        green = U08(GPU_MIN(GPU_MAX(GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + 2])), 0.0f), 1.0f) * 255.0f);
                        blue  = U08(GPU_MIN(GPU_MAX(GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + 4])), 0.0f), 1.0f) * 255.0f);
                        alpha = U08(GPU_MIN(GPU_MAX(GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + 6])), 0.0f), 1.0f) * 255.0f);
                        break;
                }

                if (!dumpAlpha)
                {
                    U32 yImage = state.d3d9PixelCoordinates ? y : (state.displayResY - 1) - y;
                    data[(yImage * state.displayResX + x) * 4 + 2] = red;
                    data[(yImage * state.displayResX + x) * 4 + 1] = green;
                    data[(yImage * state.displayResX + x) * 4 + 0] = blue;
                    //data[(yImage * state.displayResX + x) * 4 + 3] = alpha;
                    data[(yImage * state.displayResX + x) * 4 + 3] = 255;
                }
                else
                {
                    U32 yImage = state.d3d9PixelCoordinates ? y : (state.displayResY - 1) - y;
                    data[(yImage * state.displayResX + x) * 4 + 0] = 
                    data[(yImage * state.displayResX + x) * 4 + 1] = 
                    data[(yImage * state.displayResX + x) * 4 + 2] = 
                    data[(yImage * state.displayResX + x) * 4 + 3] = alpha;
                }
            }
        }
    }
    else
    {
        Vec4FP32 sampleColors[MAX_MSAA_SAMPLES];
        Vec4FP32 resolvedColor;
        Vec4FP32 referenceColor;
        Vec4FP32 currentColor;

        S32 top = state.d3d9PixelCoordinates ? 0 : (state.displayResY - 1);
        S32 bottom = state.d3d9PixelCoordinates ? (state.displayResY - 1) : 0;
        S32 nextLine = state.d3d9PixelCoordinates ? +1 : -1;

        //  Do this for the whole picture now.
        for (y = top; y != (bottom + nextLine); y = y + nextLine)
        {
            for(x = 0; x < S32(state.displayResX); x++)
            {

                address = pixelMapper[rt].computeAddress(x, y);

                //  Calculate address for the first sample in the pixel.
                address += state.rtAddress[rt];

                //  Zero resolved color.
                resolvedColor[0] = 0.0f;
                resolvedColor[1] = 0.0f;
                resolvedColor[2] = 0.0f;
                resolvedColor[3] = 0.0f;

                Vec4FP32 referenceColor;

                //  Convert data from the color color buffer 32 bit fp internal format.
                switch(state.rtFormat[rt])
                {
                    case GPU_RGBA8888:
                        referenceColor[0] = F32(memory[address + 0]) / 255.0f;
                        referenceColor[1] = F32(memory[address + 1]) / 255.0f;
                        referenceColor[2] = F32(memory[address + 2]) / 255.0f;
                        referenceColor[3] = F32(memory[address + 3]) / 255.0f;
                        break;

                    case GPU_RG16F:
                        referenceColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + 0]));
                        referenceColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + 2]));
                        referenceColor[2] = 0.0f;
                        referenceColor[3] = 0.0f;
                        break;

                    case GPU_R32F:
                        referenceColor[0] = *((F32 *) &memory[address]);
                        referenceColor[1] = 0.0f;
                        referenceColor[2] = 0.0f;
                        referenceColor[3] = 0.0f;
                        break;

                    case GPU_RGBA16:
                        referenceColor[0] = F32(*((U16 *) &memory[address + 0])) / 65535.0f;
                        referenceColor[1] = F32(*((U16 *) &memory[address + 2])) / 65535.0f;
                        referenceColor[2] = F32(*((U16 *) &memory[address + 6])) / 65535.0f;
                        referenceColor[3] = F32(*((U16 *) &memory[address + 8])) / 65535.0f;
                        break;

                    case GPU_RGBA16F:
                        referenceColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + 0]));
                        referenceColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + 2]));
                        referenceColor[2] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + 4]));
                        referenceColor[3] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + 6]));
                        break;
                }

                bool fullCoverage = true;

                //  Accumulate the color for all the samples in the pixel
                for(U32 i = 0; i < state.msaaSamples; i++)
                {
                    //  Convert data from the color color buffer 32 bit fp internal format.
                    switch(state.rtFormat[rt])
                    {
                        case GPU_RGBA8888:
                            currentColor[0] = F32(memory[address + i * 4 + 0]) / 255.0f;
                            currentColor[1] = F32(memory[address + i * 4 + 1]) / 255.0f;
                            currentColor[2] = F32(memory[address + i * 4 + 2]) / 255.0f;
                            currentColor[3] = F32(memory[address + i * 4 + 3]) / 255.0f;
                            break;

                        case GPU_RG16F:
                            currentColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + i * 4 + 0]));
                            currentColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + i * 4 + 2]));
                            currentColor[2] = 0.0f;
                            currentColor[3] = 0.0f;
                            break;

                        case GPU_R32F:
                            currentColor[0] = *((F32 *) &memory[address + i * 4]);
                            currentColor[1] = 0.0f;
                            currentColor[2] = 0.0f;
                            currentColor[3] = 0.0f;
                            break;

                        case GPU_RGBA16:
                            currentColor[0] = F32(*((U16 *) &memory[address + i * 8 + 0])) / 65535.0f;
                            currentColor[1] = F32(*((U16 *) &memory[address + i * 8 + 2])) / 65535.0f;
                            currentColor[2] = F32(*((U16 *) &memory[address + i * 8 + 4])) / 65535.0f;
                            currentColor[3] = F32(*((U16 *) &memory[address + i * 8 + 6])) / 65535.0f;
                            break;


                        case GPU_RGBA16F:
                            currentColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + i * 8 + 0]));
                            currentColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + i * 8 + 2]));
                            currentColor[2] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + i * 8 + 4]));
                            currentColor[3] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + i * 8 + 6]));
                            break;
                    }

                    sampleColors[i][0] = currentColor[0];
                    sampleColors[i][1] = currentColor[1];
                    sampleColors[i][2] = currentColor[2];
                    sampleColors[i][3] = currentColor[3];

                    resolvedColor[0] += LINEAR(currentColor[0]);
                    resolvedColor[1] += LINEAR(currentColor[1]);
                    resolvedColor[2] += LINEAR(currentColor[2]);
                    //resolvedColor[3] += LINEAR(currentColor[3]);
                    resolvedColor[3] += currentColor[3];

                    fullCoverage = fullCoverage && (referenceColor[0] == currentColor[0])
                                                && (referenceColor[1] == currentColor[1])
                                                && (referenceColor[2] == currentColor[2]);
                }


                //  Check if there is a single sample for the pixel
                if (fullCoverage)
                {
                    //  Convert from RGBA32F (internal format) to RGBA8 (PPM format)
                    red   = U08(GPU_MIN(GPU_MAX(referenceColor[0], 0.0f), 1.0f) * 255.0f);
                    green = U08(GPU_MIN(GPU_MAX(referenceColor[1], 0.0f), 1.0f) * 255.0f);
                    blue  = U08(GPU_MIN(GPU_MAX(referenceColor[2], 0.0f), 1.0f) * 255.0f);
                    alpha = U08(GPU_MIN(GPU_MAX(referenceColor[3], 0.0f), 1.0f) * 255.0f);

                }
                else
                {
                    resolvedColor[0] = GAMMA(resolvedColor[0] / F32(state.msaaSamples));
                    resolvedColor[1] = GAMMA(resolvedColor[1] / F32(state.msaaSamples));
                    resolvedColor[2] = GAMMA(resolvedColor[2] / F32(state.msaaSamples));
                    //resolvedColor[3] = GAMMA(resolvedColor[3] / F32(state.msaaSamples));
                    resolvedColor[3] = resolvedColor[3] / F32(state.msaaSamples);

                    //  Convert from RGBA32F (internal format) to RGBA8 (PPM format)
                    red   = U08(GPU_MIN(GPU_MAX(resolvedColor[0], 0.0f), 1.0f) * 255.0f);
                    green = U08(GPU_MIN(GPU_MAX(resolvedColor[1], 0.0f), 1.0f) * 255.0f);
                    blue  = U08(GPU_MIN(GPU_MAX(resolvedColor[2], 0.0f), 1.0f) * 255.0f);
                    alpha = U08(GPU_MIN(GPU_MAX(resolvedColor[3], 0.0f), 1.0f) * 255.0f);
                }

                if (!dumpAlpha)
                {
                    U32 yImage = state.d3d9PixelCoordinates ? y : (state.displayResY - 1) - y;
                    data[(yImage * state.displayResX + x) * 4 + 2] = red;
                    data[(yImage * state.displayResX + x) * 4 + 1] = green;
                    data[(yImage * state.displayResX + x) * 4 + 0] = blue;
                    //data[(yImage * state.displayResX + x) * 4 + 3] = alpha;
                    data[(yImage * state.displayResX + x) * 4 + 3] = 255;
                }
                else
                {
                    U32 yImage = state.d3d9PixelCoordinates ? y : (state.displayResY - 1) - y;
                    data[(yImage * state.displayResX + x) * 4 + 0] = 
                    data[(yImage * state.displayResX + x) * 4 + 1] = 
                    data[(yImage * state.displayResX + x) * 4 + 2] = 
                    data[(yImage * state.displayResX + x) * 4 + 3] = alpha;
                }
            }
        }
    }

    ImageSaver::getInstance().savePNG(filename, state.displayResX, state.displayResY, data);
}

//  Writes the current depth buffer as a ppm file.
void bmoGpuTop::dumpDepthBuffer(char *filename)
{
    U32 address;
    S32 x,y;

    static U08 *depthData = 0;
    static U32 depthDataSize = 0;
    
    //  Check if the buffer is large enough.
    if (depthDataSize < (state.displayResX * state.displayResY * 4))
    {
        //  Check if the old buffer must be deleted.
        if (depthData != NULL)
            delete[] depthData;
            
        //  Set the new buffer size.            
        depthDataSize = state.displayResX * state.displayResY * 4;
        
        //  Create the new buffer.
        depthData = new U08[depthDataSize];
    }

/*#else

    FILE *fout;
    
    //  Add file extension.
    char filenameAux[256];
    sprintf(filenameAux, "%s.ppm", filename);

    //  Open/Create the file for the current frame.
    fout = fopen(filename, "wb");

    //  Check if the file was correctly created.
    CG_ASSERT_COND(!(fout == NULL), "Error creating frame color output file.");
    //  Write file header.

    //  Write magic number.
    fprintf(fout, "P6");

    //  Write frame size.
    fprintf(fout, "%d %d", state.displayResX, state.displayResY);

    //  Write color component maximum value.
    fprintf(fout, "255");

#endif*/

    U32 samples = state.multiSampling ? state.msaaSamples : 1;
    U32 bytesPixel;

    bytesPixel = 4;

    zPixelMapper.setupDisplay(state.displayResX, state.displayResY, STAMP_WIDTH, STAMP_WIDTH,
                             ArchConf.ras.genWidth / STAMP_WIDTH, ArchConf.ras.genHeight / STAMP_HEIGHT,
                             ArchConf.ras.scanWidth / ArchConf.ras.genWidth, ArchConf.ras.scanHeight / ArchConf.ras.genHeight,
                             ArchConf.ras.overScanWidth, ArchConf.ras.overScanHeight,
                             samples, bytesPixel);

    U08 red;
    U08 green;
    U08 blue;
    //U08 zval;

    U08 *memory = selectMemorySpace(state.zStencilBufferBaseAddr);

    //  Check if multisampling is enabled.
    if (!state.multiSampling)
    {
        S32 top = state.d3d9PixelCoordinates ? 0 : (state.displayResY - 1);
        S32 bottom = state.d3d9PixelCoordinates ? (state.displayResY - 1) : 0;
        S32 nextLine = state.d3d9PixelCoordinates ? +1 : -1;

        //  Do this for the whole picture now.
        for (y = top; y != (bottom + nextLine); y = y + nextLine)
        {
            for (x = 0; x < S32(state.displayResX); x++)
            {
                //  Calculate address from pixel position.
                address = zPixelMapper.computeAddress(x, y);
                address += state.zStencilBufferBaseAddr;

                //  Get address inside the memory space.
                address = address & SPACE_ADDRESS_MASK;

                //red   = memory[address];
                //green = memory[address + 1];
                //blue  = memory[address + 2];

                // Invert for better reading with irfan view.
                red   = memory[address + 2];
                green = memory[address + 1];
                blue  = memory[address + 0];
                //zval = U08(F32(*((U32 *)&(memory[address + 0]))  & 0x00FFFFFF) / 16777215.0f * 255.f);  //F32(*((U32 *) data) & 0x00FFFFFF) / 16777215.0f;

                U32 yImage = state.d3d9PixelCoordinates ? y : (state.displayResY - 1) - y;
                depthData[(yImage * state.displayResX + x) * 4 + 2] = red;
                depthData[(yImage * state.displayResX + x) * 4 + 1] = green;
                depthData[(yImage * state.displayResX + x) * 4 + 0] = blue;
                //depthData[(yImage * state.displayResX + x) * 4 + 3] = 0;
                depthData[(yImage * state.displayResX + x) * 4 + 3] = 255;
            }
        }
    }
    else
    {
        CG_ASSERT("MSAA not implemented.");

        S32 top = state.d3d9PixelCoordinates ? 0 : (state.displayResY - 1);
        S32 bottom = state.d3d9PixelCoordinates ? (state.displayResY - 1) : 0;
        S32 nextLine = state.d3d9PixelCoordinates ? +1 : -1;

        //  Do this for the whole picture now.
        for (y = top; y != (bottom + nextLine); y = y + nextLine)
        {
            for(x = 0; x < S32(state.displayResX); x++)
            {
                Vec4FP32 resolvedColor;

                U32 sampleX;
                U32 sampleY;

                sampleX = x - GPU_MOD(x, STAMP_WIDTH);
                sampleY = y - GPU_MOD(y, STAMP_HEIGHT);

                address = pixelMapper[0].computeAddress(sampleX, sampleY);

                //  Calculate address for the first sample in the pixel.
                address = address + state.msaaSamples * (GPU_MOD(y, STAMP_HEIGHT) * STAMP_WIDTH + GPU_MOD(x, STAMP_WIDTH)) * bytesPixel;
                address += state.backBufferBaseAddr;

                //  Zero resolved color.
                resolvedColor[0] = 0.0f;
                resolvedColor[1] = 0.0f;
                resolvedColor[2] = 0.0f;

                Vec4FP32 referenceColor;

                //  Convert data from the color color buffer 32 bit fp internal format.
                switch(state.colorBufferFormat)
                {
                    case GPU_RGBA8888:
                        referenceColor[0] = F32(memory[address + 0]) / 255.0f;
                        referenceColor[1] = F32(memory[address + 1]) / 255.0f;
                        referenceColor[2] = F32(memory[address + 2]) / 255.0f;
                        break;

                    case GPU_RGBA16:
                        referenceColor[0] = F32(*((U16 *) &memory[address + 0])) / 65535.0f;
                        referenceColor[1] = F32(*((U16 *) &memory[address + 2])) / 65535.0f;
                        referenceColor[2] = F32(*((U16 *) &memory[address + 4])) / 65535.0f;
                        referenceColor[3] = F32(*((U16 *) &memory[address + 6])) / 65535.0f;
                        break;

                    case GPU_RGBA16F:
                        referenceColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + 0]));
                        referenceColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + 2]));
                        referenceColor[2] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + 4]));
                        break;
                }

                bool fullCoverage = true;

                //  Accumulate the color for all the samples in the pixel
                for(U32 i = 0; i < state.msaaSamples; i++)
                {
                    Vec4FP32 currentColor;

                    //  Convert data from the color color buffer 32 bit fp internal format.
                    switch(state.colorBufferFormat)
                    {
                        case GPU_RGBA8888:
                            currentColor[0] = F32(memory[address + i * 4 + 0]) / 255.0f;
                            currentColor[1] = F32(memory[address + i * 4 + 1]) / 255.0f;
                            currentColor[2] = F32(memory[address + i * 4 + 2]) / 255.0f;
                            break;

                        case GPU_RGBA16:
                            currentColor[0] = F32(*((U16 *) &memory[address + i * 8 + 0])) / 65535.0f;
                            currentColor[1] = F32(*((U16 *) &memory[address + i * 8 + 2])) / 65535.0f;
                            currentColor[2] = F32(*((U16 *) &memory[address + i * 8 + 4])) / 65535.0f;
                            break;

                        case GPU_RGBA16F:
                            currentColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + i * 8 + 0]));
                            currentColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + i * 8 + 2]));
                            currentColor[2] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + i * 8 + 4]));
                            break;
                    }

                    resolvedColor[0] += currentColor[0];
                    resolvedColor[1] += currentColor[1];
                    resolvedColor[2] += currentColor[2];

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

                    U32 yImage = state.d3d9PixelCoordinates ? y : (state.displayResY - 1) - y;
                    depthData[(yImage * state.displayResX + x) * 4 + 2] = red;
                    depthData[(yImage * state.displayResX + x) * 4 + 1] = green;
                    depthData[(yImage * state.displayResX + x) * 4 + 0] = blue;
                    depthData[(yImage * state.displayResX + x) * 4 + 3] = 0;
                }
                else
                {
                    //  Resolve color as the average of all the sample colors.
                    resolvedColor[0] = GAMMA(resolvedColor[0] / F32(state.msaaSamples));
                    resolvedColor[1] = GAMMA(resolvedColor[1] / F32(state.msaaSamples));
                    resolvedColor[2] = GAMMA(resolvedColor[2] / F32(state.msaaSamples));

                    U32 yImage = state.d3d9PixelCoordinates ? y : (state.displayResY - 1) - y;
                    depthData[(yImage * state.displayResX + x) * 4 + 2] = red;
                    depthData[(yImage * state.displayResX + x) * 4 + 1] = green;
                    depthData[(yImage * state.displayResX + x) * 4 + 0] = blue;
                    depthData[(yImage * state.displayResX + x) * 4 + 3] = 0;
                }
            }
        }
    }

    ImageSaver::getInstance().savePNG(filename, state.displayResX, state.displayResY, depthData);
}

//  Writes the current depth buffer as a ppm file.
void bmoGpuTop::dumpStencilBuffer(char *filename)
{
    U32 address;
    S32 x,y;

    static U08 *stencilData = 0;
    static U32 stencilDataSize = 0;
    
    //  Check if the buffer is large enough.
    if (stencilDataSize < (state.displayResX * state.displayResY * 4))
    {
        //  Check if the old buffer must be deleted.
        if (stencilData != NULL)
            delete[] stencilData;
            
        //  Set the new buffer size.            
        stencilDataSize = state.displayResX * state.displayResY * 4;
        
        //  Create the new buffer.
        stencilData = new U08[stencilDataSize];
    }

/*#else

    FILE *fout;
    
    //  Add file extension.
    char filenameAux[256];
    sprintf(filenameAux, "%s.ppm", filename);
    //  Open/Create the file for the current frame.
    fout = fopen(filename, "wb");

    //  Check if the file was correctly created.
    CG_ASSERT_COND(!(fout == NULL), "Error creating frame color output file.");
    //  Write file header.

    //  Write magic number.
    fprintf(fout, "P6");

    //  Write frame size.
    fprintf(fout, "%d %d", state.displayResX, state.displayResY);

    //  Write color component maximum value.
    fprintf(fout, "255");

#endif*/

    U32 samples = state.multiSampling ? state.msaaSamples : 1;
    U32 bytesPixel;

    bytesPixel = 4;

    zPixelMapper.setupDisplay(state.displayResX, state.displayResY, STAMP_WIDTH, STAMP_WIDTH,
                             ArchConf.ras.genWidth / STAMP_WIDTH, ArchConf.ras.genHeight / STAMP_HEIGHT,
                             ArchConf.ras.scanWidth / ArchConf.ras.genWidth, ArchConf.ras.scanHeight / ArchConf.ras.genHeight,
                             ArchConf.ras.overScanWidth, ArchConf.ras.overScanHeight,
                             samples, bytesPixel);

    U08 red;
    U08 green;
    U08 blue;

    U08 *memory = selectMemorySpace(state.zStencilBufferBaseAddr);

    //  Check if multisampling is enabled.
    if (!state.multiSampling)
    {
        S32 top = state.d3d9PixelCoordinates ? 0 : (state.displayResY - 1);
        S32 bottom = state.d3d9PixelCoordinates ? (state.displayResY - 1) : 0;
        S32 nextLine = state.d3d9PixelCoordinates ? +1 : -1;

        //  Do this for the whole picture now.
        for (y = top; y != (bottom + nextLine); y = y + nextLine)
        {
            for (x = 0; x < S32(state.displayResX); x++)
            {
                //  Calculate address from pixel position.
                address = zPixelMapper.computeAddress(x, y);
                address += state.zStencilBufferBaseAddr;

                //  Get address inside the memory space.
                address = address & SPACE_ADDRESS_MASK;

                //  Put some color to make small differences more easy to discover.
                red   = memory[address + 3];
                green = red & 0xF0;
                blue  = (red & 0x0F) << 4;

                U32 yImage = state.d3d9PixelCoordinates ? y : (state.displayResY - 1) - y;
                stencilData[(yImage * state.displayResX + x) * 4 + 2] = red;
                stencilData[(yImage * state.displayResX + x) * 4 + 1] = green;
                stencilData[(yImage * state.displayResX + x) * 4 + 0] = blue;
                //stencilData[(yImage * state.displayResX + x) * 4 + 3] = 0;
                stencilData[(yImage * state.displayResX + x) * 4 + 3] = 255;
            }
        }
    }
    else
    {
        CG_ASSERT("MSAA not implemented.");

        S32 top = state.d3d9PixelCoordinates ? 0 : (state.displayResY - 1);
        S32 bottom = state.d3d9PixelCoordinates ? (state.displayResY - 1) : 0;
        S32 nextLine = state.d3d9PixelCoordinates ? +1 : -1;

        //  Do this for the whole picture now.
        for (y = top; y != (bottom + nextLine); y = y + nextLine)
        {
            for(x = 0; x < S32(state.displayResX); x++)
            {
                Vec4FP32 resolvedColor;

                U32 sampleX;
                U32 sampleY;

                sampleX = x - GPU_MOD(x, STAMP_WIDTH);
                sampleY = y - GPU_MOD(y, STAMP_HEIGHT);

                address = pixelMapper[0].computeAddress(sampleX, sampleY);

                //  Calculate address for the first sample in the pixel.
                address = address + state.msaaSamples * (GPU_MOD(y, STAMP_HEIGHT) * STAMP_WIDTH + GPU_MOD(x, STAMP_WIDTH)) * bytesPixel;
                address += state.backBufferBaseAddr;

                //  Zero resolved color.
                resolvedColor[0] = 0.0f;
                resolvedColor[1] = 0.0f;
                resolvedColor[2] = 0.0f;

                Vec4FP32 referenceColor;

                //  Convert data from the color color buffer 32 bit fp internal format.
                switch(state.colorBufferFormat)
                {
                    case GPU_RGBA8888:
                        referenceColor[0] = F32(memory[address + 0]) / 255.0f;
                        referenceColor[1] = F32(memory[address + 1]) / 255.0f;
                        referenceColor[2] = F32(memory[address + 2]) / 255.0f;
                        break;

                    case GPU_RGBA16:
                        referenceColor[0] = F32(*((U16 *) &memory[address + 0])) / 65535.0f;
                        referenceColor[1] = F32(*((U16 *) &memory[address + 2])) / 65535.0f;
                        referenceColor[2] = F32(*((U16 *) &memory[address + 4])) / 65535.0f;
                        break;

                    case GPU_RGBA16F:
                        referenceColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + 0]));
                        referenceColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + 2]));
                        referenceColor[2] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + 4]));
                        break;
                }

                bool fullCoverage = true;

                //  Accumulate the color for all the samples in the pixel
                for(U32 i = 0; i < state.msaaSamples; i++)
                {
                    Vec4FP32 currentColor;

                    //  Convert data from the color color buffer 32 bit fp internal format.
                    switch(state.colorBufferFormat)
                    {
                        case GPU_RGBA8888:
                            currentColor[0] = F32(memory[address + i * 4 + 0]) / 255.0f;
                            currentColor[1] = F32(memory[address + i * 4 + 1]) / 255.0f;
                            currentColor[2] = F32(memory[address + i * 4 + 2]) / 255.0f;
                            break;

                        case GPU_RGBA16:
                            currentColor[0] = F32(*((U16 *) &memory[address + i * 8 + 0])) / 65535.0f;
                            currentColor[1] = F32(*((U16 *) &memory[address + i * 8 + 2])) / 65535.0f;
                            currentColor[2] = F32(*((U16 *) &memory[address + i * 8 + 4])) / 65535.0f;
                            break;

                        case GPU_RGBA16F:
                            currentColor[0] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + i * 8 + 0]));
                            currentColor[1] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + i * 8 + 2]));
                            currentColor[2] = GPUMath::convertFP16ToFP32(*((U16 *) &memory[address + i * 8 + 4]));
                            break;
                    }

                    resolvedColor[0] += currentColor[0];
                    resolvedColor[1] += currentColor[1];
                    resolvedColor[2] += currentColor[2];

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

                    U32 yImage = state.d3d9PixelCoordinates ? y : (state.displayResY - 1) - y;
                    stencilData[(yImage * state.displayResX + x) * 4 + 2] = red;
                    stencilData[(yImage * state.displayResX + x) * 4 + 1] = green;
                    stencilData[(yImage * state.displayResX + x) * 4 + 0] = blue;
                    stencilData[(yImage * state.displayResX + x) * 4 + 3] = 0;
                }
                else
                {
                    //  Resolve color as the average of all the sample colors.
                    resolvedColor[0] = GAMMA(resolvedColor[0] / F32(state.msaaSamples));
                    resolvedColor[1] = GAMMA(resolvedColor[1] / F32(state.msaaSamples));
                    resolvedColor[2] = GAMMA(resolvedColor[2] / F32(state.msaaSamples));

                    U32 yImage = state.d3d9PixelCoordinates ? y : (state.displayResY - 1) - y;
                    stencilData[(yImage * state.displayResX + x) * 4 + 2] = red;
                    stencilData[(yImage * state.displayResX + x) * 4 + 1] = green;
                    stencilData[(yImage * state.displayResX + x) * 4 + 0] = blue;
                    stencilData[(yImage * state.displayResX + x) * 4 + 3] = 0;
                }
            }
        }
    }

    ImageSaver::getInstance().savePNG(filename, state.displayResX, state.displayResY, stencilData);
}


//  Load the current vertex program in the shader behaviorModel.
void bmoGpuTop::loadVertexProgram()
{
    U08 *code;

    U08 *memory = selectMemorySpace(state.vertexProgramAddr);
    U32 address = state.vertexProgramAddr & SPACE_ADDRESS_MASK;

    code = &memory[address];

    GPU_DEBUG(
        CG_INFO("Loading vertex program from address %08x to PC %04x size %d", state.vertexProgramAddr,
            state.vertexProgramStartPC, state.vertexProgramSize);
        bool endProgram = false;
        U32 nopsAtTheEnd = 0;
        for(U32 i = 0; i < (state.vertexProgramSize / cgoShaderInstr::CG1_ISA_INSTR_SIZE); i++)
        {
            cgoShaderInstr shInstr(&code[i * cgoShaderInstr::CG1_ISA_INSTR_SIZE]);

            if (!endProgram)
            {
                char dis[1024];
                shInstr.disassemble(dis, 1024);
                CG_INFO(" %04x : ", state.vertexProgramStartPC + i);
                for(U32 b = 0; b < cgoShaderInstr::CG1_ISA_INSTR_SIZE; b++)
                    CG_INFO("%02x ", code[i * cgoShaderInstr::CG1_ISA_INSTR_SIZE + b]);
                CG_INFO(" : %s", dis);
                fflush(stdout);
                endProgram = shInstr.getEndFlag();
            }
            else
            {
                if (shInstr.getOpcode() != CG1_ISA_OPCODE_NOP)
                    CG_ASSERT("Unexpected instruction after end of program.");

                nopsAtTheEnd++;
            }
        }
        CG_INFO("Trail NOPs = %d", nopsAtTheEnd);
    )

    bmShader->loadShaderProgram(code, state.vertexProgramStartPC, state.vertexProgramSize, VERTEX_PARTITION);
}

//  Load the current fragment program in the shader behaviorModel.
void bmoGpuTop::loadFragmentProgram()
{
    U08 *code;

    U08 *memory = selectMemorySpace(state.fragProgramAddr);
    U32 address = state.fragProgramAddr & SPACE_ADDRESS_MASK;

    code = &memory[address];

    GPU_DEBUG(
        CG_INFO("Loading fragment program from address %08x to PC %04x size %d", state.fragProgramAddr,
            state.fragProgramStartPC, state.fragProgramSize);
        bool endProgram = false;
        U32 nopsAtTheEnd = 0;
        for(U32 i = 0; i < (state.fragProgramSize / cgoShaderInstr::CG1_ISA_INSTR_SIZE); i++)
        {
            cgoShaderInstr shInstr(&code[i * cgoShaderInstr::CG1_ISA_INSTR_SIZE]);

            if (!endProgram)
            {
                char dis[1024];
                shInstr.disassemble(dis, 1024);
                CG_INFO(" %04x : ", state.fragProgramStartPC + i);
                for(U32 b = 0; b < cgoShaderInstr::CG1_ISA_INSTR_SIZE; b++)
                    CG_INFO("%02x ", code[i * cgoShaderInstr::CG1_ISA_INSTR_SIZE + b]);
                CG_INFO(" : %s", dis);
                fflush(stdout);
                endProgram = shInstr.getEndFlag();
            }
            else
            {
                if (shInstr.getOpcode() != CG1_ISA_OPCODE_NOP)
                    CG_ASSERT("Unexpected instruction after end of program.");

                nopsAtTheEnd++;
            }
        }
        CG_INFO("Trail NOPs = %d", nopsAtTheEnd);
    )

    bmShader->loadShaderProgram(code, state.fragProgramStartPC, state.fragProgramSize, FRAGMENT_PARTITION);

}

//  Load the shader program in the shader behaviorModel.
void bmoGpuTop::loadShaderProgram()
{
    U08 *code;

    U08 *memory = selectMemorySpace(state.programAddress);
    U32 address = state.programAddress & SPACE_ADDRESS_MASK;

    code = &memory[address];

    GPU_DEBUG(
        CG_INFO("Loading shader program from address %08x to PC %04x size %d", state.programAddress,
            state.programLoadPC, state.programSize);
        bool endProgram = false;
        U32 nopsAtTheEnd = 0;
        for(U32 i = 0; i < (state.programSize / cgoShaderInstr::CG1_ISA_INSTR_SIZE); i++)
        {
            cgoShaderInstr shInstr(&code[i * cgoShaderInstr::CG1_ISA_INSTR_SIZE]);

            if (!endProgram)
            {
                char dis[1024];
                shInstr.disassemble(dis, 1024);
                CG_INFO(" %04x : ", state.programLoadPC + i);
                for(U32 b = 0; b < cgoShaderInstr::CG1_ISA_INSTR_SIZE; b++)
                    CG_INFO("%02x ", code[i * cgoShaderInstr::CG1_ISA_INSTR_SIZE + b]);
                CG_INFO(" : %s", dis);
                endProgram = shInstr.getEndFlag();
            }
            else
            {
                if (shInstr.getOpcode() != CG1_ISA_OPCODE_NOP)
                    CG_ASSERT("Unexpected instruction after end of program.");

                nopsAtTheEnd++;
            }
        }
        CG_INFO("Trail NOPs = %d", nopsAtTheEnd);
    )

    bmShader->loadShaderProgram(code, state.programLoadPC, state.programSize, FRAGMENT_PARTITION);

}

void bmoGpuTop::clearZStencilBuffer()
{
    GLOBAL_PROFILER_ENTER_REGION("clearZStencilBuffer", "", "clearZStencilBuffer")

    if (!skipBatchMode)
    {
        U08 zStencilClear[4 * 4];

        for(U32 p = 0; p < 4; p++)
            *((U32 *) &zStencilClear[p * 4]) = (state.zBufferClear & 0x00FFFFFF) | ((state.stencilBufferClear && 0xFF) << 24);

        U32 samples = state.multiSampling ? state.msaaSamples : 1;
        U32 bytesPixel;

        bytesPixel = 4;

        zPixelMapper.setupDisplay(state.displayResX, state.displayResY, STAMP_WIDTH, STAMP_WIDTH,
                                  ArchConf.ras.genWidth / STAMP_WIDTH, ArchConf.ras.genHeight / STAMP_HEIGHT,
                                  ArchConf.ras.scanWidth / ArchConf.ras.genWidth, ArchConf.ras.scanHeight / ArchConf.ras.genHeight,
                                  ArchConf.ras.overScanWidth, ArchConf.ras.overScanHeight,
                                  samples, bytesPixel);

        U08 *memory = selectMemorySpace(state.zStencilBufferBaseAddr);

        for(U32 y = 0; y < state.displayResY; y += 2)
        {
            for(U32 x = 0; x < state.displayResX; x += 2)
            {
                U32 address = zPixelMapper.computeAddress(x, y);

                address += state.zStencilBufferBaseAddr;

                //  Get address inside the memory space.
                address = address & SPACE_ADDRESS_MASK;

                //  Check if multisampling is enabled.
                if (!state.multiSampling)
                {
                    //  Write z/stencil clear value.
                    *((U64 *) &memory[address + 0]) = *((U64 *) &zStencilClear[0]);
                    *((U64 *) &memory[address + 8]) = *((U64 *) &zStencilClear[8]);
                }
                else
                {
                    for(U32 s = 0; s < (STAMP_FRAGMENTS * state.msaaSamples); s++)
                    {
                        //  Write z/stencil clear value..
                        *((U32 *) &memory[address + s * 4]) = *((U32 *) &zStencilClear[0]);
                    }
                }
            }
        }
    
    }
    
    GLOBAL_PROFILER_EXIT_REGION()
}

void bmoGpuTop::resetState()
{
    state.statusRegister = GPU_ST_RESET;
    state.displayResX = 400;
    state.displayResY = 400;
    state.frontBufferBaseAddr = 0x00200000;
    state.backBufferBaseAddr =  0x00400000;
    state.zStencilBufferBaseAddr = 0x00000000;
    state.textureMemoryBaseAddr = 0x00000000;
    state.programMemoryBaseAddr = 0x00000000;

    state.vertexProgramAddr = 0;
    state.vertexProgramStartPC = 0;
    state.vertexProgramSize = 0;
    state.vertexThreadResources = 0;

    for(U32 c = 0; c < MAX_VERTEX_CONSTANTS; c++)
    {
        state.vConstants[c][0] = 0.0f;
        state.vConstants[c][1] = 0.0f;
        state.vConstants[c][2] = 0.0f;
        state.vConstants[c][3] = 0.0f;
    }

    for(U32 a = 0; a < MAX_VERTEX_ATTRIBUTES; a++)
    {
        state.outputAttribute[a] = false;
        state.attributeMap[a] = ST_INACTIVE_ATTRIBUTE;
    }

    for(U32 s = 0; s < MAX_STREAM_BUFFERS; s++)
    {
        state.attrDefValue[s][0] = 0.0f;
        state.attrDefValue[s][1] = 0.0f;
        state.attrDefValue[s][2] = 0.0f;
        state.attrDefValue[s][3] = 1.0f;
        state.streamAddress[s] = 0x00000000;
        state.streamStride[s] = 0;
        state.streamElements[s] = 0;
        state.streamData[s] = SD_U32BIT;
        state.streamElements[s] = 0;
        state.streamFrequency[s] = 0;
        state.d3d9ColorStream[s] = false;
    }

    state.streamStart = 0;
    state.streamCount = 0;
    state.streamInstances = 1;
    state.indexedMode = true;
    state.indexStream = 0;
    state.attributeLoadBypass = false;

    state.primitiveMode = cg1gpu::TRIANGLE;
    state.frustumClipping = false;

    for(U32 c = 0; c < 6; c++)
    {
        state.userClip[c][0] = 0.0f;
        state.userClip[c][1] = 0.0f;
        state.userClip[c][2] = 0.0f;
        state.userClip[c][3] = 0.0f;
    }

    state.userClipPlanes = false;
    state.faceMode = cg1gpu::GPU_CCW;
    state.cullMode = cg1gpu::NONE;
    state.hzTest = false;
    state.earlyZ = true;

    state.d3d9PixelCoordinates = false;
    state.viewportIniX = 0;
    state.viewportIniY = 0;
    state.viewportHeight = 400;
    state.viewportWidth = 400;
    state.scissorTest = false;
    state.scissorIniX = 0;
    state.scissorIniY = 0;
    state.scissorHeight = 0;
    state.scissorWidth = 0;
    state.nearRange = 0.0f;
    state.farRange = 1.0f;
    state.d3d9DepthRange = false;
    state.slopeFactor = 0.0f;
    state.unitOffset = 0.0f;
    state.zBufferBitPrecission = 24;
    state.d3d9RasterizationRules = false;
    state.twoSidedLighting = false;
    state.multiSampling = false;
    state.msaaSamples = 1;

    for(U32 a = 0; a < MAX_FRAGMENT_ATTRIBUTES; a++)
    {
        state.interpolation[a] = true;
        state.fragmentInputAttributes[a] = false;
    }

    state.fragProgramAddr = 0;
    state.fragProgramStartPC = 0;
    state.fragProgramSize = 0;
    state.fragThreadResources = 0;

    for(U32 c = 0; c < MAX_FRAGMENT_CONSTANTS; c++)
    {
        state.fConstants[c][0] = 0.0f;
        state.fConstants[c][1] = 0.0f;
        state.fConstants[c][2] = 0.0f;
        state.fConstants[c][3] = 0.0f;
    }

    state.modifyDepth = false;

    for (U32 t = 0; t < MAX_TEXTURES; t++)
    {
        state.textureEnabled[t] = false;
        state.textureMode[t] = GPU_TEXTURE2D;
        state.textureWidth[t] = 0;
        state.textureHeight[t] = 0;
        state.textureDepth[t] = 0;
        state.textureWidth2[t] = 0;
        state.textureHeight2[t] = 0;
        state.textureDepth2[t] = 0;
        state.textureBorder[t] = 0;
        state.textureFormat[t] = GPU_RGBA8888;
        state.textureReverse[t] = false;
        state.textD3D9ColorConv[t] = false;
        state.textD3D9VInvert[t] = false;
        state.textureCompr[t] = GPU_NO_TEXTURE_COMPRESSION;
        state.textureBlocking[t] = GPU_TXBLOCK_TEXTURE;
        state.textBorderColor[t][0] = 0.0f;
        state.textBorderColor[t][1] = 0.0f;
        state.textBorderColor[t][2] = 0.0f;
        state.textBorderColor[t][3] = 1.0f;
        state.textureWrapS[t] = GPU_TEXT_CLAMP;
        state.textureWrapT[t] = GPU_TEXT_CLAMP;
        state.textureWrapR[t] = GPU_TEXT_CLAMP;
        state.textureNonNormalized[t] = false;
        state.textureMinFilter[t] = GPU_NEAREST;
        state.textureMagFilter[t] = GPU_NEAREST;
        state.textureEnableComparison[t] = false;
        state.textureComparisonFunction[t] = GPU_LEQUAL;
        state.textureSRGB[t] = false;
        state.textureMinLOD[t] = 0.0f;
        state.textureMaxLOD[t] = 12.0f;
        state.textureLODBias[t] = 0.0f;
        state.textureMinLevel[t] = 0;
        state.textureMaxLevel[t] = 12;
        state.textureUnitLODBias[t] = 0.0f;
        state.maxAnisotropy[t] = 1;
    }

    state.zBufferClear = 0x00FFFFFF;
    state.stencilBufferClear = 0x00;
    state.zstencilStateBufferAddr = 0x00000000;
    state.stencilTest = false;
    state.stencilFunction = GPU_ALWAYS;
    state.stencilReference = 0x00;
    state.stencilTestMask = 0xFF;
    state.stencilUpdateMask = 0xFF;
    state.stencilFail = STENCIL_KEEP;
    state.depthFail = STENCIL_KEEP;
    state.depthPass = STENCIL_KEEP;
    state.depthTest = false;
    state.depthFunction = GPU_LESS;
    state.depthMask = true;
    state.zStencilCompression = true;

    state.colorBufferFormat = GPU_RGBA8888;
    state.colorCompression = true;
    state.colorSRGBWrite = false;

    for(U32 r = 0; r < MAX_RENDER_TARGETS; r++)
    {
        state.rtEnable[r] = false;
        state.rtFormat[r] = GPU_R32F;
        state.rtAddress[r] = 0x600000 + 0x100000 * r;
        state.colorBlend[r] = false;
        state.blendEquation[r] = BLEND_FUNC_ADD;
        state.blendSourceRGB[r] = BLEND_ONE;
        state.blendDestinationRGB[r] = BLEND_ZERO;
        state.blendSourceAlpha[r] = BLEND_ONE;
        state.blendDestinationAlpha[r] = BLEND_ZERO;
        state.blendColor[r][0] = 0.0f;
        state.blendColor[r][1] = 0.0f;
        state.blendColor[r][2] = 0.0f;
        state.blendColor[r][3] = 1.0f;
        state.colorMaskR[r] = true;
        state.colorMaskG[r] = true;
        state.colorMaskB[r] = true;
        state.colorMaskA[r] = true;
    }

    //  The backbuffer is aliased to render target 0.
    state.rtEnable[0] = true;
    state.rtFormat[0] = state.colorBufferFormat;
    state.rtAddress[0] = state.backBufferBaseAddr;
    
    state.colorBufferClear[0] = 0.0f;
    state.colorBufferClear[1] = 0.0f;
    state.colorBufferClear[2] = 0.0f;
    state.colorBufferClear[3] = 1.0f;
    state.colorStateBufferAddr = 0x00000000;

    state.logicalOperation = false;
    state.logicOpFunction = LOGICOP_COPY;

    state.mcSecondInterleavingStartAddr = 0x00000000;
    state.blitIniX = 0;
    state.blitIniY = 0;
    state.blitXOffset = 0;
    state.blitYOffset = 0;
    state.blitWidth = 400;
    state.blitHeight = 400;
    state.blitTextureWidth2 = 9;
    state.blitDestinationAddress = 0x08000000;
    state.blitDestinationTextureFormat = GPU_RGBA8888;
    state.blitDestinationTextureBlocking = GPU_TXBLOCK_TEXTURE;

    bmTexture->reset();
}

void bmoGpuTop::draw()
{
    if (!skipBatchMode)
    {
        setupDraw();
        for(U32 instance = 0; instance < state.streamInstances; instance++)
        {
            GLOBAL_PROFILER_ENTER_REGION("emulateStreamer", "", "emulateStreamer")
            emulateStreamer(instance);
            GLOBAL_PROFILER_EXIT_REGION()
            GLOBAL_PROFILER_ENTER_REGION("emulatePrimitiveAssembly", "", "emulatePrimitiveAssembly")
            emulatePrimitiveAssembly();
            GLOBAL_PROFILER_EXIT_REGION()
            cleanup();
        }
        printf("B");
        fflush(stdout);
    }
}

void bmoGpuTop::emulateStreamer(U32 instance)
{
    //  Obtain the size of the index data type in bytes (STRIDE IS NOT USED FOR INDEX STREAMS!!!).
    U32 indexStreamDataSize;
    switch(state.streamData[state.indexStream])
    {
        case SD_UNORM8:
        case SD_SNORM8:
        case SD_UINT8:
        case SD_SINT8:
            indexStreamDataSize = 1;
            break;
        case SD_UNORM16:
        case SD_SNORM16:
        case SD_UINT16:
        case SD_SINT16:
        case SD_FLOAT16:
            indexStreamDataSize = 2;
            break;
        case SD_UNORM32:
        case SD_SNORM32:
        case SD_UINT32:
        case SD_SINT32:
        case SD_FLOAT32:
            indexStreamDataSize = 4;
            break;
    }

/*U32 minIndex = 0xFFFFFFFF;
U32 maxIndex = 0;
vector<U32> readIndices;
vector<U32> shadedIndices;*/

    U32 currentIndex = state.streamStart;

    for(U32 v = 0; v < state.streamCount; v++)
    {
        if (state.indexedMode)
        {
            //  Read the next index from the index buffer.
            U08 *memory = selectMemorySpace(state.streamAddress[state.indexStream]);

            U32 address = state.streamAddress[state.indexStream] + (state.streamStart + v) * indexStreamDataSize;

            //  Get address inside the memory space.
            address = address & SPACE_ADDRESS_MASK;

            switch(state.streamData[state.indexStream])
            {
                case SD_UINT8:
                case SD_UNORM8:
                    currentIndex = U32(memory[address]);
                    break;
                case SD_UINT16:
                case SD_UNORM16:
                    currentIndex = U32(*((U16 *) &memory[address]));
                    break;
                case SD_UINT32:
                case SD_UNORM32:
                    currentIndex = *((U32 *) &memory[address]);
                    break;
                case SD_SNORM8:
                case SD_SNORM16:
                case SD_SNORM32:
                case SD_FLOAT16:
                case SD_FLOAT32:
                case SD_SINT8:
                case SD_SINT16:
                case SD_SINT32:
                    CG_ASSERT("Format type not supported for index data..");
                    break;
                default:
                    CG_ASSERT("Undefined index data format type.");
                    break;
            }

            GPU_DEBUG(
                CG_INFO("Read index %d instance %d at address %08x from stream %d address %x stride %d format ", currentIndex, instance,
                    address, state.indexStream, state.streamAddress[state.indexStream], state.streamStride[state.indexStream]);
                switch(state.streamData[state.indexStream])
                {
                    case SD_UINT8:
                    case SD_UNORM8:    //  For compatibility with old library.
                        CG_INFO("SD_UINT8.");
                        break;
                    case SD_UINT16:
                    case SD_UNORM16:    //  For compatibility with old library.
                        CG_INFO("SD_UINT16.");
                        break;
                    case SD_UINT32:
                    case SD_UNORM32:    //  For compatibility with old library.
                        CG_INFO("SD_UINT32.");
                        break;
                    case SD_SNORM8:
                    case SD_SNORM16:
                    case SD_SNORM32:
                    case SD_FLOAT16:
                    case SD_FLOAT32:
                    case SD_SINT8:
                    case SD_SINT16:
                    case SD_SINT32:
                        CG_ASSERT("Format type not supported for index data..");
                        break;
                    default:
                        CG_ASSERT("Undefined index data format type.");
                        break;
                }
            ) // GPU_DEBUG(
        }
        else
        {
            GPU_DEBUG(
                CG_INFO("Generated index %d.", currentIndex);
            )
        }

        //  Add index to the index list.
        indexList.push_back(currentIndex);

        map<U32, ShadedVertex*>::iterator it;

/*if (state.indexedMode)
{
    readIndices.push_back(currentIndex);
}*/

        //  Search index in the vertex list.
        it = vertexList.find(currentIndex);

        //  Check if the vertex for the current index has been already shaded.
        if (it == vertexList.end())
        {
/*if (state.indexedMode)
{
    minIndex = GPU_MIN(minIndex, currentIndex);
    maxIndex = GPU_MAX(maxIndex, currentIndex);
    shadedIndices.push_back(currentIndex);
}*/

            //  Read input attributes for the vertex.
            Vec4FP32 attributes[MAX_VERTEX_ATTRIBUTES];

            for(U32 a = 0; a < MAX_VERTEX_ATTRIBUTES; a++)
            {
                //  Check if the vertex input attribute is active.
                if (state.attributeMap[a] != ST_INACTIVE_ATTRIBUTE)
                {
                    //  Read attribute from the corresponding stream.
                    U32 stream = state.attributeMap[a];
                    U32 address = state.streamAddress[stream];
                    U08 *memory = selectMemorySpace(address);

                    //  Check if the attribute is per-instance or per-index.
                    if (state.streamFrequency[stream] == 0)
                    {
                        //  Per-index attribute.
                        address = address + state.streamStride[stream] * currentIndex;
                    }
                    else
                    {
                        //  Per-instance attribute.
                        address = address + state.streamStride[stream] * (instance / state.streamFrequency[stream]);
                    }

                    //  Get address inside the memory space.
                    address = address & SPACE_ADDRESS_MASK;

                    U32 e;
                    for(e = 0; e < state.streamElements[stream]; e++)
                    {
                        switch(state.streamData[stream])
                        {
                            case SD_UNORM8:
                                attributes[a][e] = F32(memory[address + 1 * e]) * (1.0f / 255.0f);
                                break;
                            case SD_SNORM8:
                                attributes[a][e] = F32(*((S08 *) &memory[address + 1 * e])) * (1.0f / 127.0f);
                                break;
                            case SD_UNORM16:
                                attributes[a][e] = F32(*((U16 *) &memory[address + 2 * e])) * (1.0f / 65535.0f);
                                break;
                            case SD_SNORM16:
                                attributes[a][e] = F32(*((S16 *) &memory[address + 2 * e])) * (1.0f / 32767.0f);
                                break;
                            case SD_UNORM32:
                                attributes[a][e] = F32(*((U32 *) &memory[address + 4 * e])) * (1.0f / 4294967295.0f);
                                break;
                            case SD_SNORM32:
                                attributes[a][e] = F32(*((S32 *) &memory[address + 4 * e])) * (1.0f / 2147483647.0f);
                                break;
                            case SD_FLOAT16:
                                attributes[a][e] = GPUMath::convertFP16ToFP32(*((f16bit *) &memory[address + 2 * e]));
                                break;
                            case SD_FLOAT32:
                                attributes[a][e] = *((F32 *) &memory[address + 4 * e]);
                                break;
                            case SD_UINT8:
                                attributes[a][e] = F32(memory[address + 1 * e]);
                                break;
                            case SD_SINT8:
                                attributes[a][e] = F32(*((S08 *) &memory[address + 1 * e]));
                                break;
                            case SD_UINT16:
                                attributes[a][e] = F32(*((U16 *) &memory[address + 2 * e]));
                                break;
                            case SD_SINT16:
                                attributes[a][e] = F32(*((S16 *) &memory[address + 2 * e]));
                                break;
                            case SD_UINT32:
                                attributes[a][e] = F32(*((U32 *) &memory[address + 4 * e]));
                                break;
                            case SD_SINT32:
                                attributes[a][e] = F32(*((S32 *) &memory[address + 4 * e]));
                                break;
                            default:
                                CG_ASSERT("Undefined stream data format.");
                                break;
                        }

                        GPU_DEBUG(
                            CG_INFO("Read attribute %d element %d with value %f from stream %d address %x stride %d elements %d format ",
                                a, e, attributes[a][e], stream, state.streamAddress[stream], state.streamStride[stream],
                                state.streamElements[stream]);
                            switch(state.streamData[stream])
                            {
                                case SD_UNORM8:
                                    CG_INFO("SD_UNORM8.");
                                    break;
                                case SD_SNORM8:
                                    CG_INFO("SD_SNORM8.");
                                    break;
                                case SD_UNORM16:
                                    CG_INFO("SD_UNORM16.");
                                    break;
                                case SD_SNORM16:
                                    CG_INFO("SD_SNORM16.");
                                    break;
                                case SD_UNORM32:
                                    CG_INFO("SD_UNORM32.");
                                    break;
                                case SD_SNORM32:
                                    CG_INFO("SD_SNORM32.");
                                    break;
                                case SD_FLOAT16:
                                    CG_INFO("SD_FLOAT16.");
                                    break;
                                case SD_FLOAT32:
                                    CG_INFO("SD_FLOAT32.");
                                    break;
                                case SD_UINT8:
                                    CG_INFO("SD_UINT8.");
                                    break;
                                case SD_SINT8:
                                    CG_INFO("SD_SINT8.");
                                    break;
                                case SD_UINT16:
                                    CG_INFO("SD_UINT16.");
                                    break;
                                case SD_SINT16:
                                    CG_INFO("SD_SINT16.");
                                    break;
                                case SD_UINT32:
                                    CG_INFO("SD_UINT32.");
                                    break;
                                case SD_SINT32:
                                    CG_INFO("SD_SINT32.");
                                    break;
                                default:
                                    CG_ASSERT("Undefined format.");
                                    break;
                            }
                        )
                    }

                    //  Set default value for undefined attribute components.
                    for(; e < 4; e++)
                    {
                        attributes[a][e] = state.attrDefValue[a][e];

                        GPU_DEBUG(
                            CG_INFO("Setting default value for attribute %d element %d value %f",
                                a, e, attributes[a][e]);
                        )
                    }

                    //  Check if the D3D9 color order for the color components must be used.
                    if (state.d3d9ColorStream[stream])
                    {
                        //
                        //  The D3D9 color formats are stored in little endian order with the alpha in higher addresses:
                        //
                        //  For example:
                        //
                        //     D3DFMT_A8R8G8B8 is stored as B G R A
                        //     D3DFMT_X8R8G8B8 is stored as B G R X
                        //

                        F32 red = attributes[a][2];
                        F32 green = attributes[a][1];
                        F32 blue = attributes[a][0];
                        F32 alpha = attributes[a][3];

                        attributes[a][0] = red;
                        attributes[a][1] = green;
                        attributes[a][2] = blue;
                        attributes[a][3] = alpha;
                    }
                }
                else
                {
                    //  Set default value for inactive vertex input attribute.
                    attributes[a][0] = state.attrDefValue[a][0];
                    attributes[a][1] = state.attrDefValue[a][1];
                    attributes[a][2] = state.attrDefValue[a][2];
                    attributes[a][3] = state.attrDefValue[a][3];

                    GPU_DEBUG(
                        CG_INFO("Setting default value for attribute %d value {%f, %f, %f, %f}",
                            a, attributes[a][0], attributes[a][1], attributes[a][2], attributes[a][3]);
                    )
                }
            }

            //  Shade the vertex.
            ShadedVertex *vertex;
            vertex = new ShadedVertex(attributes);

            CG1_BMDL_TRACE(
                if (traceLog || (traceBatch && (watchBatch == batchCounter)) || (traceVertex && (currentIndex == watchIndex)))
                {
                    CG_INFO("Vertex Shader input for batch %d index %d : ", batchCounter, currentIndex);
                    for(U32 a = 0; a < MAX_VERTEX_ATTRIBUTES; a++)
                    {
                        if (state.attributeMap[a] != ST_INACTIVE_ATTRIBUTE)
                            CG_INFO("i[%d] = {%f, %f, %f, %f}", a, attributes[a][0], attributes[a][1], attributes[a][2], attributes[a][3]);
                    }
                }
            )

            //  Check if validation mode is enabled.
            if (validationMode)
            {
                //  Copy the vertex information.
                VertexInputInfo vInfo;
                vInfo.vertexID.index = currentIndex;
                vInfo.vertexID.instance = instance;
                vInfo.differencesBetweenReads = false;
                vInfo.timesRead = 1;
                for(U32 a = 0; a < MAX_VERTEX_ATTRIBUTES; a++)
                    vInfo.attributes[a] = attributes[a];
                
                //  Log the shaded vertex.    
                vertexInputLog.insert(make_pair(vInfo.vertexID, vInfo));
            }
            
            CG1_BMDL_TRACE(
                traceVShader = traceLog || 
                ((traceBatch && (batchCounter == watchBatch)) && (watchIndex == currentIndex)) ||
                (traceVertex && (watchIndex == currentIndex));
            )
            
            emulateVertexShader(vertex);
            
            CG1_BMDL_TRACE(
                traceVShader = false;
            )

            //  Clamp the color attribute to the [0, 1] range.
            Vec4FP32 *vattributes = vertex->getAttributes();
            vattributes[COLOR_ATTRIBUTE][0] = GPU_CLAMP(vattributes[COLOR_ATTRIBUTE][0], 0.0f, 1.0f);
            vattributes[COLOR_ATTRIBUTE][1] = GPU_CLAMP(vattributes[COLOR_ATTRIBUTE][1], 0.0f, 1.0f);
            vattributes[COLOR_ATTRIBUTE][2] = GPU_CLAMP(vattributes[COLOR_ATTRIBUTE][2], 0.0f, 1.0f);
            vattributes[COLOR_ATTRIBUTE][3] = GPU_CLAMP(vattributes[COLOR_ATTRIBUTE][3], 0.0f, 1.0f);

            CG1_BMDL_TRACE(
                if (traceLog || (traceBatch && (watchBatch == batchCounter)) || (traceVertex && (watchIndex == currentIndex)))
                {
                    CG_INFO("Vertex Shader output for batch %d index %d : ", batchCounter, currentIndex);
                    for(U32 a = 0; a < MAX_VERTEX_ATTRIBUTES; a++)
                    {
                        if (state.outputAttribute[a])
                            CG_INFO("o[%d] = {%f, %f, %f, %f}", a, vattributes[a][0], vattributes[a][1], vattributes[a][2], vattributes[a][3]);
                    }
                }
            )

            //  Add the shaded vertex to the table of shaded vertices.
            vertexList.insert(make_pair(currentIndex, vertex));
            
            //  Check if validation mode is enabled.
            if (validationMode)
            {
                //  Copy the vertex information.
                ShadedVertexInfo vInfo;
                vInfo.vertexID.index = currentIndex;
                vInfo.vertexID.instance = instance;
                vInfo.differencesBetweenShading = false;
                vInfo.timesShaded = 1;
                for(U32 a = 0; a < MAX_VERTEX_ATTRIBUTES; a++)
                    vInfo.attributes[a] = vattributes[a];
                
                //  Log the shaded vertex.    
                shadedVertexLog.insert(make_pair(vInfo.vertexID, vInfo));
            }
        }

        if (!state.indexedMode)
            currentIndex++;
    }

/*if (state.indexedMode)
{
    fprintf(fIndexList, "INDEXED DRAW Count = %d Range = [%d, %d] Range Count = %d Unique Indices = %d", state.streamCount,
        minIndex, maxIndex, maxIndex - minIndex + 1, shadedIndices.size());
    if (F32(maxIndex - minIndex + 1) < (shadedIndices.size() * 1.05))
        fprintf(fIndexList, "");
    else if (F32(maxIndex - minIndex + 1) < (shadedIndices.size() * 1.20))
        fprintf(fIndexList, " SPARSE");
    else
        fprintf(fIndexList, " VERY SPARSE");
    fprintf(fIndexList, " READ INDICES : ");
    for(U32 i = 0; i < readIndices.size(); i++)
       fprintf(fIndexList, "    %8d", readIndices[i]);
    fprintf(fIndexList, " SHADED INDICES : ");
    for(U32 i = 0; i < shadedIndices.size(); i++)
        fprintf(fIndexList, "    %8d", shadedIndices[i]);
}
else
    fprintf(fIndexList, "DRAW Count = %d Index Range = [%d, %d]",
        state.streamCount, state.streamStart, state.streamStart + state.streamCount - 1);
*/
}

U08 *bmoGpuTop::selectMemorySpace(U32 address)
{
    U08 *memory;

    //  Check address space.
    if ((address & ADDRESS_SPACE_MASK) == GPU_ADDRESS_SPACE)
        memory = gpuMemory;
    else if ((address & ADDRESS_SPACE_MASK) == SYSTEM_ADDRESS_SPACE)
        memory = sysMemory;
    else
        CG_ASSERT("Undefined address space");

    return memory;
}

void bmoGpuTop::emulateVertexShader(ShadedVertex *vertex)
{
    GLOBAL_PROFILER_ENTER_REGION("emulateVertexShader", "", "emulateVertexShader")
    //  Initialize thread for the current vertex.
    bmShader->resetShaderState(0);
    //  Load the new shader input into the shader input register bank of the thread element in the shader behaviorModel.
    bmShader->loadShaderState(0, cg1gpu::IN, vertex->getAttributes());
    //  Set PC for the thread element in the shader behaviorModel to the start PC of the vector thread.
    bmShader->setThreadPC(0, state.vertexProgramStartPC);
    bool programEnd = false;
    U32 pc = state.vertexProgramStartPC;

    //  Execute all the instructions in the program.
    while (!programEnd)
    {
        cgoShaderInstr::cgoShaderInstrEncoding *shDecInstr;

        //  Fetch the instruction.
        shDecInstr = bmShader->FetchInstr(0, pc);

        GPU_ASSERT(
            char buffer[256];
            sprintf(buffer, "Error fetching vertex program instruction at %02x", pc);
            if (shDecInstr == NULL)
                CG_ASSERT(buffer);
        )

        GPU_DEBUG(
            char shInstrDisasm[256];
            shDecInstr->getShaderInstruction()->disassemble(shInstrDisasm, 256);
            CG_INFO("VSh => Executing instruction @ %04x : %s", pc, shInstrDisasm);
        )

        CG1_BMDL_TRACE(
            if (traceVShader)
            {
                char shInstrDisasm[256];
                shDecInstr->getShaderInstruction()->disassemble(shInstrDisasm, 256);
                CG_INFO("     %04x : %s", pc, shInstrDisasm);
                
                printShaderInstructionOperands(shDecInstr);
            }
        )
        bmShader->execShaderInstruction(shDecInstr);        //  Execute instruction.
        CG1_BMDL_TRACE(
            if (traceVShader)
            {                        
                printShaderInstructionResult(shDecInstr);
                CG_INFO("-------------------");
                fflush(stdout);
            }
        )
       
        //  Process texture requests.  Texture requests are processed per fragment quad.
        TextureAccess *texAccess = bmShader->nextVertexTextureAccess();

        //  Check if there is a pending texture request from the previous shader instruction.
        if (texAccess != NULL)
        {
            //  Process the texture requests.
            emulateTextureUnit(texAccess);
        }

        bool jump = false;
        U32 destPC = 0;
        
        //  Check for jump instructions.
        if (shDecInstr->getShaderInstruction()->isAJump())
        {
            //  Check if the jump is performed.
            jump = bmShader->checkJump(shDecInstr, 1, destPC);
        }
        
        //  Check if this is the last instruction in the program.
        programEnd = shDecInstr->getShaderInstruction()->getEndFlag();

        //  Update PC.
        if (jump)
            pc = destPC;
        else
            pc++;
    }

    //  Get output attributes for the vertex.
    bmShader->readShaderState(0, cg1gpu::OUT, vertex->getAttributes());

    GPU_DEBUG(
        CG_INFO("VSh => Vertex Outputs :");
        for(U32 a = 0; a < MAX_VERTEX_ATTRIBUTES; a++)
        {
            Vec4FP32 *attributes = vertex->getAttributes();
            CG_INFO(" OUT[%02d] = {%f, %f, %f, %f}", a, attributes[a][0], attributes[a][1], attributes[a][2], attributes[a][3]);
        }
    )

    GLOBAL_PROFILER_EXIT_REGION()
}

void bmoGpuTop::emulatePrimitiveAssembly()
{
    ShadedVertex *vertex1, *vertex2, *vertex3;
    U32 requiredVertices;
    U32 remainingVertices = indexList.size();
    U32 currentIndex;
    switch(state.primitiveMode)
    {
        case TRIANGLE:
            currentIndex = 0;
            requiredVertices = 3;
            break;
        case TRIANGLE_STRIP:
        case TRIANGLE_FAN:
            currentIndex = 2;
            remainingVertices -= 2;
            requiredVertices = 1;
            break;
        case QUAD:
            currentIndex = 0;
            requiredVertices = 4;
            break;
        case QUAD_STRIP:
            currentIndex = 3;
            remainingVertices -= 2;
            requiredVertices = 2;
            break;
        case LINE:
            CG_ASSERT("Line primitive not supported yet.");
            break;
        case LINE_FAN:
            CG_ASSERT("Line Fan primitive not supported yet.");
            break;
        case LINE_STRIP:
            CG_ASSERT("Line Strip primitive not supported yet.");
            break;
        case POINT:
            CG_ASSERT("Point primitive not supported yet.");
            break;
        default:
            CG_ASSERT("Primitive mode not supported.");
            break;
    }

    bool oddTriangle = true;

    while (remainingVertices >= requiredVertices)
    {
        switch(state.primitiveMode)
        {
            case TRIANGLE:
                //  Get three vertices.
                vertex1 = getVertex(indexList[currentIndex]);
                vertex2 = getVertex(indexList[currentIndex + 1]);
                vertex3 = getVertex(indexList[currentIndex + 2]);
                CG_INFO("Assembled TRIANGLE with indices %d %d %d",
                        indexList[currentIndex], indexList[currentIndex + 1], indexList[currentIndex + 2]);
                //  Check if all the vertices were found.
                CG_ASSERT_COND(((vertex1 != NULL) && (vertex2 != NULL) && (vertex3 != NULL)), "Error assembling vertices.");
                emulateRasterization(vertex1, vertex2, vertex3);
                currentIndex += 3;
                remainingVertices -= 3;
                break;

            case TRIANGLE_STRIP:

                //  Check odd/even triangle for propper vertex ordering.
                if (oddTriangle)
                {
                    //  Get three vertices.
                    vertex1 = getVertex(indexList[currentIndex - 2]);
                    vertex2 = getVertex(indexList[currentIndex - 1]);
                    vertex3 = getVertex(indexList[currentIndex]);

                    oddTriangle = false;

                    GPU_DEBUG(
                        CG_INFO("Assembled TRIANGLE(STRIP-odd) with indices %d %d %d",
                            indexList[currentIndex - 2], indexList[currentIndex - 1], indexList[currentIndex]);
                    )
                }
                else
                {
                    //  Get three vertices.
                    vertex1 = getVertex(indexList[currentIndex - 1]);
                    vertex2 = getVertex(indexList[currentIndex - 2]);
                    vertex3 = getVertex(indexList[currentIndex]);

                    oddTriangle = true;

                    GPU_DEBUG(
                        CG_INFO("Assembled TRIANGLE(STRIP-even) with indices %d %d %d",
                            indexList[currentIndex - 1], indexList[currentIndex - 2], indexList[currentIndex]);
                    )
                }

                emulateRasterization(vertex1, vertex2, vertex3);

                //  Check if all the vertices were found.
                CG_ASSERT_COND(!((vertex1 == NULL) || (vertex2 == NULL) || (vertex3 == NULL)), "Error assembling vertices.");
                currentIndex++;
                remainingVertices -= 1;
                break;

            case TRIANGLE_FAN:

                //  Get three vertices.
                vertex1 = getVertex(indexList[0]);
                vertex2 = getVertex(indexList[currentIndex - 1]);
                vertex3 = getVertex(indexList[currentIndex]);

                GPU_DEBUG(
                    CG_INFO("Assembled TRIANGLE(FAN) with indices %d %d %d",
                        indexList[0], indexList[currentIndex - 1], indexList[currentIndex]);
                )

                emulateRasterization(vertex1, vertex2, vertex3);

                //  Check if all the vertices were found.
                CG_ASSERT_COND(!((vertex1 == NULL) || (vertex2 == NULL) || (vertex3 == NULL)), "Error assembling vertices.");
                currentIndex++;
                remainingVertices -= 1;
                break;

            case QUAD:

                //  Get three vertices for the first triangle forming the quad.
                vertex1 = getVertex(indexList[currentIndex]);
                vertex2 = getVertex(indexList[currentIndex + 1]);
                vertex3 = getVertex(indexList[currentIndex + 3]);

                GPU_DEBUG(
                    CG_INFO("Assembled QUAD (1st TRIANGLE) with indices %d %d %d",
                        indexList[currentIndex], indexList[currentIndex + 1], indexList[currentIndex + 3]);
                )

                emulateRasterization(vertex1, vertex2, vertex3);

                //  Check if all the vertices were found.
                CG_ASSERT_COND(!((vertex1 == NULL) || (vertex2 == NULL) || (vertex3 == NULL)), "Error assembling vertices.");
                //  Get three vertices for the second triangle forming the quad.
                vertex1 = getVertex(indexList[currentIndex + 1]);
                vertex2 = getVertex(indexList[currentIndex + 2]);
                vertex3 = getVertex(indexList[currentIndex + 3]);

                GPU_DEBUG(
                    CG_INFO("Assembled QUAD (2nd TRIANGLE) with indices %d %d %d",
                        indexList[currentIndex + 1], indexList[currentIndex + 2], indexList[currentIndex + 3]);
                )

                //  Check if all the vertices were found.
                CG_ASSERT_COND(!((vertex1 == NULL) || (vertex2 == NULL) || (vertex3 == NULL)), "Error assembling vertices.");
                emulateRasterization(vertex1, vertex2, vertex3);

                currentIndex += 4;
                remainingVertices -= 4;
                break;

            case QUAD_STRIP:

                //  Get three vertices for the first triangle forming the quad.
                vertex1 = getVertex(indexList[currentIndex - 3]);
                vertex2 = getVertex(indexList[currentIndex - 2]);
                vertex3 = getVertex(indexList[currentIndex]);

                GPU_DEBUG(
                    CG_INFO("Assembled QUAD(STRIP) (1st TRIANGLE) with indices %d %d %d",
                        indexList[currentIndex - 3], indexList[currentIndex - 2], indexList[currentIndex]);
                )

                //  Check if all the vertices were found.
                CG_ASSERT_COND(!((vertex1 == NULL) || (vertex2 == NULL) || (vertex3 == NULL)), "Error assembling vertices.");
                emulateRasterization(vertex1, vertex2, vertex3);

                //  Get three vertices for the second triangle forming the quad.
                vertex1 = getVertex(indexList[currentIndex - 1]);
                vertex2 = getVertex(indexList[currentIndex - 3]);
                vertex3 = getVertex(indexList[currentIndex]);

                GPU_DEBUG(
                    CG_INFO("Assembled QUAD-(STRIP) (2nd TRIANGLE) with indices %d %d %d",
                        indexList[currentIndex - 1], indexList[currentIndex - 3], indexList[currentIndex]);
                )

                //  Check if all the vertices were found.
                CG_ASSERT_COND(!((vertex1 == NULL) || (vertex2 == NULL) || (vertex3 == NULL)), "Error assembling vertices.");
                emulateRasterization(vertex1, vertex2, vertex3);

                currentIndex += 2;
                remainingVertices -= 2;
                break;

            case LINE:
                CG_ASSERT("Line primitive not supported yet.");
                break;

            case LINE_FAN:
                CG_ASSERT("Line Fan primitive not supported yet.");
                break;

            case LINE_STRIP:
                CG_ASSERT("Line Strip primitive not supported yet.");
                break;

            case POINT:
                CG_ASSERT("Point primitive not supported yet.");
                break;

            default:
                CG_ASSERT("Primitive mode not supported.");
                break;
        }
    }
}

bmoGpuTop::ShadedVertex *bmoGpuTop::getVertex(U32 index)
{
    map<U32, bmoGpuTop::ShadedVertex *>::iterator it;
    it = vertexList.find(index);

    if (it == vertexList.end())
        return NULL;
    else
        return it->second;
}


void bmoGpuTop::emulateRasterization(ShadedVertex *vertex1, ShadedVertex *vertex2, ShadedVertex *vertex3)
{
    GLOBAL_PROFILER_ENTER_REGION("emulateRasterization", "", "emulateRasterization")

    Vec4FP32 *attribV1 = vertex1->getAttributes();
    Vec4FP32 *attribV2 = vertex2->getAttributes();
    Vec4FP32 *attribV3 = vertex3->getAttributes();

    if (bmoClipper::trivialReject(attribV1[0], attribV2[0], attribV3[0], state.d3d9DepthRange))
    {
        //  Cull the triangle that is outside the frustrum.
        CG_INFO("Culled triangle with positions :");
        CG_INFO("    v1 = (%f, %f, %f, %f)", attribV1[0][0], attribV1[0][1], attribV1[0][2], attribV1[0][3]);
        CG_INFO("    v2 = (%f, %f, %f, %f)", attribV2[0][0], attribV2[0][1], attribV2[0][2], attribV2[0][3]);
        CG_INFO("    v3 = (%f, %f, %f, %f)", attribV3[0][0], attribV3[0][1], attribV3[0][2], attribV3[0][3]);
    }
    else
    {
        //  Rasterize the triangle.
        CG_INFO("Rasterize triangle with positions :");
        CG_INFO("    v1 = (%f, %f, %f, %f)", attribV1[0][0], attribV1[0][1], attribV1[0][2], attribV1[0][3]);
        CG_INFO("    v2 = (%f, %f, %f, %f)", attribV2[0][0], attribV2[0][1], attribV2[0][2], attribV2[0][3]);
        CG_INFO("    v3 = (%f, %f, %f, %f)", attribV3[0][0], attribV3[0][1], attribV3[0][2], attribV3[0][3]);
        Vec4FP32 *attributesVertex1 = new Vec4FP32[MAX_VERTEX_ATTRIBUTES];
        Vec4FP32 *attributesVertex2 = new Vec4FP32[MAX_VERTEX_ATTRIBUTES];
        Vec4FP32 *attributesVertex3 = new Vec4FP32[MAX_VERTEX_ATTRIBUTES];

        for(U32 a = 0; a < MAX_VERTEX_ATTRIBUTES; a++)
        {
            attributesVertex1[a] = attribV1[a];
            attributesVertex2[a] = attribV2[a];
            attributesVertex3[a] = attribV3[a];
        }

        //  Setup the triangle.
        U32 triangleID = bmRaster->setup(attributesVertex1, attributesVertex2, attributesVertex3);
        CG_INFO("Triangle setup ID %d", triangleID);
        //  Perform cull face test.
        bool dropTriangle = cullTriangle(triangleID);
        if (!dropTriangle) //  Check if the triangle must be culled.
        {
            //  Generate fragments for the triangle.
            //  Initiate rasterization of the triangle.
            U32 batchID = bmRaster->startRecursiveMulti(&triangleID, 1, state.multiSampling);
            CG_INFO("Starting recursive algorithm");
            bool lastFragment = false;
            CG_INFO("Updating recursive algorithm");
            bmRaster->updateRecursiveMultiv2(batchID); //  Update the triangle rasterization algorithm.
            while(!bmRaster->lastFragment(triangleID)) //  Process all the triangle fragments.
            {
                U32 currentTriangleID;
                //  Get the next fragment quad for the triangle.
                Fragment **stamp = bmRaster->nextStampRecursiveMulti(batchID, currentTriangleID);
                CG_INFO("Requested next fragment quad. Empty = %s", (stamp == NULL) ? "T" : "F");
                //  Check if fragments were obtained.
                if (stamp != NULL)
                {
                    // Remove fragments outside the viewport or scissor windows.

                    //  Check if multisampling is enabled.
                    if (state.multiSampling)
                    {
                        //  Compute samples for all the fragments in the quad.
                        for(U32 p = 0; p < STAMP_FRAGMENTS; p++)
                            bmRaster->computeMSAASamples(stamp[p], state.msaaSamples);
                    }
                    //  Create array of shaded fragments.
                    ShadedFragment *quad[4];
                    bool notAllFragmentsCulled = false;
                    bool culled[4];

                    //  Cull fragments and compute fragments attributes for the quad.
                    for(U32 p = 0; p < STAMP_FRAGMENTS; p++)
                    {
                        //  Check for the last triangle fragment.
                        lastFragment = stamp[p]->isLastFragment();
                        //  Cull fragments outside the triangle.
                        culled[p] = !stamp[p]->isInsideTriangle();
                        //  Cull fragments outside the screen, the viewport or the scissor rectangle.
                        if (!culled[p])
                            culled[p] = (stamp[p]->getX() < 0) ||
                                     (stamp[p]->getX() >= S32(state.displayResX)) ||
                                     (stamp[p]->getY() < 0) ||
                                     (stamp[p]->getY() >= S32(state.displayResY)) ||
                                     (stamp[p]->getX() < state.viewportIniX) ||
                                     (stamp[p]->getX() >= (state.viewportIniX + S32(state.viewportWidth))) ||
                                     (stamp[p]->getY() < state.viewportIniY) ||
                                     (stamp[p]->getY() >= (state.viewportIniY + S32(state.viewportHeight))) ||
                                     (state.scissorTest &&
                                      ((stamp[p]->getX() < state.scissorIniX) ||
                                       (stamp[p]->getX() >= (state.scissorIniX + S32(state.scissorWidth))) ||
                                       (stamp[p]->getY() < state.scissorIniY) ||
                                       (stamp[p]->getY() >= (state.scissorIniY + S32(state.scissorHeight)))
                                      )
                                     );
                        notAllFragmentsCulled = notAllFragmentsCulled || !culled[p];

                        GPU_DEBUG(
                            if (culled[p])
                            {
                                if (!stamp[p]->isInsideTriangle())
                                    CG_INFO("Fragment at (%d, %d) has been culled.  Outside triangle.", stamp[p]->getX(), stamp[p]->getY());
                                else if ((stamp[p]->getX() < 0) ||
                                         (stamp[p]->getX() >= state.displayResX) ||
                                         (stamp[p]->getY() < 0) ||
                                         (stamp[p]->getY() >= state.displayResY))
                                    CG_INFO("Fragment at (%d, %d) has been culled.  Outside display.", stamp[p]->getX(), stamp[p]->getY());
                                else if ((stamp[p]->getX() < state.viewportIniX) ||
                                         (stamp[p]->getX() >= (state.viewportIniX + state.viewportWidth)) ||
                                         (stamp[p]->getY() < state.viewportIniY) ||
                                         (stamp[p]->getY() >= (state.viewportIniY + state.viewportHeight)))
                                    CG_INFO("Fragment at (%d, %d) has been culled.  Outside viewport.", stamp[p]->getX(), stamp[p]->getY());
                                else if (state.scissorTest &&
                                         ((stamp[p]->getX() < state.scissorIniX) ||
                                          (stamp[p]->getX() >= (state.scissorIniX + state.scissorWidth)) ||
                                          (stamp[p]->getY() < state.scissorIniY) ||
                                          (stamp[p]->getY() >= (state.scissorIniY + state.scissorHeight))))
                                    CG_INFO("Fragment at (%d, %d) has been culled.  Outside scissor rectangle.", stamp[p]->getX(), stamp[p]->getY());
                            }
                            else
                                CG_INFO("Fragment at (%d, %d) generated", stamp[p]->getX(), stamp[p]->getY());
                        )
                    }

                    //  Check if all the fragments in the quad are culled.
                    if (notAllFragmentsCulled)
                    {
                        GLOBAL_PROFILER_ENTER_REGION("emulateRasterization(attribute interpolation)", "", "emulateRasterization")
                        //  Interpolate attributes for all the fragments.
                        for(U32 p = 0; p < STAMP_FRAGMENTS; p++)
                        {
                            quad[p] = new ShadedFragment(stamp[p], culled[p]);

                            Vec4FP32 *attributes = quad[p]->getAttributes();

                            //  Interpolate fragment attributes.
                            for(U32 a = 0; a < MAX_FRAGMENT_ATTRIBUTES; a++)
                            {
                                //  Check if the attribute is active.
                                if (state.fragmentInputAttributes[a])
                                {
                                    //  Check if attribute interpolation is enabled.
                                    if (state.interpolation[a])
                                    {
                                        //  Interpolate attribute;
                                        attributes[a] = bmRaster->interpolate(stamp[p], a);
                                        CG_INFO("Interpolating attribute %d to fragment attribute : {%f, %f, %f, %f}",
                                                a, attributes[a][0],  attributes[a][1],  attributes[a][2],  attributes[a][3]);
                                    }
                                    else
                                    {
                                        //  Copy attribute from the triangle last vertex attribute.
                                        attributes[a] = bmRaster->copy(stamp[p], a, 2);
                                        CG_INFO("Copying attribute %d from vertex 2 to fragment attribute : {%f, %f, %f, %f}",
                                                a, attributes[a][0],  attributes[a][1],  attributes[a][2],  attributes[a][3]);
                                    }
                                }
                                else
                                {
                                    CG_INFO("Setting default value to fragment attribute %d", a);
                                    //  Set default attribute value.
                                    attributes[a][0] = 0.0f;
                                    attributes[a][1] = 0.0f;
                                    attributes[a][2] = 0.0f;
                                    attributes[a][3] = 0.0f;
                                }
                            }


                            //  Set position attribute (special case).
                            attributes[POSITION_ATTRIBUTE][0] = F32(stamp[p]->getX());
                            attributes[POSITION_ATTRIBUTE][1] = F32(stamp[p]->getY());
                            attributes[POSITION_ATTRIBUTE][2] = ((F32) stamp[p]->getZ()) / ((F32) ((1 << state.zBufferBitPrecission) - 1));
                            attributes[POSITION_ATTRIBUTE][3] = 0.0f;
                        
                            //  Set face (triangle area) attribute (special case)
                            attributes[FACE_ATTRIBUTE][3] = F32(stamp[p]->getTriangle()->getArea());
                        }

                        GLOBAL_PROFILER_EXIT_REGION()

                        bool watchPixelFound = false;
                        U32 watchPixelPosInQuad = 0;
                        
                        CG1_BMDL_TRACE(
                            if (traceLog || (traceBatch && (batchCounter == watchBatch)) || tracePixel)
                            {
                                U32 watchPixelPosInQuad = 0;
                                while (!watchPixelFound && (watchPixelPosInQuad < STAMP_FRAGMENTS))
                                {
                                    Fragment *fr = quad[watchPixelPosInQuad]->getFragment();
                                    watchPixelFound = ((fr->getX() == watchPixelX) && (fr->getY() == watchPixelY));
                                    if (!watchPixelFound)
                                        watchPixelPosInQuad++;
                                }
                                if (watchPixelFound)
                                {
                                    CG_INFO("emuPrimAssembly => Cull flag for pixel (%d, %d) before zstencil is %s",
                                        watchPixelX, watchPixelY, quad[watchPixelPosInQuad]->isCulled() ? "True" : "False");
                                }
                            }
                        )
                        
                        //  Perform early z.
                        if (state.earlyZ && !state.modifyDepth)
                        {
                            emulateZStencilTest(quad);
                        }

                        CG1_BMDL_TRACE(
                            if (traceLog || (traceBatch && (batchCounter == watchBatch)) || tracePixel)
                            {
                                if (watchPixelFound)
                                {
                                    CG_INFO("emuPrimAssembly => Cull flag for pixel (%d, %d) after zstencil (earlyz) is %s",
                                        watchPixelX, watchPixelY, quad[watchPixelPosInQuad]->isCulled() ? "True" : "False");

                                    traceFShader = true;
                                    traceTexture = true;
                                }
                            }
                        )
                        
                        //  Shade the fragment quad.
                        emulateFragmentShading(quad);

                        CG1_BMDL_TRACE(
                            if (traceLog || (traceBatch && (batchCounter == watchBatch)) || tracePixel)
                            {
                                if (watchPixelFound)
                                {
                                    Vec4FP32 *attributes = quad[watchPixelPosInQuad]->getAttributes();
                                    CG_INFO("emuPrimAssembly => Output color for pixel (%d, %d) -> {%f, %f, %f, %f} [%02x, %02x, %02x, %02x]",
                                        watchPixelX, watchPixelY,
                                        attributes[COLOR_ATTRIBUTE][0], attributes[COLOR_ATTRIBUTE][1],
                                        attributes[COLOR_ATTRIBUTE][2], attributes[COLOR_ATTRIBUTE][3],
                                        U08(attributes[COLOR_ATTRIBUTE][0] * 255.0f), U08(attributes[COLOR_ATTRIBUTE][1] * 255.0f),
                                        U08(attributes[COLOR_ATTRIBUTE][2] * 255.0f), U08(attributes[COLOR_ATTRIBUTE][3] * 255.0f));

                                    traceFShader = false;
                                    traceTexture = false;
                                }
                            }
                        )
                        
                        //  Perform late z.
                        if (!state.earlyZ || state.modifyDepth)
                        {
                            emulateZStencilTest(quad);
                        }

                        CG1_BMDL_TRACE(
                            if (traceLog || (traceBatch && (batchCounter == watchBatch)) || tracePixel)
                            {
                                if (watchPixelFound)
                                {
                                    CG_INFO("emuPrimAssembly => Cull flag for pixel (%d, %d) after zstencil (late z) is %s",
                                        watchPixelX, watchPixelY, quad[watchPixelPosInQuad]->isCulled() ? "True" : "False");
                                }
                            }
                        )

                        //  Write/combine the shaded pixel color in/with the current color buffer.
                        emulateColorWrite(quad);

                        //  Delete fragment quad.
                        for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
                        {
                            delete quad[f]->getFragment();
                            delete quad[f];
                        }
                    }
                    else // if (notAllFragmentsCulled)
                    {
                        CG_INFO("All fragments in the quad culled");
                        for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
                            delete stamp[f];
                    }

                    //  Delete the arrays of pointers to fragments for the current quad.
                    delete[] stamp;
                }
                else
                {
                    CG_INFO("Updating recursive algorithm");
                    bmRaster->updateRecursiveMultiv2(batchID); //  Update the triangle rasterization algorithm.
                }
            }
        }
        bmRaster->destroyTriangle(triangleID); //  Eliminate triangle.
    }

    //char filename[1024];
    //sprintf(filename, "frame%04d-batch%05d-triangle%09d", frameCounter, batchCounter, triangleCounter);
    //dumpFrame(filename);
    triangleCounter++;
    GLOBAL_PROFILER_EXIT_REGION()
}


void bmoGpuTop::setupDraw()
{
    //  Configure rasterizer.
    bmRaster->setViewport(state.d3d9PixelCoordinates, state.viewportIniX, state.viewportIniY, state.viewportWidth, state.viewportHeight);
    bmRaster->setScissor(state.displayResX, state.displayResY, state.scissorTest, state.scissorIniX, state.scissorIniY, state.scissorWidth, state.scissorHeight);
    bmRaster->setDepthRange(state.d3d9DepthRange, state.nearRange, state.farRange);
    bmRaster->setPolygonOffset(state.slopeFactor, state.unitOffset);
    bmRaster->setFaceMode(state.faceMode);
    bmRaster->setDepthPrecission(state.zBufferBitPrecission);
    bmRaster->setD3D9RasterizationRules(state.d3d9RasterizationRules);
    
    //  Configure the blend emulation for all render targets.
    for(U32 rt = 0; rt < MAX_RENDER_TARGETS; rt++)
    {
        //  Configure the blend emulation for a render target.
        bmFragOp->setBlending(rt, state.blendEquation[rt], state.blendSourceRGB[rt], state.blendSourceAlpha[rt],
                             state.blendDestinationRGB[rt], state.blendDestinationAlpha[rt], state.blendColor[rt]);
    }
    //  Configure logical operation emulation.
    bmFragOp->setLogicOpMode(state.logicOpFunction);


    for(U32 rt = 0; rt < MAX_RENDER_TARGETS; rt++)
    {
        //  Set display parameters in the Pixel Mapper.
        U32 samples = state.multiSampling ? state.msaaSamples : 1;
        U32 bytesPixel;

        switch(state.rtFormat[rt])
        {
            case GPU_RGBA8888:
            case GPU_RG16F:
            case GPU_R32F:
                bytesPixel = 4;
                break;
            case GPU_RGBA16:
            case GPU_RGBA16F:
                bytesPixel = 8;
                break;
        }

        pixelMapper[rt].setupDisplay(state.displayResX, state.displayResY, STAMP_WIDTH, STAMP_WIDTH,
                                 ArchConf.ras.genWidth / STAMP_WIDTH, ArchConf.ras.genHeight / STAMP_HEIGHT,
                                 ArchConf.ras.scanWidth / ArchConf.ras.genWidth, ArchConf.ras.scanHeight / ArchConf.ras.genHeight,
                                 ArchConf.ras.overScanWidth, ArchConf.ras.overScanHeight,
                                 samples, bytesPixel);
    
        bytesPerPixelColor[rt] = bytesPixel;
    }
    
    U32 samples = state.multiSampling ? state.msaaSamples : 1;
    U32 bytesPixel = 4;
    bytesPerPixelDepth = bytesPixel;
    zPixelMapper.setupDisplay(state.displayResX, state.displayResY, STAMP_WIDTH, STAMP_WIDTH,
                             ArchConf.ras.genWidth / STAMP_WIDTH, ArchConf.ras.genHeight / STAMP_HEIGHT,
                             ArchConf.ras.scanWidth / ArchConf.ras.genWidth, ArchConf.ras.scanHeight / ArchConf.ras.genHeight,
                             ArchConf.ras.overScanWidth, ArchConf.ras.overScanHeight,
                             samples, bytesPixel);

    //  Configure Z test emulation.
    bmFragOp->configureZTest(state.depthFunction, state.depthMask);
    bmFragOp->setZTest(state.depthTest);

    //  Configure stencil test emulation.
    bmFragOp->configureStencilTest(state.stencilFunction, state.stencilReference,
        state.stencilTestMask, state.stencilUpdateMask, state.stencilFail, state.depthFail, state.depthPass);
    bmFragOp->setStencilTest(state.stencilTest);

    //  Reset the triangle counter.
    triangleCounter = 0;
}

bool bmoGpuTop::cullTriangle(U32 triangleID)
{
    bool dropTriangle;
    F32 tArea;

    //  Get the triangle signed area.
    tArea = bmRaster->triangleArea(triangleID);

    GPU_DEBUG(
        CG_INFO("Triangle area is %f", tArea);
    )

    //  Check face culling mode.
    switch(state.cullMode)
    {
        case NONE:

            //  If area is negative.
            if (tArea < 0)
            {
                //  Change the triangle edge equation signs so backfacing triangles can be rasterized.
                bmRaster->invertTriangleFacing(triangleID);

                GPU_DEBUG(
                    CG_INFO("Cull NONE.  Inverting back facing triangle.");
                )

            }

            //  Do not drop any triangle because of face.
            dropTriangle = false;

            break;

        case FRONT:

            //  If area is positive and front face culling.
            if (tArea > 0)
            {
                //  Drop front facing triangle.
                dropTriangle = true;

                GPU_DEBUG(
                    CG_INFO("Cull FRONT.  Culling front facing triangle.");
                )
            }
            else
            {
                //  Change the triangle edge equation signs so back facing triangles can be rasterized.
                bmRaster->invertTriangleFacing(triangleID);

                //  Do not drop back facing triangles.
                dropTriangle = false;

                GPU_DEBUG(
                    CG_INFO("Cull FRONT.  Inverting back facing triangle.");
                )
            }

            break;

        case BACK:

            //  If area is negative and back face culling is selected.
            if (tArea < 0)
            {
                //  Drop back facing triangles.
                dropTriangle = true;

                GPU_DEBUG(
                    CG_INFO("Cull BACK.  Culling back facing triangle.");
                )
            }
            else
            {
                //  Do not drop front facing triangles.
                dropTriangle = false;
            }

            break;

        case FRONT_AND_BACK:

            //  Drop all triangles.
            dropTriangle = true;

            GPU_DEBUG(
                CG_INFO("Cull FRONT_AND_BACK.  Culling triangle.");
            )

            break;

        default:

            CG_ASSERT("Unsupported triangle culling mode.");
            break;
    }

    //  Check the triangle area.
    if (!dropTriangle && (tArea == 0.0f))
    {
        //  Drop the triangle.
        dropTriangle = true;

        GPU_DEBUG(
            CG_INFO("Triangle with zero area culled.");
        )
    }

    return dropTriangle;
}

void bmoGpuTop::emulateFragmentShading(ShadedFragment **quad)
{
    GLOBAL_PROFILER_ENTER_REGION("emulateFragmentShading", "", "emulateFragmentShading");
    GLOBAL_PROFILER_ENTER_REGION("emulateFragmentShading (load attributes)", "", "emulateFragmentShading")
    //  Initialize four threads for the fragment quad.
    for(U32 p = 0; p < STAMP_FRAGMENTS; p++)
    {
        //  Initialize thread for the current fragment.
        bmShader->resetShaderState(p);
        //  Load the new shader input into the shader input register bank of the thread element in the shader behaviorModel.
        bmShader->loadShaderState(p, cg1gpu::IN, quad[p]->getAttributes());
        //  Set PC for the thread element in the shader behaviorModel to the start PC of the vector thread.
        bmShader->setThreadPC(p, state.fragProgramStartPC);
    }
    GLOBAL_PROFILER_EXIT_REGION()
    bool programEnd = false;
    bool fragmentEnd[4];
    fragmentEnd[0] = fragmentEnd[1] = fragmentEnd[2] = fragmentEnd[3] = false;
    U32 pc = state.fragProgramStartPC;

    U32 p;
    cgoShaderInstr::cgoShaderInstrEncoding *shDecInstr;
    TextureAccess *texAccess;

    //  Emulation trace generation macro.
    CG1_BMDL_TRACE(        
        if (traceLog || (traceBatch && (batchCounter == watchBatch)))
        {
            CG_INFO("FSh => Executing program at %04x for pixels: ", pc);
            for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
                CG_INFO("    P%d -> (%4d, %4d) ", f, quad[f]->getFragment()->getX(), quad[f]->getFragment()->getY());
        }
    )
    //  Execute all the instructions in the program.
    while (!programEnd)
    {
        bool jump = false;
        U32 destPC = 0;
        
        //  Execute the instruction for the four fragment/threads in the current quad.
        for(p = 0; p < STAMP_FRAGMENTS; p++)
        {
            //  Check if the program already finished for this fragment.
            //if (!fragmentEnd[p])
            //{
                GLOBAL_PROFILER_ENTER_REGION("emulateFragmentShading (fetch)", "", "emulateFragmentShading")

                //  Fetch the instruction.
                shDecInstr = bmShader->FetchInstr(p, pc);

                GLOBAL_PROFILER_EXIT_REGION()

                GPU_ASSERT(
                    if (shDecInstr == NULL)
                    {
                        char buffer[256];
                        sprintf(buffer, "Error fetching fragment program instruction at %02x", pc);
                        CG_ASSERT(buffer);
                    }
                )

                CG1_BMDL_TRACE(
                    if (traceFShader)
                    {
                        char shInstrDisasm[256];
                        shDecInstr->getShaderInstruction()->disassemble(shInstrDisasm, 256);
                        CG_INFO("P%1d => %04x : %s", p, pc, shInstrDisasm);
                        
                        printShaderInstructionOperands(shDecInstr);
                    }
                )
                
                GPU_DEBUG(
                    char shInstrDisasm[256];
                    shDecInstr->getShaderInstruction()->disassemble(shInstrDisasm, 256);
                    CG_INFO("FSh => Executing instruction @ %04x : %s", pc, shInstrDisasm);
                )
                GLOBAL_PROFILER_ENTER_REGION("emulateFragmentShading (exec)", "", "emulateFragmentShading")
                bmShader->execShaderInstruction(shDecInstr); //  Execute instruction.
                GLOBAL_PROFILER_EXIT_REGION();
                //  Check for jump instructions (only the first thread in the 4-way vector).
                if ((p == 0) && shDecInstr->getShaderInstruction()->isAJump())
                {
                    jump = bmShader->checkJump(shDecInstr, 4, destPC); //  Check if the jump is performed.
                }
                //  Check if this is the last instruction in the program.
                fragmentEnd[p] = shDecInstr->getShaderInstruction()->getEndFlag() || bmShader->threadKill(p);

                CG1_BMDL_TRACE(
                    if (traceFShader)
                    {
                        printShaderInstructionResult(shDecInstr);
                        CG_INFO("             CG1_ISA_OPCODE_KILL MASK -> %s", bmShader->threadKill(p) ? "true" : "false");
                        CG_INFO("             FRAGMENT CG1_ISA_OPCODE_END -> %s", fragmentEnd[p] ? "true" : "false");
                        if (p == (STAMP_FRAGMENTS - 1))
                            CG_INFO("-------------------");

                        fflush(stdout);
                    }
                )
            //}
            
        }
        texAccess = bmShader->nextTextureAccess(); //  Process texture requests.  Texture requests are processed per fragment quad.
        if (texAccess != NULL) //  Check if there is a pending texture request from the previous shader instruction.
        {
            emulateTextureUnit(texAccess); //  Process the texture requests.
        }
        //  Program finishes when the four fragments in the quad finish.
        programEnd = fragmentEnd[0] && fragmentEnd[1] && fragmentEnd[2] && fragmentEnd[3];

        //  Update PC.
        if (jump)
        {
            pc = destPC;
            
            CG1_BMDL_TRACE(
                if (traceFShader)
                    CG_INFO("Jumping to PC %04x", pc);
            )
        }
        else
            pc++;
    }
   
    GLOBAL_PROFILER_ENTER_REGION("emulateFragmentShading (retrieve attributes)", "", "emulateFragmentShading")
    //  Get the result attributes for the four fragments in the quad.
    for(U32 p = 0; p < STAMP_FRAGMENTS; p++)
    {
        //  Get output attributes for the fragment.
        bmShader->readShaderState(p, cg1gpu::OUT, quad[p]->getAttributes());

        //  Set fragment as culled if the fragment was killed.
        if (bmShader->threadKill(p))
            quad[p]->setAsCulled();

        GPU_DEBUG(
            CG_INFO("FSh => Fragment Outputs :");
            //for(U32 a = 0; a < MAX_FRAGMENT_ATTRIBUTES; a++)
            for(U32 a = 0; a < 5; a++)
            {
                Vec4FP32 *attributes = quad[p]->getAttributes();
                CG_INFO(" OUT[%02d] = {%f, %f, %f, %f}", a, attributes[a][0], attributes[a][1], attributes[a][2], attributes[a][3]);
            }
        )
    }
    GLOBAL_PROFILER_EXIT_REGION()
    GLOBAL_PROFILER_EXIT_REGION()
}


//  Emulate the texture unit.
void bmoGpuTop::emulateTextureUnit(TextureAccess *texAccess)
{
    GLOBAL_PROFILER_ENTER_REGION("emulateTextureUnit", "", "emulateTextureUnit")

    //  Calculate addresses for all the aniso samples required for the texture request.
    for(texAccess->currentAnisoSample = 1; texAccess->currentAnisoSample <= texAccess->anisoSamples; texAccess->currentAnisoSample++)
        bmTexture->calculateAddress(texAccess);

    //  Read texture data from memory.
    for(U32 s = 0; s < texAccess->anisoSamples; s++)
    {
        //  For all fragments in the quad.
        for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
        {
            //  For all texels to read.
            for(U32 t = 0; t < texAccess->trilinear[s]->texelsLoop[f] * texAccess->trilinear[s]->loops[f]; t++)
            {
                //  Read and convert texel data.
                U08 data[128];

                //  Read memory.
                readTextureData(texAccess->trilinear[s]->address[f][t], texAccess->texelSize[f], data);

                //  Convert to internal format.
                bmTexture->convertFormat(*texAccess, s, f, t, data);
                
                /*if (batchCounter == 51) {
                 CG_INFO("         Float32 (%f, %f) = (%f, %f, %f, %f)",
                     texAccess->coordinates[f][0], texAccess->coordinates[f][1],
                     texAccess->trilinear[s]->texel[f][t][0], texAccess->trilinear[s]->texel[f][t][1],
                     texAccess->trilinear[s]->texel[f][t][2], texAccess->trilinear[s]->texel[f][t][3]);
                }*/
            }
        }
    }

    //  Filter the texture request.
    for(U32 s = 0; s < texAccess->anisoSamples; s++)
        bmTexture->filter(*texAccess, s);

    CG1_BMDL_TRACE(
        if (traceTexture)
            printTextureAccessInfo(texAccess);
    )
    
    U32 threads[STAMP_FRAGMENTS];

    //  Send texture results to the shader behaviorModel.
    bmShader->writeTextureAccess(texAccess->accessID, texAccess->sample, threads, false);

    delete texAccess;

    GLOBAL_PROFILER_EXIT_REGION()
}

void bmoGpuTop::readTextureData(U64 texelAddress, U32 size, U08 *data)
{

    //  Check for black texel address (out of bounds).
    if (texelAddress == BLACK_TEXEL_ADDRESS)
    {
        //  Return zeros.
        for(U32 b = 0; b < size; b++)
            data[b] = 0;

        return;
    }

    //
    //  WARNING!!! Texel addresses must be aligned to 4 bytes before accessing memory.
    //

    //  Check if the address is for a compressed texture.
    switch(texelAddress & TEXTURE_ADDRESS_SPACE_MASK)
    {
        case UNCOMPRESSED_TEXTURE_SPACE:
            {
                U08 *memory = selectMemorySpace(U32(texelAddress & 0xffffffff));
                U32 address = U32(texelAddress & 0xfffffffc) & SPACE_ADDRESS_MASK;

                for(U32 b = 0; b < size; b++)
                    data[b] = memory[address + b];
            }

            break;

        case COMPRESSED_TEXTURE_SPACE_DXT1_RGB:

            cacheDXT1RGB.readData(texelAddress, data, size);

            break;


        case COMPRESSED_TEXTURE_SPACE_DXT1_RGBA:

            cacheDXT1RGBA.readData(texelAddress, data, size);

            break;

        case COMPRESSED_TEXTURE_SPACE_DXT3_RGBA:

            cacheDXT3RGBA.readData(texelAddress, data, size);

            break;

        case COMPRESSED_TEXTURE_SPACE_DXT5_RGBA:

            cacheDXT5RGBA.readData(texelAddress, data, size);

            break;

        case COMPRESSED_TEXTURE_SPACE_LATC1:

            cacheLATC1.readData(texelAddress, data, size);

            break;

        case COMPRESSED_TEXTURE_SPACE_LATC1_SIGNED:

            cacheLATC1_SIGNED.readData(texelAddress, data, size);

            break;

        case COMPRESSED_TEXTURE_SPACE_LATC2:

            cacheLATC2.readData(texelAddress, data, size);

            break;

        case COMPRESSED_TEXTURE_SPACE_LATC2_SIGNED:

            cacheLATC2_SIGNED.readData(texelAddress, data, size);

            break;
        default:
            CG_ASSERT("Unsupported texture address space.");
            break;
    }

}

void bmoGpuTop::emulateColorWrite(ShadedFragment **quad)
{
    GLOBAL_PROFILER_ENTER_REGION("emulateColorWrite", "", "emulateColorWrite")

    U32 bytesPixel;
    
    //  Write output for all render targets.
    for(U32 rt = 0; rt < MAX_RENDER_TARGETS; rt++)
    {
        //  Check if the render targets is enabled and color mask is true.
        if (state.rtEnable[rt] && ((state.colorMaskR[rt] || state.colorMaskG[rt] || state.colorMaskB[rt] || state.colorMaskA[rt])))
        {
            Vec4FP32 destColorQF[STAMP_FRAGMENTS * MAX_MSAA_SAMPLES];
            Vec4FP32 inputColorQF[STAMP_FRAGMENTS * MAX_MSAA_SAMPLES];
            bool writeMask[STAMP_FRAGMENTS * MAX_MSAA_SAMPLES];
            U08 *colorData;

            //  Compute the address in memory for the fragment quad.
            U32 address = pixelMapper[rt].computeAddress(quad[0]->getFragment()->getX(), quad[0]->getFragment()->getY());

            //  Add back buffer base address to the address inside the color buffer.
            address += state.rtAddress[rt];

            U08 *memory = selectMemorySpace(address);
            address = address & SPACE_ADDRESS_MASK;

            colorData = &memory[address];

            for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
            {
                //  Get fragment attributes.
                Vec4FP32 *attributes = quad[f]->getAttributes();

                //  Get fragment.
                Fragment *fr = quad[f]->getFragment();

/*if ((fr->getX() == 204) && (fr->getY() == 312))
{
bool *coverage = fr->getMSAACoverage();
CG_INFO("render target = %d", rt);
CG_INFO("coverage = %d %d %d %d", coverage[0], coverage[1], coverage[2], coverage[3]);
CG_INFO("color = %f %f %f %f", GPU_CLAMP(attributes[COLOR_ATTRIBUTE + rt][0], 0.0f, 1.0f),
GPU_CLAMP(attributes[COLOR_ATTRIBUTE + rt][1], 0.0f, 1.0f),
GPU_CLAMP(attributes[COLOR_ATTRIBUTE + rt][2], 0.0f, 1.0f),
GPU_CLAMP(attributes[COLOR_ATTRIBUTE + rt][3], 0.0f, 1.0f));
}*/

                //  Check if multisampling is enabled.
                if (!state.multiSampling)
                {
                    //  Get fragment input color from color attribute.
                    if (state.rtFormat[rt] == GPU_RGBA8888)
                    {
                        inputColorQF[f][0] = GPU_CLAMP(attributes[COLOR_ATTRIBUTE + rt][0], 0.0f, 1.0f);
                        inputColorQF[f][1] = GPU_CLAMP(attributes[COLOR_ATTRIBUTE + rt][1], 0.0f, 1.0f);
                        inputColorQF[f][2] = GPU_CLAMP(attributes[COLOR_ATTRIBUTE + rt][2], 0.0f, 1.0f);
                        inputColorQF[f][3] = GPU_CLAMP(attributes[COLOR_ATTRIBUTE + rt][3], 0.0f, 1.0f);
                    }
                    else
                    {
                        inputColorQF[f][0] = attributes[COLOR_ATTRIBUTE + rt][0];
                        inputColorQF[f][1] = attributes[COLOR_ATTRIBUTE + rt][1];
                        inputColorQF[f][2] = attributes[COLOR_ATTRIBUTE + rt][2];
                        inputColorQF[f][3] = attributes[COLOR_ATTRIBUTE + rt][3];
                    }

                    //  Build write mask for the fragment.
                    //switch(state.colorBufferFormat)
                    switch(state.rtFormat[rt])
                    {
                        case GPU_RGBA8888:

                            writeMask[f * 4    ] = state.colorMaskR[rt] && !quad[f]->isCulled();
                            writeMask[f * 4 + 1] = state.colorMaskG[rt] && !quad[f]->isCulled();
                            writeMask[f * 4 + 2] = state.colorMaskB[rt] && !quad[f]->isCulled();
                            writeMask[f * 4 + 3] = state.colorMaskA[rt] && !quad[f]->isCulled();
                            break;

                        case GPU_RG16F:

                            writeMask[f * 4    ] = writeMask[f * 4 + 1] = state.colorMaskR[rt] && !quad[f]->isCulled();
                            writeMask[f * 4 + 2] = writeMask[f * 4 + 3] = state.colorMaskG[rt] && !quad[f]->isCulled();
                            break;

                        case GPU_R32F:

                            writeMask[f * 4    ] = writeMask[f * 4 + 1] =
                            writeMask[f * 4 + 2] = writeMask[f * 4 + 3] = state.colorMaskR[rt] && !quad[f]->isCulled();
                            break;

                        case GPU_RGBA16:
                            writeMask[f * 8 + 0] = writeMask[f * 8 + 1] = state.colorMaskR[rt] && !quad[f]->isCulled();
                            writeMask[f * 8 + 2] = writeMask[f * 8 + 3] = state.colorMaskG[rt] && !quad[f]->isCulled();
                            writeMask[f * 8 + 4] = writeMask[f * 8 + 5] = state.colorMaskB[rt] && !quad[f]->isCulled();
                            writeMask[f * 8 + 6] = writeMask[f * 8 + 7] = state.colorMaskA[rt] && !quad[f]->isCulled();
                            break;

                        case GPU_RGBA16F:

                            writeMask[f * 8 + 0] = writeMask[f * 8 + 1] = state.colorMaskR[rt] && !quad[f]->isCulled();
                            writeMask[f * 8 + 2] = writeMask[f * 8 + 3] = state.colorMaskG[rt] && !quad[f]->isCulled();
                            writeMask[f * 8 + 4] = writeMask[f * 8 + 5] = state.colorMaskB[rt] && !quad[f]->isCulled();
                            writeMask[f * 8 + 6] = writeMask[f * 8 + 7] = state.colorMaskA[rt] && !quad[f]->isCulled();
                            break;
                    }
                }
                else
                {
                    //  Get sample coverage mask for the fragment.
                    bool *sampleCoverage = fr->getMSAACoverage();

                    //  Copy shaded fragment color to all samples for the fragment.
                    for(U32 s = 0; s < state.msaaSamples; s++)
                    {
                        //  Get fragment input color from color attribute.
                        //if (state.colorBufferFormat == GPU_RGBA8888)
                        if (state.rtFormat[rt] == GPU_RGBA8888)
                        {
                            inputColorQF[f * state.msaaSamples + s][0] = GPU_CLAMP(attributes[COLOR_ATTRIBUTE + rt][0], 0.0f, 1.0f);
                            inputColorQF[f * state.msaaSamples + s][1] = GPU_CLAMP(attributes[COLOR_ATTRIBUTE + rt][1], 0.0f, 1.0f);
                            inputColorQF[f * state.msaaSamples + s][2] = GPU_CLAMP(attributes[COLOR_ATTRIBUTE + rt][2], 0.0f, 1.0f);
                            inputColorQF[f * state.msaaSamples + s][3] = GPU_CLAMP(attributes[COLOR_ATTRIBUTE + rt][3], 0.0f, 1.0f);
                        }
                        else
                        {
                            inputColorQF[f * state.msaaSamples + s][0] = attributes[COLOR_ATTRIBUTE + rt][0];
                            inputColorQF[f * state.msaaSamples + s][1] = attributes[COLOR_ATTRIBUTE + rt][1];
                            inputColorQF[f * state.msaaSamples + s][2] = attributes[COLOR_ATTRIBUTE + rt][2];
                            inputColorQF[f * state.msaaSamples + s][3] = attributes[COLOR_ATTRIBUTE + rt][3];
                        }

                        //  Build write mask for the fragment samples.
                        //switch(state.colorBufferFormat)
                        switch(state.rtFormat[rt])
                        {
                            case GPU_RGBA8888:

                                writeMask[(f * state.msaaSamples + s) * 4 + 0] = state.colorMaskR[rt] && sampleCoverage[s];
                                writeMask[(f * state.msaaSamples + s) * 4 + 1] = state.colorMaskG[rt] && sampleCoverage[s];
                                writeMask[(f * state.msaaSamples + s) * 4 + 2] = state.colorMaskB[rt] && sampleCoverage[s];
                                writeMask[(f * state.msaaSamples + s) * 4 + 3] = state.colorMaskA[rt] && sampleCoverage[s];
                                break;

                            case GPU_RG16F:

                                writeMask[(f * state.msaaSamples + s) * 4 + 0] =
                                writeMask[(f * state.msaaSamples + s) * 4 + 1] = state.colorMaskR[rt] && sampleCoverage[s];
                                writeMask[(f * state.msaaSamples + s) * 4 + 2] =
                                writeMask[(f * state.msaaSamples + s) * 4 + 3] = state.colorMaskG[rt] && sampleCoverage[s];
                                break;

                            case GPU_R32F:

                                writeMask[(f * state.msaaSamples + s) * 4 + 0] =
                                writeMask[(f * state.msaaSamples + s) * 4 + 1] =
                                writeMask[(f * state.msaaSamples + s) * 4 + 2] =
                                writeMask[(f * state.msaaSamples + s) * 4 + 3] = state.colorMaskR[rt] && sampleCoverage[s];

                                break;

                            case GPU_RGBA16:
                                
                                writeMask[(f * state.msaaSamples + s) * 8 + 0] =
                                writeMask[(f * state.msaaSamples + s) * 8 + 1] = state.colorMaskR[rt] && sampleCoverage[s];
                                writeMask[(f * state.msaaSamples + s) * 8 + 2] =
                                writeMask[(f * state.msaaSamples + s) * 8 + 3] = state.colorMaskG[rt] && sampleCoverage[s];
                                writeMask[(f * state.msaaSamples + s) * 8 + 4] =
                                writeMask[(f * state.msaaSamples + s) * 8 + 5] = state.colorMaskB[rt] && sampleCoverage[s];
                                writeMask[(f * state.msaaSamples + s) * 8 + 6] =
                                writeMask[(f * state.msaaSamples + s) * 8 + 7] = state.colorMaskA[rt] && sampleCoverage[s];
                                
                                break;
                                
                            case GPU_RGBA16F:
                            
                                writeMask[(f * state.msaaSamples + s) * 8 + 0] =
                                writeMask[(f * state.msaaSamples + s) * 8 + 1] = state.colorMaskR[rt] && sampleCoverage[s];
                                writeMask[(f * state.msaaSamples + s) * 8 + 2] =
                                writeMask[(f * state.msaaSamples + s) * 8 + 3] = state.colorMaskG[rt] && sampleCoverage[s];
                                writeMask[(f * state.msaaSamples + s) * 8 + 4] =
                                writeMask[(f * state.msaaSamples + s) * 8 + 5] = state.colorMaskB[rt] && sampleCoverage[s];
                                writeMask[(f * state.msaaSamples + s) * 8 + 6] =
                                writeMask[(f * state.msaaSamples + s) * 8 + 7] = state.colorMaskA[rt] && sampleCoverage[s];
                                break;
                        }
                    }
                }
            }

            GPU_DEBUG(
                CG_INFO("Write Mask = ");
                for(U32 b = 0; b < (STAMP_FRAGMENTS * 4); b++)
                    CG_INFO("%d ", writeMask[b]);
                CG_INFO("");

                CG_INFO("Color Data = ");
                for(U32 b = 0; b < (STAMP_FRAGMENTS * 4); b++)
                    CG_INFO("%02x ", colorData[b]);
                CG_INFO("");
            )

            //  Check if multisampling is enabled.
            if (!state.multiSampling)
            {
                //  Convert color buffer data to internal representation (RGBA32F).
                //switch(state.colorBufferFormat)
                switch(state.rtFormat[rt])
                {
                    case GPU_RGBA8888:

                        colorRGBA8ToRGBA32F(colorData, destColorQF);
                        break;

                    case GPU_RG16F:

                        colorRG16FToRGBA32F(colorData, destColorQF);
                        break;

                    case GPU_R32F:

                        colorR32FToRGBA32F(colorData, destColorQF);
                        break;

                    case GPU_RGBA16:

                        colorRGBA16ToRGBA32F(colorData, destColorQF);
                        break;

                    case GPU_RGBA16F:

                        colorRGBA16FToRGBA32F(colorData, destColorQF);
                        break;
                }
                
                //  Convert from sRGB to linear space if required.
                if (state.colorSRGBWrite)
                    colorSRGBToLinear(destColorQF);
            }
            else
            {
                //  Convert fixed point color data to float point data for all the samples in the stamp
                for(U32 s = 0; s < state.msaaSamples; s++)
                {
                    //  Convert color buffer data to internal representation (RGBA32F).
                    switch(state.rtFormat[rt])
                    {
                        case GPU_RGBA8888:

                            colorRGBA8ToRGBA32F(&colorData[s * STAMP_FRAGMENTS * 4], &destColorQF[STAMP_FRAGMENTS * s]);
                            break;

                        case GPU_RG16F:

                            colorRG16FToRGBA32F(&colorData[s * STAMP_FRAGMENTS * 4], &destColorQF[STAMP_FRAGMENTS * s]);
                            break;

                        case GPU_R32F:

                            colorR32FToRGBA32F(&colorData[s * STAMP_FRAGMENTS * 4], &destColorQF[STAMP_FRAGMENTS * s]);
                            break;

                        case GPU_RGBA16:

                            colorRGBA16ToRGBA32F(&colorData[s * STAMP_FRAGMENTS * 8], &destColorQF[STAMP_FRAGMENTS * s]);
                            break;

                        case GPU_RGBA16F:

                            colorRGBA16FToRGBA32F(&colorData[s * STAMP_FRAGMENTS * 8], &destColorQF[STAMP_FRAGMENTS * s]);
                            break;
                    }
                    
                    //  Convert from sRGB to linear space if required.
                    if (state.colorSRGBWrite)
                        colorSRGBToLinear(&destColorQF[STAMP_FRAGMENTS * s]);
                }
            }

/*if (batchCounter == 622) {
    for(U32 b = 0; b < (STAMP_FRAGMENTS); b++) {
        CG_INFO("Source Data = ");
        CG_INFO("(%f, %f, %f, %f)", inputColorQF[b][0], inputColorQF[b][1], inputColorQF[b][2], inputColorQF[b][3]);
        CG_INFO("Dest Data = ");
        CG_INFO("(%f, %f, %f, %f)", destColorQF[b][0], destColorQF[b][1], destColorQF[b][2], destColorQF[b][3]);
    }
}*/
            FragmentQuadMemoryUpdateInfo quadMemUpdate;
                            
            //  Determine if blend mode is active.
            if (state.colorBlend[rt])
            {
                //  Check if multisampling is enabled.
                if (!state.multiSampling)
                {
                    //  Check if validation mode is enabled.
                    if (validationMode)
                    {
                        //  Check if there are unculled fragments in the quad.
                        bool anyNotAlreadyCulled = false;            
                        for(U32 f = 0; (f < STAMP_FRAGMENTS) && !anyNotAlreadyCulled; f++)
                            anyNotAlreadyCulled = anyNotAlreadyCulled || !quad[f]->isCulled();

                        //  Store the information for the quad if there is a valid fragment.
                        if (anyNotAlreadyCulled)
                        {
                            //  Set the quad identifier to the top left fragment of the quad.
                            quadMemUpdate.fragID.x = quad[0]->getFragment()->getX();
                            quadMemUpdate.fragID.y = quad[0]->getFragment()->getY();
                            quadMemUpdate.fragID.triangleID = triangleCounter;
                            
                            quadMemUpdate.fragID.sample = 0;

                            //  Copy the input (color computed per fragment) and read data (color from the render target) for
                            //  the quad.
                            for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
                            {
                                ((F32 *) quadMemUpdate.inData)[f * 4 + 0] = inputColorQF[f][0];
                                ((F32 *) quadMemUpdate.inData)[f * 4 + 1] = inputColorQF[f][1];
                                ((F32 *) quadMemUpdate.inData)[f * 4 + 2] = inputColorQF[f][2];
                                ((F32 *) quadMemUpdate.inData)[f * 4 + 3] = inputColorQF[f][3];
                            }
                            for(U32 qw = 0; qw < ((bytesPerPixelColor[rt] * 4) >> 3); qw++)
                                ((U64 *) quadMemUpdate.readData)[qw] = ((U64 *) colorData)[qw];
                        }
                    }
                    
                    //  Perform blend operation.  
                    bmFragOp->blend(rt, inputColorQF, inputColorQF, destColorQF);
                }
                else
                {
                    //  Perform blending for all the samples in the stamp.
                    for(U32 s = 0; s < state.msaaSamples; s++)
                    {
                        //  Perform blending for a group of samples.
                        bmFragOp->blend(rt, &inputColorQF[STAMP_FRAGMENTS * s], &inputColorQF[STAMP_FRAGMENTS * s],
                                       &destColorQF[STAMP_FRAGMENTS * s]);
                    }
                }
            }
            else
            {
                //  Check if validation mode is enabled.
                if (validationMode)
                {
                    //  Check if there are unculled fragments in the quad.
                    bool anyNotAlreadyCulled = false;            
                    for(U32 f = 0; (f < STAMP_FRAGMENTS) && !anyNotAlreadyCulled; f++)
                        anyNotAlreadyCulled = anyNotAlreadyCulled || !quad[f]->isCulled();

                    //  Store the information for the quad if there is a valid fragment.
                    if (anyNotAlreadyCulled)
                    {
                        //  Set the quad identifier to the top left fragment of the quad.
                        quadMemUpdate.fragID.x = quad[0]->getFragment()->getX();
                        quadMemUpdate.fragID.y = quad[0]->getFragment()->getY();
                        quadMemUpdate.fragID.triangleID = triangleCounter;
                        
                        quadMemUpdate.fragID.sample = 0;

                        //  Copy the input (color computed per fragment) and read data (color from the render target) for
                        //  the quad.
                        for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
                        {
                            ((F32 *) quadMemUpdate.inData)[f * 4 + 0] = inputColorQF[f][0];
                            ((F32 *) quadMemUpdate.inData)[f * 4 + 1] = inputColorQF[f][1];
                            ((F32 *) quadMemUpdate.inData)[f * 4 + 2] = inputColorQF[f][2];
                            ((F32 *) quadMemUpdate.inData)[f * 4 + 3] = inputColorQF[f][3];
                        }
                        for(U32 qw = 0; qw < ((bytesPerPixelColor[rt] * 4) >> 3); qw++)
                            ((U64 *) quadMemUpdate.readData)[qw] = 0xDEFADA7ADEFADA7AULL;
                    }
                }
            }

/*if (batchCounter == 622) {
    for(U32 b = 0; b < (STAMP_FRAGMENTS); b++) {
        CG_INFO("Color Data = ");
        CG_INFO("(%f, %f, %f, %f)", inputColorQF[b][0], inputColorQF[b][1], inputColorQF[b][2], inputColorQF[b][3]);
    }
}*/

            U08 outColor[STAMP_FRAGMENTS * MAX_MSAA_SAMPLES * 8];
            U32 bytesPerSample;

            GPU_DEBUG(
                CG_INFO("Color Data = ");
                for(U32 b = 0; b < (STAMP_FRAGMENTS * 4); b++)
                    CG_INFO("%02x ", colorData[b]);
                CG_INFO("");
            )

            //  Determine if logical operation is active.
            if ((rt == 0) && state.logicalOperation)
            {
                GPU_ASSERT(
                    //if (state.colorBufferFormat != GPU_RGBA8888)
                    if (state.rtFormat[rt] != GPU_RGBA8888)
                        CG_ASSERT("Logic operation only supported with RGBA8 color buffer format.");
                )

                bytesPerSample = 4;
                
                //  Check if multisampling is enabled
                if (!state.multiSampling)
                {
                    //  Convert output color from linear space to sRGB space if required.
                    if (state.colorSRGBWrite)
                        colorLinearToSRGB(inputColorQF);
            
                    //  Convert the stamp color data to integer format.
                    colorRGBA32FToRGBA8(inputColorQF, outColor);

                    //  Perform logical operation.
                    bmFragOp->logicOp(outColor, colorData, outColor);
                }
                else
                {
                    //  Convert to integer format and perform logical op for all the samples in the stamp.
                    for(U32 s = 0; s < state.msaaSamples; s++)
                    {
                        //  Convert output color from linear space to sRGB space if required.
                        if (state.colorSRGBWrite)
                            colorLinearToSRGB(&inputColorQF[STAMP_FRAGMENTS * s]);

                        //  Convert the sample color data to integer format.
                        colorRGBA32FToRGBA8(&inputColorQF[STAMP_FRAGMENTS * s], &outColor[STAMP_FRAGMENTS * s * 4]);

                        //  Perform logical operation for a group of samples.
                        bmFragOp->logicOp(&outColor[STAMP_FRAGMENTS * s * 4], &colorData[STAMP_FRAGMENTS * s * 4], &outColor[STAMP_FRAGMENTS * s * 4]);
                    }
                }
            }
            else
            {
                //  Check if multisampling is enabled.
                if (!state.multiSampling)
                {
                    //  Convert output color from linear space to sRGB space if required.
                    if (state.colorSRGBWrite)
                        colorLinearToSRGB(inputColorQF);
                        
                    //  Convert the stamp color data in internal format to the color buffer format.
                    //switch(state.colorBufferFormat)
                    switch(state.rtFormat[rt])
                    {
                        case GPU_RGBA8888:

                            colorRGBA32FToRGBA8(inputColorQF, outColor);
                            bytesPerSample = 4;
                            break;

                        case GPU_RG16F:

                            colorRGBA32FToRG16F(inputColorQF, outColor);
                            bytesPerSample = 4;
                            break;

                        case GPU_R32F:

                            colorRGBA32FToR32F(inputColorQF, outColor);
                            bytesPerSample = 4;
                            break;

                        case GPU_RGBA16:

                            colorRGBA32FToRGBA16(inputColorQF, outColor);
                            bytesPerSample = 8;
                            break;

                        case GPU_RGBA16F:

                            colorRGBA32FToRGBA16F(inputColorQF, outColor);
                            bytesPerSample = 8;
                            break;
                    }
                }
                else
                {
                    //  Convert the sample color data for all the stamp.
                    for(U32 s = 0; s < state.msaaSamples; s++)
                    {
                        //  Convert output color from linear space to sRGB space if required.
                        if (state.colorSRGBWrite)
                            colorLinearToSRGB(&inputColorQF[STAMP_FRAGMENTS * s]);

                        //  Convert the stamp color data in internal format to the color buffer format.
                        //switch(state.colorBufferFormat)
                        switch(state.rtFormat[rt])
                        {
                            case GPU_RGBA8888:

                                colorRGBA32FToRGBA8(&inputColorQF[STAMP_FRAGMENTS * s], &outColor[STAMP_FRAGMENTS * s * 4]);
                                bytesPerSample = 4;
                                break;

                            case GPU_RG16F:

                                colorRGBA32FToRG16F(&inputColorQF[STAMP_FRAGMENTS * s], &outColor[STAMP_FRAGMENTS * s * 4]);
                                bytesPerSample = 4;
                                break;

                            case GPU_R32F:

                                colorRGBA32FToR32F(&inputColorQF[STAMP_FRAGMENTS * s], &outColor[STAMP_FRAGMENTS * s * 4]);
                                bytesPerSample = 4;
                                break;

                            case GPU_RGBA16:

                                colorRGBA32FToRGBA16(&inputColorQF[STAMP_FRAGMENTS * s], &outColor[STAMP_FRAGMENTS * s * 8]);
                                bytesPerSample = 8;
                                break;

                            case GPU_RGBA16F:

                                colorRGBA32FToRGBA16F(&inputColorQF[STAMP_FRAGMENTS * s], &outColor[STAMP_FRAGMENTS * s * 8]);
                                bytesPerSample = 8;
                                break;
                        }
                    }
                }
            }

            GPU_DEBUG(
                CG_INFO("Color Data = ");
                for(U32 b = 0; b < (STAMP_FRAGMENTS * 4); b++)
                    CG_INFO("%02x ", colorData[b]);
                CG_INFO("");
                CG_INFO("Out Color = ");
                for(U32 b = 0; b < (STAMP_FRAGMENTS * 4); b++)
                    CG_INFO("%02x ", outColor[b]);
                CG_INFO("");
            )

            //  Check if multisampling is enabled.
            if (!state.multiSampling)
            {
                //  Masked write of the result color.
                for(U32 b = 0; b < STAMP_FRAGMENTS * bytesPerSample; b++)
                {
                    if (writeMask[b])
                        colorData[b] = outColor[b];
                }

                if (validationMode)
                {
                    //  Check if a write is performed (fragment not culled).
                    bool anyNotCulled = false;                       
                    for(U32 f = 0; (f < STAMP_FRAGMENTS) && !anyNotCulled; f++)
                        anyNotCulled = anyNotCulled || !quad[f]->isCulled();
                    
                    //  Store information for the quad and add to the z/stencil memory update map.
                    if (anyNotCulled)
                    {
                        //  Store the result z and stencil results for the quad.
                        for(U32 qw = 0; qw < ((bytesPerPixelColor[rt] * 4) >> 3) ; qw++)
                            ((U64 *) quadMemUpdate.writeData)[qw] = ((U64 *) outColor)[qw];
                            
                        //  Store the write mask for the quad.
                        for(U32 b = 0; b < (STAMP_FRAGMENTS * bytesPerPixelColor[rt]); b++)
                            quadMemUpdate.writeMask[b] = writeMask[b];
                            
                        //  Store the cull mask for the quad.
                        for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
                            quadMemUpdate.cullMask[f] = quad[f]->isCulled();
                            
                        //  Store the information for the quad in the z stencil memory update map.
                        colorMemoryUpdateMap[rt].insert(make_pair(quadMemUpdate.fragID, quadMemUpdate));
                    }
                }
            }
            else
            {
                //  Masked write of the result color.
                for(U32 b = 0; b < STAMP_FRAGMENTS * state.msaaSamples * bytesPerSample; b++)
                {
                    if (writeMask[b])
                        colorData[b] = outColor[b];
                }
            }

            GPU_DEBUG(
                CG_INFO("Color Data = ");
                for(U32 b = 0; b < (STAMP_FRAGMENTS * 4); b++)
                    CG_INFO("%02x ", colorData[b]);
                CG_INFO("");
            )
        }
    }
    
    GLOBAL_PROFILER_EXIT_REGION()
}

void bmoGpuTop::emulateZStencilTest(ShadedFragment **quad)
{
    //  Optimization.
    if (!state.depthTest && !state.stencilTest)
        return;

    GLOBAL_PROFILER_ENTER_REGION("emulateZStencilTest", "", "emulateZStencilTest")

    U32 inputDepth[STAMP_FRAGMENTS * MAX_MSAA_SAMPLES];
    bool sampleCullMask[STAMP_FRAGMENTS * MAX_MSAA_SAMPLES];
    bool writeMask[STAMP_FRAGMENTS * MAX_MSAA_SAMPLES * 4];
    bool culledFragments[STAMP_FRAGMENTS];
    U08 *zStencilData;
    U32 zStencilInOutData[STAMP_FRAGMENTS * MAX_MSAA_SAMPLES];


    U32 bytesPerSample = 4;

    //  Compute the address in memory for the fragment quad.
    U32 address = zPixelMapper.computeAddress(quad[0]->getFragment()->getX(), quad[0]->getFragment()->getY());

    //  Add back buffer base address to the address inside the color buffer.
    address += state.zStencilBufferBaseAddr;

    U08 *memory = selectMemorySpace(address);
    address = address & SPACE_ADDRESS_MASK;

    GPU_DEBUG(
        CG_INFO("ZStencilTest for quad at (%d, %d) at address %08x (base address %08x)",
        quad[0]->getFragment()->getX(), quad[0]->getFragment()->getY(),
        address, state.zStencilBufferBaseAddr);
    )

    zStencilData = &memory[address];

    //  Check if multisampling is enabled.
    if (!state.multiSampling)
    {
        //  Read z stencil data from the buffer.
        for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
            zStencilInOutData[f] = *((U32 *) &zStencilData[f * bytesPerSample]);
    }
    else
    {
        //  Read z stencil data from the buffer.
        for(U32 s = 0; s < STAMP_FRAGMENTS * state.msaaSamples; s++)
            zStencilInOutData[s] = *((U32 *) &zStencilData[s * bytesPerSample]);
    }

    GPU_DEBUG(
        CG_INFO("Z Stencil Buffer Data = ");
        for(U32 b = 0; b < (STAMP_FRAGMENTS * 4); b++)
            CG_INFO("%02x ", zStencilData[b]);
        CG_INFO("");
    )

    //  Get the input depth data from the fragments.
    for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
    {
        culledFragments[f] = quad[f]->isCulled();

        //  Get fragment attributes.
        Vec4FP32 *attributes = quad[f]->getAttributes();

        //  Get fragment.
        Fragment *fr = quad[f]->getFragment();

        //  Check if fragment depth was modified by the fragment shader.
        if (state.modifyDepth)
        {
            //  Check if multisampling is enabled.
            if (!state.multiSampling)
            {
                //  Convert modified fragment depth to integer format.
                inputDepth[f] = bmRaster->convertZ(GPU_CLAMP(attributes[POSITION_ATTRIBUTE][3], 0.0f, 1.0f));
            }
            else
            {
                //  Get pointer to the coverage mask for the fragment.
                bool *fragmentCoverage = fr->getMSAACoverage();

                //  Convert modified fragment depth to integer format.
                inputDepth[f * state.msaaSamples] = bmRaster->convertZ(GPU_CLAMP(attributes[POSITION_ATTRIBUTE][3], 0.0f, 1.0f));
                sampleCullMask[f * state.msaaSamples] = !fragmentCoverage[0];

                //  Copy Z sample values from the fragment z value computed in the shader.
                for(U32 s = 1; s < state.msaaSamples; s++)
                {
                    inputDepth[f * state.msaaSamples + s] = inputDepth[f * state.msaaSamples];
                    sampleCullMask[f * state.msaaSamples + s] = !fragmentCoverage[s];
                }
            }
        }
        else
        {
            //  Check if multisampling is enabled.
            if (!state.multiSampling)
            {
                //  Copy input z from the fragment.
                inputDepth[f] = fr->getZ();
            }
            else
            {
                //  Get pointer to the Z samples for the fragment.
                U32 *fragmentZSamples = fr->getMSAASamples();

                //  Get pointer to the coverage mask for the fragment.
                bool *fragmentCoverage = fr->getMSAACoverage();

                //  Copy Z sample values.
                for(U32 s = 0; s < state.msaaSamples; s++)
                {
                    inputDepth[f * state.msaaSamples + s] = fragmentZSamples[s];
                    sampleCullMask[f * state.msaaSamples + s] = !fragmentCoverage[s];
                }
            }
        }

        //  Check if multisampling is enabled.
        if (!state.multiSampling)
        {
            //  Build write mask for the stamp.
            writeMask[f * bytesPerSample + 3] = (state.stencilUpdateMask == 0) ? false : true;
            writeMask[f * bytesPerSample + 2] =
            writeMask[f * bytesPerSample + 1] =
            writeMask[f * bytesPerSample + 0] = state.depthMask;
        }
        else
        {
            //  Create write mask for all the samples in the stamp.
            for(U32 s = 0; s < state.msaaSamples; s++)
            {
                //  Build write mask for the stamp.
                writeMask[(f * state.msaaSamples + s) * bytesPerSample + 3] = (state.stencilUpdateMask == 0) ? false : true;
                writeMask[(f * state.msaaSamples + s) * bytesPerSample + 2] =
                writeMask[(f * state.msaaSamples + s) * bytesPerSample + 1] =
                writeMask[(f * state.msaaSamples + s) * bytesPerSample + 0] = state.depthMask;
            }
        }
    }

    GPU_DEBUG(
        CG_INFO("Input Depth = ");
        for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
            CG_INFO("%08x ", inputDepth[f]);
        CG_INFO("");
        CG_INFO("Culled = ");
        for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
            CG_INFO("%d ", culledFragments[f]);
        CG_INFO("");
        CG_INFO("Write Mask = ");
        for(U32 b = 0; b < (STAMP_FRAGMENTS * 4); b++)
            CG_INFO("%02x ", writeMask[b]);
        CG_INFO("");
    )

    FragmentQuadMemoryUpdateInfo quadMemUpdate;
    bool anyNotAlreadyCulled;
    
    //  Check if multisampling is enabled
    if (!state.multiSampling)
    {
        //  Check if validation mode is enabled.
        if (validationMode)
        {
            //  Check if there are unculled fragments in the quad.
            anyNotAlreadyCulled = false;
            for(U32 f = 0; (f < STAMP_FRAGMENTS) && !anyNotAlreadyCulled; f++)
                anyNotAlreadyCulled = anyNotAlreadyCulled || !culledFragments[f];

            //  Store the information for the quad if there is a valid fragment.
            if (anyNotAlreadyCulled)
            {
                //  Set the quad identifier to the top left fragment of the quad.
                quadMemUpdate.fragID.x = quad[0]->getFragment()->getX();
                quadMemUpdate.fragID.y = quad[0]->getFragment()->getY();
                quadMemUpdate.fragID.triangleID = triangleCounter;
                
                quadMemUpdate.fragID.sample = 0;
                
                //  Copy the input (z computed for fragment) and read data (z/stencil from the z stencil buffer) for
                //  the quad.
                for(U32 qw = 0; qw < ((bytesPerPixelDepth * 4) >> 3); qw++)
                {
                    ((U64 *) quadMemUpdate.inData)[qw] = ((U64 *) inputDepth)[qw];
                    ((U64 *) quadMemUpdate.readData)[qw] = ((U64 *) zStencilInOutData)[qw];
                }
            }
        }
        
        //  Perform Stencil and Z tests.
        bmFragOp->stencilZTest(inputDepth, zStencilInOutData, culledFragments);

        //  Update cull mask for the fragments.
        for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
        {
            if (culledFragments[f])
                quad[f]->setAsCulled();
        }

        //  Check if validation mode is enabled.
        if (validationMode)
        {
            //  Check if there are unculled fragments in the quad.
            anyNotAlreadyCulled = false;
            for(U32 f = 0; (f < STAMP_FRAGMENTS) && !anyNotAlreadyCulled; f++)
                anyNotAlreadyCulled = anyNotAlreadyCulled || !culledFragments[f];

            //  Store information for the quad and add to the z/stencil memory update map.
            if (anyNotAlreadyCulled)
            {
                //  Store the result z and stencil results for the quad.
                for(U32 qw = 0; qw < ((bytesPerPixelDepth * 4) >> 3); qw++)
                    ((U64 *) quadMemUpdate.writeData)[qw] = ((U64 *) zStencilInOutData)[qw];
                    
                //  Store the write mask for the quad after the write.
                for(U32 b = 0; b < (STAMP_FRAGMENTS * bytesPerPixelDepth); b++)
                    quadMemUpdate.writeMask[b] = writeMask[b];
                    
                //  Store the write mask for the quad after write.
                for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
                    quadMemUpdate.cullMask[f] = culledFragments[f];
                    
                //  Store the information for the quad in the z stencil memory update map.
                zstencilMemoryUpdateMap.insert(make_pair(quadMemUpdate.fragID, quadMemUpdate));
            }
        }
    }
    else
    {
        //  Create cull mask for the samples

        //  Perform Stencil and Z tests for all the stamps.
        for(U32 s = 0; s < state.msaaSamples; s++)
            bmFragOp->stencilZTest(&inputDepth[s * STAMP_FRAGMENTS],
                                  &zStencilInOutData[s * STAMP_FRAGMENTS],
                                  &sampleCullMask[s * STAMP_FRAGMENTS]);

        //  Update coverage mask for all the fragments in the stamp.
        for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
        {
            //  Get pointer to the coverage mask for the fragment.
            bool *fragmentCoverage = quad[f]->getFragment()->getMSAACoverage();

            bool allSamplesCulled = true;

            //  Update coverage mask for the sample.
            for(U32 s = 0; s < state.msaaSamples; s++)
            {
                fragmentCoverage[s] = !sampleCullMask[f * state.msaaSamples + s];

                //  Check if all the samples in the fragment were culled.
                allSamplesCulled = allSamplesCulled && sampleCullMask[f * state.msaaSamples + s];
            }

            //  Set cull mask for the fragment.
            if (allSamplesCulled)
                quad[f]->setAsCulled();
        }
    }

    GPU_DEBUG(
        CG_INFO("Result Z Stencil Data = ");
        for(U32 b = 0; b < STAMP_FRAGMENTS; b++)
            CG_INFO("%08x ", zStencilInOutData[b]);
        CG_INFO("");
        CG_INFO("Culled = ");
        for(U32 f = 0; f < STAMP_FRAGMENTS; f++)
            CG_INFO("%d ", culledFragments[f]);
        CG_INFO("");
    )

    //  Check if multisampling is enabled.
    if (!state.multiSampling)
    {
        //  Masked write of the result color.
        for(U32 b = 0; b < STAMP_FRAGMENTS * bytesPerSample; b++)
        {
            if (writeMask[b])
                zStencilData[b] = ((U08 *) zStencilInOutData)[b];
        }
    }
    else
    {
        //  Masked write of the result color.
        for(U32 b = 0; b < STAMP_FRAGMENTS * state.msaaSamples * bytesPerSample; b++)
        {
            if (writeMask[b])
                zStencilData[b] = ((U08 *) zStencilInOutData)[b];
        }
    }

    GPU_DEBUG(
        CG_INFO("Final Z Stencil Data = ");
        for(U32 b = 0; b < (STAMP_FRAGMENTS * 4); b++)
            CG_INFO("%02x ", zStencilData[b]);
        CG_INFO("--------");
    )

    GLOBAL_PROFILER_EXIT_REGION()
}

void bmoGpuTop::cleanup()
{
    indexList.clear();

    map<U32, ShadedVertex *>::iterator it = vertexList.begin();

    while (it != vertexList.end())
    {
        delete it->second;
        it++;
    }

    vertexList.clear();
}

void bmoGpuTop::emulateBlitter()
{
    //if (state.colorBufferFormat != GPU_RGBA8888)
    if (state.rtFormat[0] != GPU_RGBA8888)
    {
        CG_ASSERT("Blitter only supports RGBA8 color buffer.");
    }
    //  Set display parameters in the Pixel Mapper.
    U32 samples = state.multiSampling ? state.msaaSamples : 1;
    U32 bytesPixel;

    switch(state.rtFormat[0])
    {
        case GPU_RGBA8888:
        case GPU_RG16F:
        case GPU_R32F:
            bytesPixel = 4;
            break;
        case GPU_RGBA16:
        case GPU_RGBA16F:
            bytesPixel = 8;
            break;
    }

    pixelMapper[0].setupDisplay(state.displayResX, state.displayResY, STAMP_WIDTH, STAMP_WIDTH,
                             ArchConf.ras.genWidth / STAMP_WIDTH, ArchConf.ras.genHeight / STAMP_HEIGHT,
                             ArchConf.ras.scanWidth / ArchConf.ras.genWidth, ArchConf.ras.scanHeight / ArchConf.ras.genHeight,
                             ArchConf.ras.overScanWidth, ArchConf.ras.overScanHeight,
                             samples, bytesPixel);

    //U08 *sourceMemory = selectMemorySpace(state.backBufferBaseAddr);
    U08 *sourceMemory = selectMemorySpace(state.rtAddress[0]);
    U08 *destMemory = selectMemorySpace(state.blitDestinationAddress);

    //  Initialize blitter pixel mapper if required.
    if (state.blitDestinationTextureBlocking == GPU_TXBLOCK_FRAMEBUFFER)
    {
        // Setup display in the Pixel Mapper.
        //U32 samples = state.multiSampling ? state.msaaSamples : 1;
        blitPixelMapper.setupDisplay(state.blitWidth, state.blitHeight, STAMP_WIDTH, STAMP_WIDTH,
                                     ArchConf.ras.genWidth / STAMP_WIDTH, ArchConf.ras.genHeight / STAMP_HEIGHT,
                                     ArchConf.ras.scanWidth / ArchConf.ras.genWidth, ArchConf.ras.scanHeight / ArchConf.ras.genHeight,
                                     ArchConf.ras.overScanWidth, ArchConf.ras.overScanHeight, 1, 4);
    }

    //  Compute the bytes per texel.
    U32 bytesTexel;
    switch(state.blitDestinationTextureFormat)
    {
        case GPU_RGBA8888:

            bytesTexel = 4;
            break;

        default :

            CG_ASSERT("Unsupported blit format.");
            break;
    }

    //  Copy all pixel/texels to the destination buffer.
    for(U32 y = 0; y < state.blitHeight; y++)
    {
        U32 yPix = y + state.blitIniY;
        U32 yTex = y + state.blitYOffset;

        for(U32 x = 0; x < state.blitWidth; x++)
        {
            U32 xPix = x + state.blitIniX;
            U32 xTex = x + state.blitXOffset;

            //  Compute the address of the texel inside the texture.
            U32 texelAddress;
            switch(state.blitDestinationTextureBlocking)
            {
                case GPU_TXBLOCK_TEXTURE:

                    // Compute the morton address for the texel.
                    texelAddress = texel2MortonAddress(xTex, yTex, ArchConf.ush.texBlockDim, ArchConf.ush.texSuperBlockDim, state.blitTextureWidth2) * bytesTexel;
                    break;

                case GPU_TXBLOCK_FRAMEBUFFER:

                    // Compute the framebuffer tiling/blocking address for the first corner texel of the block.
                    texelAddress = blitPixelMapper.computeAddress(xTex, yTex);
                    break;

                default:

                    CG_ASSERT("Unsupported blocking mode.");
                    break;
            }

            //  Compute the address of the pixel inside the color buffer.
            U32 pixelAddress = pixelMapper[0].computeAddress(xPix, yPix);

            //pixelAddress += state.backBufferBaseAddr;
            pixelAddress += (state.rtAddress[0] & SPACE_ADDRESS_MASK);
            texelAddress += (state.blitDestinationAddress & SPACE_ADDRESS_MASK);

            //  Copy pixel data to texel data (only for RGBA8).
            *((U32 *) &destMemory[texelAddress]) = *((U32 *) &sourceMemory[pixelAddress]);
        }
    }
}

U32 bmoGpuTop::texel2MortonAddress(U32 i, U32 j, U32 blockDim, U32 sBlockDim, U32 width)
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

void bmoGpuTop::printTextureAccessInfo(TextureAccess *texAccess)
{
    CG_INFO("Texture Trace");
    CG_INFO("-------------");
    CG_INFO("        Texture Unit = %d", texAccess->textUnit);
    CG_INFO("          Width = %d", state.textureWidth[texAccess->textUnit]);
    CG_INFO("          Height = %d", state.textureHeight[texAccess->textUnit]);
    if (state.textureMode[texAccess->textUnit] == GPU_TEXTURE3D)
        CG_INFO("          Depth = %d", state.textureDepth[texAccess->textUnit]);
    CG_INFO("          Format = %d", state.textureFormat[texAccess->textUnit]);
    CG_INFO("          Type = ");
    switch(state.textureMode[texAccess->textUnit])
    {
        case GPU_TEXTURE1D:
            CG_INFO("1D");
            break;
        case GPU_TEXTURE2D:
            CG_INFO("2D");
            break;
        case GPU_TEXTURECUBEMAP:
            CG_INFO("Cube");
            break;
        case GPU_TEXTURE3D:
            CG_INFO("3D");
            break;
        default:
            CG_INFO("Unknown");
            break;
    }
    CG_INFO("          Blocking = ");
    switch(state.textureBlocking[texAccess->textUnit])
    {
        case GPU_TXBLOCK_TEXTURE:
            CG_INFO("TEXTURE");
            break;
        case GPU_TXBLOCK_FRAMEBUFFER:
            CG_INFO("FRAMEBUFFER");
            break;
        default:
            CG_INFO("Unknown");
    }
    for(U32 p = 0; p < STAMP_FRAGMENTS; p++)
    {
        CG_INFO(" Pixel %d", p);
        CG_INFO("------------");
        CG_INFO("         LOD = %f", texAccess->lod[p]);
        CG_INFO("         LOD0 = %d", texAccess->level[p][0]);
        CG_INFO("         LOD1 = %d", texAccess->level[p][1]);
        F32 w = texAccess->lod[p] - static_cast<F32>(GPU_FLOOR(texAccess->lod[p]));
        CG_INFO("         WeightLOD = %f", w);
        CG_INFO("         Samples = %d", texAccess->anisoSamples);
        if (state.textureMode[texAccess->textUnit] == GPU_TEXTURECUBEMAP)
        {
            CG_INFO("Pixel %d Coord (orig) = (%f, %f, %f, %f)", p,
                texAccess->originalCoord[p][0], texAccess->originalCoord[p][1],
                texAccess->originalCoord[p][2], texAccess->originalCoord[p][3]);
            CG_INFO("Pixel %d Coord (post cube) = (%f, %f, %f, %f)", p,
                texAccess->coordinates[p][0], texAccess->coordinates[p][1],
                texAccess->coordinates[p][2], texAccess->coordinates[p][3]);
            CG_INFO("Face = %d (", texAccess->cubemap);
            switch(texAccess->cubemap)
            {
                case GPU_CUBEMAP_NEGATIVE_X:
                    CG_INFO("-X)");
                    break;
                case GPU_CUBEMAP_POSITIVE_X:
                    CG_INFO("+X)");
                    break;
                case GPU_CUBEMAP_NEGATIVE_Y:
                    CG_INFO("-Y)");
                    break;
                case GPU_CUBEMAP_POSITIVE_Y:
                    CG_INFO("+Y)");
                    break;
                case GPU_CUBEMAP_NEGATIVE_Z:
                    CG_INFO("-Z)");
                    break;
                case GPU_CUBEMAP_POSITIVE_Z:
                    CG_INFO("+Z)");
                    break;
                default:
                    CG_INFO("Unknown face)");
                    break;
            }
        }
        else
        {
            CG_INFO("Pixel %d Coord = (%f, %f, %f, %f)", p,
                texAccess->coordinates[p][0], texAccess->coordinates[p][1],
                texAccess->coordinates[p][2], texAccess->coordinates[p][3]);
        }
     
        for(U32 s = 0; s < texAccess->anisoSamples; s++)
        {
            CG_INFO("  Sample %d", s);
            CG_INFO("-----------------");
            CG_INFO("         Sample from two mips = %s",
                texAccess->trilinear[s]->sampleFromTwoMips[p] ? "YES":"NO");
            CG_INFO("         LOD0");
            CG_INFO("          WeightA = %f", texAccess->trilinear[s]->a[p][0]);
            CG_INFO("          WeightB = %f", texAccess->trilinear[s]->b[p][0]);
            CG_INFO("          WeightC = %f", texAccess->trilinear[s]->c[p][0]);
            CG_INFO("         LOD1");
            CG_INFO("          WeightA = %f", texAccess->trilinear[s]->a[p][1]);
            CG_INFO("          WeightB = %f", texAccess->trilinear[s]->b[p][1]);
            CG_INFO("          WeightC = %f", texAccess->trilinear[s]->c[p][1]);
            CG_INFO("         Loops = %d", texAccess->trilinear[s]->loops[p]);
            for(U32 b = 0; b < texAccess->trilinear[s]->loops[p]; b++)
            {
                U32 texelsLoop = texAccess->trilinear[s]->texelsLoop[p];
                CG_INFO("  Loop %d", b);
                CG_INFO("-------------");
                CG_INFO("         Texels/Loop = %d", texelsLoop);
                for(U32 t = 0; t < texelsLoop; t++)
                {
                    CG_INFO("         Texel = (%d, %d)",
                    texAccess->trilinear[s]->i[p][b * texelsLoop + t],
                    texAccess->trilinear[s]->j[p][b * texelsLoop + t]);
                    if (state.textureMode[texAccess->textUnit] == GPU_TEXTURE3D)
                        CG_INFO("         Slice = %d", texAccess->trilinear[s]->k[p][b * texelsLoop + t]);
                    CG_INFO("         Address = %08x",
                        texAccess->trilinear[s]->address[p][b * texelsLoop + t]);
                    CG_INFO("         Value = {%f, %f, %f, %f} [%02x, %02x, %02x, %02x]",
                        texAccess->trilinear[s]->texel[p][b * texelsLoop + t][0],
                        texAccess->trilinear[s]->texel[p][b * texelsLoop + t][1],
                        texAccess->trilinear[s]->texel[p][b * texelsLoop + t][2],
                        texAccess->trilinear[s]->texel[p][b * texelsLoop + t][3],
                        U32(255.0f * texAccess->trilinear[s]->texel[p][b * texelsLoop + t][0]),
                        U32(255.0f * texAccess->trilinear[s]->texel[p][b * texelsLoop + t][1]),
                        U32(255.0f * texAccess->trilinear[s]->texel[p][b * texelsLoop + t][2]),
                        U32(255.0f * texAccess->trilinear[s]->texel[p][b * texelsLoop + t][3]));
                }
            }
        }
        CG_INFO("         Result = {%f, %f, %f, %f}",
            texAccess->sample[p][0], texAccess->sample[p][1],
            texAccess->sample[p][2], texAccess->sample[p][3]);
    }

    CG_INFO("-------------");
}

void bmoGpuTop::printShaderInstructionOperands(cgoShaderInstr::cgoShaderInstrEncoding *shDecInstr)
{
    if (shDecInstr->getShaderInstruction()->getNumOperands() > 0)
    {
        switch(shDecInstr->getShaderInstruction()->getBankOp1())
        {
            case cg1gpu::TEXT:            
                break;
          
            case cg1gpu::PRED:
                {
                    bool *op1 = (bool *) shDecInstr->getShEmulOp1();
                    CG_INFO("             OP1 -> %s", *op1 ? "true" : "false");
                }
                break;

            default:
            
                //  Check for predicator operator instruction with constant register operator.
                switch(shDecInstr->getShaderInstruction()->getOpcode())
                {
                    case cg1gpu::CG1_ISA_OPCODE_ANDP:
                        {
                            bool *op1 = (bool *) shDecInstr->getShEmulOp1();
                            CG_INFO("             OP1 -> {%s, %s, %s, %s}", op1[0] ? "true" : "false", op1[1] ? "true" : "false",
                                op1[2] ? "true" : "false", op1[3] ? "true" : "false");
                        }
                        break;
                      
                    case cg1gpu::CG1_ISA_OPCODE_ADDI:
                    case cg1gpu::CG1_ISA_OPCODE_MULI:
                    case cg1gpu::CG1_ISA_OPCODE_STPEQI:
                    case cg1gpu::CG1_ISA_OPCODE_STPGTI:
                    case cg1gpu::CG1_ISA_OPCODE_STPLTI:
                        {
                            S32 *op1 = (S32 *) shDecInstr->getShEmulOp1();
                            CG_INFO("             OP1 -> {%d, %d, %d, %d}", op1[0], op1[1], op1[2], op1[3]);
                        }                        
                        break;
                          
                    default:
                        {
                            F32 *op1 = (F32 *) shDecInstr->getShEmulOp1();
                            CG_INFO("             OP1 -> {%f, %f, %f, %f}", op1[0], op1[1], op1[2], op1[3]);
                        }                        
                        break;
                }
                break;                
        }
    }
    
    if (shDecInstr->getShaderInstruction()->getNumOperands() > 1)
    {
        switch(shDecInstr->getShaderInstruction()->getBankOp2())
        {
            case cg1gpu::TEXT:            
                break;
          
            case cg1gpu::PRED:
                {
                    bool *op2 = (bool *) shDecInstr->getShEmulOp2();
                    CG_INFO("             OP2 -> %s", *op2 ? "true" : "false");
                }
                break;
                
            default:
            
                //  Check for predicator operator instruction with constant register operator.
                switch(shDecInstr->getShaderInstruction()->getOpcode())
                {
                    case cg1gpu::CG1_ISA_OPCODE_ANDP:
                        {
                            bool *op2 = (bool *) shDecInstr->getShEmulOp2();
                            CG_INFO("             OP2 -> {%s, %s, %s, %s}", op2[0] ? "true" : "false", op2[1] ? "true" : "false",
                                op2[2] ? "true" : "false", op2[3] ? "true" : "false");
                        }
                        break;
                      
                    case cg1gpu::CG1_ISA_OPCODE_ADDI:
                    case cg1gpu::CG1_ISA_OPCODE_MULI:
                    case cg1gpu::CG1_ISA_OPCODE_STPEQI:
                    case cg1gpu::CG1_ISA_OPCODE_STPGTI:
                    case cg1gpu::CG1_ISA_OPCODE_STPLTI:
                        {
                            S32 *op2 = (S32 *) shDecInstr->getShEmulOp2();
                            CG_INFO("             OP2 -> {%d, %d, %d, %d}", op2[0], op2[1], op2[2], op2[3]);
                        }                        
                        break;
                          
                    default:
                        {
                            F32 *op2 = (F32 *) shDecInstr->getShEmulOp2();
                            CG_INFO("             OP2 -> {%f, %f, %f, %f}", op2[0], op2[1], op2[2], op2[3]);
                        }                        
                        break;
                }
                
                break;
        }
    }
    
    if (shDecInstr->getShaderInstruction()->getNumOperands() > 2)
    {
        switch(shDecInstr->getShaderInstruction()->getBankOp3())
        {
            case cg1gpu::TEXT:            
                break;
          
            case cg1gpu::PRED:
                {
                    bool *op3 = (bool *) shDecInstr->getShEmulOp3();
                    CG_INFO("             OP3 -> %s", *op3 ? "true" : "false");
                }
                break;
                
            default:
                //  Check for predicator operator instruction with constant register operator.
                switch(shDecInstr->getShaderInstruction()->getOpcode())
                {
                    case cg1gpu::CG1_ISA_OPCODE_ANDP:
                        {
                            bool *op3 = (bool *) shDecInstr->getShEmulOp3();
                            CG_INFO("             OP3 -> {%s, %s, %s, %s}", op3[0] ? "true" : "false", op3[1] ? "true" : "false",
                                op3[2] ? "true" : "false", op3[3] ? "true" : "false");
                        }
                        break;
                      
                    case cg1gpu::CG1_ISA_OPCODE_ADDI:
                    case cg1gpu::CG1_ISA_OPCODE_MULI:
                    case cg1gpu::CG1_ISA_OPCODE_STPEQI:
                    case cg1gpu::CG1_ISA_OPCODE_STPGTI:
                    case cg1gpu::CG1_ISA_OPCODE_STPLTI:
                        {
                            S32 *op3 = (S32 *) shDecInstr->getShEmulOp3();
                            CG_INFO("             OP3 -> {%d, %d, %d, %d}", op3[0], op3[1], op3[2], op3[3]);
                        }                        
                        break;
                          
                    default:
                        {
                            F32 *op3 = (F32 *) shDecInstr->getShEmulOp3();
                            CG_INFO("             OP3 -> {%f, %f, %f, %f}", op3[0], op3[1], op3[2], op3[3]);
                        }                        
                        break;
                }
                
                break;
        }
    }

    if (shDecInstr->getShaderInstruction()->getPredicatedFlag())
    {
        bool *pred = (bool *) shDecInstr->getShEmulPredicate();
        CG_INFO("             PREDICATE -> %s", *pred ? "true" : "false");
    }
}

void bmoGpuTop::printShaderInstructionResult(cgoShaderInstr::cgoShaderInstrEncoding *shDecInstr)
{
    if (shDecInstr->getShaderInstruction()->hasResult())
    {
        switch(shDecInstr->getShaderInstruction()->getBankRes())
        {
            case PRED:
                {
                    bool *res = (bool *) shDecInstr->getShEmulResult();                
                    CG_INFO("             RESULT -> %s", *res ? "true" : "false");
                }                
                break;

            default:
            
                switch(shDecInstr->getShaderInstruction()->getOpcode())
                {
                    case CG1_ISA_OPCODE_ADDI:
                    case CG1_ISA_OPCODE_MULI:
                        {
                            S32 *res = (S32 *) shDecInstr->getShEmulResult();                
                            CG_INFO("             RESULT -> {%d, %d, %d, %d}", res[0], res[1], res[2], res[3]);
                        }
                        break;
                        
                    default:
                        {
                            F32 *res = (F32 *) shDecInstr->getShEmulResult();                
                            CG_INFO("             RESULT -> {%f, %f, %f, %f}", res[0], res[1], res[2], res[3]);
                        }
                        break;
                }
                break;
        }
    }
}

//void bmoGpuTop::simulationLoop()
//{
//    GLOBAL_PROFILER_ENTER_REGION("simulationLoop", "", "")
//    traceEnd = false;
//    TraceDriver->startTrace(); //  Start the trace driver.
//    resetState();
//    while(!traceEnd && (frameCounter < (ArchConf.sim.startFrame + ArchConf.sim.simFrames)) && !AbortSim)
//    {
//        cgoMetaStream *CurMetaStream;
//        GLOBAL_PROFILER_ENTER_REGION("driver", "", "")
//        CurMetaStream = TraceDriver->nxtMetaStream(); //  Get next transaction from the driver
//        GLOBAL_PROFILER_EXIT_REGION()
//        if (CurMetaStream != NULL)
//        {
//            emulateCommandProcessor(CurMetaStream);
//        }
//        else
//        {
//            traceEnd = true;
//            CG_INFO("End of trace.");
//        }
//    }
//    CG_INFO_COND(AbortSim, "Simulation aborted");
//    GLOBAL_PROFILER_EXIT_REGION()
//}

//void bmoGpuTop::abortSimulation()
//{
//    AbortSim = true;
//}

U32 bmoGpuTop::getFrameCounter()
{
    return frameCounter;
}

U32 bmoGpuTop::getBatchCounter()
{
    return batchCounter;
}

U32 bmoGpuTop::getTriangleCounter()
{
    return triangleCounter;
}

void bmoGpuTop::getCounters(U32 &frame, U32 &batch, U32 &triangle)
{
    frame = frameCounter;
    batch = batchCounter;
    triangle = triangleCounter;
}

void bmoGpuTop::setValidationMode(bool enable)
{
    validationMode = enable;
}

ShadedVertexMap &bmoGpuTop::getShadedVertexLog()
{
    return shadedVertexLog;
}

VertexInputMap &bmoGpuTop::getVertexInputLog()
{
    return vertexInputLog;
}

void bmoGpuTop::setFullTraceLog(bool enable)
{
    traceLog = enable;
}

void bmoGpuTop::setBatchTraceLog(bool enable, U32 batch)
{
    traceBatch = enable;
    watchBatch = batch;
}

void bmoGpuTop::setVertexTraceLog(bool enable, U32 index)
{
    traceVertex = enable;
    watchIndex = index;
}

void bmoGpuTop::setPixelTraceLog(bool enable, U32 x, U32 y)
{
    tracePixel = enable;
    watchPixelX = x;
    watchPixelY = y;
}

FragmentQuadMemoryUpdateMap &bmoGpuTop::getZStencilUpdateMap()
{
    return zstencilMemoryUpdateMap;
}

FragmentQuadMemoryUpdateMap &bmoGpuTop::getColorUpdateMap(U32 rt)
{
    CG_ASSERT_COND(!(rt > MAX_RENDER_TARGETS), "Render target identifier out of range.");    
    return colorMemoryUpdateMap[rt];
}

TextureFormat bmoGpuTop::getRenderTargetFormat(U32 rt)
{
    CG_ASSERT_COND(!(rt > MAX_RENDER_TARGETS), "Render target identifier out of range.");
    return state.rtFormat[rt];
}


void bmoGpuTop::setSkipBatch(bool enable)
{
    skipBatchMode = enable;
}


//
//
//  TODO:
//
//   - bypass attributes in StreamController stage
//   - two sided lighting at setup
//


}  // namespace cg1gpu
