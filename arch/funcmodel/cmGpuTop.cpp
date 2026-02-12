/**************************************************************************
 *
 * GPU simulator implementation file.
 * This file contains the implementation of functions for the CG1 GPU simulator.
 */

#include "cmGpuTop.h"
#include "CommandLineReader.h"
#include "TraceDriverMeta.h"
#include "ColorCompressor.h"
#include "DepthCompressor.h"
#include "cmStatisticsManager.h"
#include "support.h"
#include <ctime>

using namespace std;

namespace arch
{

//  Constructor.
cmoGpuTop::cmoGpuTop(cgsArchConfig ArchConf, cgoTraceDriverBase *TraceDriver) :
    ArchConf(ArchConf),
    TraceDriver(TraceDriver)
{
    char **vshPrefix;
    char **fshPrefix;
    char **suPrefix;
    char **tuPrefixes;
    char tuPrefix[256];
    char mduName[128];
    char **slPrefixes;

    bmoColorCompressor::configureCompressor(ArchConf.cwr.comprAlgo);    // Configure compressors.
    bmoDepthCompressor::configureCompressor(ArchConf.zst.comprAlgo);

    // Notify boxes when the specific signal trace code is required.
    // Allows optimization in boxes with several inner signals only used to
    // show STV information
    cmoMduBase::setSignalTracing(ArchConf.sim.dumpSignalTrace, ArchConf.sim.startDump, ArchConf.sim.dumpCycles);

    //  Allocate pointer array for StreamController loader prefixes.
    slPrefixes = new char*[ArchConf.str.streamerLoaderUnits];
    CG_ASSERT_COND((slPrefixes != NULL), "Error allocating StreamController loader prefix array."); //  Check allocation.
    //  Create prefixes for the StreamController loader units.
    for(U32 i = 0; i < ArchConf.str.streamerLoaderUnits; i++)
    {
        slPrefixes[i] = new char[6]; //  Allocate prefix.
        CG_ASSERT_COND((slPrefixes[i] != NULL),"Error allocating StreamController loader prefix.");
        sprintf(slPrefixes[i], "SL%d", i); //  Write prefix.
    }

    //  Allocate pointer array for the vertex shader prefixes.
    vshPrefix = new char*[(ArchConf.gpu.numFShaders > ArchConf.gpu.numVShaders) ? ArchConf.gpu.numFShaders : ArchConf.gpu.numVShaders];
    CG_ASSERT_COND((vshPrefix != NULL), "Error allocating vertex shader prefix array.");//  Check allocation.
    U32 numVShaders = (ArchConf.gpu.numFShaders > ArchConf.gpu.numVShaders) ? ArchConf.gpu.numFShaders : ArchConf.gpu.numVShaders;
    //  Create prefixes for the virtual vertex shaders.
    for(U32 i = 0; i < numVShaders; i++)
    {
        vshPrefix[i] = new char[6]; //  Allocate prefix.
        CG_ASSERT_COND((vshPrefix[i] != NULL), "Error allocating vertex shader prefix.");
        sprintf(vshPrefix[i], "VS%d", i); //  Write prefix.
    }

    //  Allocate pointer array for the fragment shader prefixes.
    fshPrefix = new char*[ArchConf.gpu.numFShaders];
    CG_ASSERT_COND((fshPrefix != NULL), "Error allocating fragment shader prefix array.");//  Check allocation.
    //  Create prefixes for the fragment shaders.
    for(U32 i = 0; i < ArchConf.gpu.numFShaders; i++)
    {
        fshPrefix[i] = new char[6]; //  Allocate prefix.
        CG_ASSERT_COND((fshPrefix[i] != NULL), "Error allocating fragment shader prefix.");
        sprintf(fshPrefix[i], "FS%d", i); //  Write prefix.
    }

    //  Allocate pointer array for the texture unit prefixes.
    tuPrefixes = new char*[ArchConf.gpu.numFShaders * ArchConf.ush.textureUnits];
    CG_ASSERT_COND((tuPrefixes != NULL), "Error allocating texture unit prefix array."); //  Check allocation.
    //  Create prefixes for the fragment shaders.
    for(U32 i = 0; i < ArchConf.gpu.numFShaders; i++)
    {
        for(U32 j = 0; j < ArchConf.ush.textureUnits; j++)
        {
            tuPrefixes[i * ArchConf.ush.textureUnits + j] = new char[10]; //  Allocate prefix.
            CG_ASSERT_COND((tuPrefixes[i * ArchConf.ush.textureUnits + j] != NULL), "Error allocating texture unit prefix.");
            sprintf(tuPrefixes[i * ArchConf.ush.textureUnits + j], "FS%dTU%d", i, j); //  Write prefix.
        }
    }

    //  Allocate pointer array for the stamp unit prefixes.
    suPrefix = new char*[ArchConf.gpu.numStampUnits];
    CG_ASSERT_COND((suPrefix != NULL),"Error allocating stamp unit prefix array.");//  Check allocation.
    //  Create prefixes for the stamp units.
    for(U32 i = 0; i < ArchConf.gpu.numStampUnits; i++)
    {
        suPrefix[i] = new char[6]; //  Allocate prefix.
        CG_ASSERT_COND((suPrefix[i] != NULL), "Error allocating stamp unit prefix.");
        sprintf(suPrefix[i], "SU%d", i); //  Write prefix.
    }

    MduArray.clear(); //  Clear the mdu vector array.

    //  Determine clock operation mode.
    shaderClockDomain = (ArchConf.gpu.gpuClock != ArchConf.gpu.shaderClock);
    memoryClockDomain = (ArchConf.gpu.gpuClock != ArchConf.gpu.memoryClock);
    multiClock = shaderClockDomain || memoryClockDomain;
    
    //  In multi clock domain mode use multiple arrays.
    if (multiClock)
    {
        GpuDomainMduArray.clear();
        ShaderDomainMduArray.clear();
        MemoryDomainMduArray.clear();
    }
    CG_ASSERT_COND(!(shaderClockDomain && !ArchConf.ush.useVectorShader), "Shader clock domain only supported by Vector Shader model.");
    CG_ASSERT_COND(!(memoryClockDomain && !ArchConf.mem.memoryControllerV2), "Memory clock domain only supported with MemoryControllerV2.");

    CP = new cmoCommandProcessor( &ArchConf, TraceDriver, "CommandProc");
    MduArray.push_back(CP);//  Add Command Processor mdu to the mdu array.
    if (multiClock)
        GpuDomainMduArray.push_back(CP);

    // Select a memory controller based on simulator parameters
    MC = createMemoryController(ArchConf, (const char**) tuPrefixes, (const char**) suPrefix, (const char**) slPrefixes, "MemoryController", 0);
    MduArray.push_back(MC); //  Add Memory Controller mdu to the mdu array.
    if (multiClock && !memoryClockDomain)
        GpuDomainMduArray.push_back(MC);
    else if (multiClock && memoryClockDomain)
        MemoryDomainMduArray.push_back((cmoMduMultiClk *) MC);        
        

    //  NOTE:  INPUT CACHE IS FULLY ASSOCIATIVE AND MUST HAVE AT
    //  LEAST AS MANY LINES AS INPUT ATTRIBUTES OR IT WILL BECOME
    //  DEADLOCKED WHILE FETCHING ATTRIBUTES (CONFLICTS!!).

    //  Create and initialize the StreamController unit.  
    StreamController = new cmoStreamController(
        ArchConf.str.indicesCycle,        //  Indices processed per cycle.
        ArchConf.str.idxBufferSize,       //  Index buffer size in bytes.
        ArchConf.str.outputFIFOSize,      //  Output FIFO size.
        ArchConf.str.outputMemorySize,    //  Output memory size.
        ArchConf.gpu.numVShaders,         //  Number of attached vertex shaders.
        ArchConf.ush.outputLatency,       //  Maximum latency of the shader output signal.
        ArchConf.str.verticesCycle,       //  Vertices sent to Primitive Assembly per cycle.
        ArchConf.str.attrSentCycle,       //  Vertex attributes sent per cycle to Primitive Assembly.
        ArchConf.pas.inputBusLat,         //  Latency of the vertex bus with Primitive Assembly.
        ArchConf.str.streamerLoaderUnits, //  Number of cmoStreamController Loader units.
        ArchConf.str.slIndicesCycle,      //  Indices per cycle processed per cmoStreamController Loader unit.
        ArchConf.str.slInputReqQueueSize, //  Input request queue size (cmoStreamController Loader unit).
        ArchConf.str.slAttributesCycle,   //  Shader input attributes filled per cycle.
        ArchConf.str.slInCacheLines,      //  Input cache lines.
        ArchConf.str.slInCacheLineSz,     //  Input cache line size.
        ArchConf.str.slInCachePortWidth,  //  Input cache port width in bytes.
        ArchConf.str.slInCacheReqQSz,     //  Input cache request queue size.
        ArchConf.str.slInCacheInputQSz,   //  Input cache read memory request queue size.
        ArchConf.ush.vAttrLoadFromShader, //  Force attribute load bypass.  Vertex attribute load is performed in the shader.
        slPrefixes,                   //  cmoStreamController Loader prefixes.
        vshPrefix,                    //  Vertex Shader Prefixes.
        "cmoStreamController",
        NULL);

    //  Add cmoStreamController mdu to the mdu array.
    MduArray.push_back(StreamController);
    if (multiClock)
        GpuDomainMduArray.push_back(StreamController);

    //  Create and initialize the Primitive Assembly unit.
    PrimAssembler = new cmoPrimitiveAssembler( ArchConf.pas.verticesCycle,     //  Vertices per cycle received from the cmoStreamController.
                                            ArchConf.pas.trianglesCycle,    //  Triangles per cycle sent to the cmoClipper.
                                            ArchConf.str.attrSentCycle,     //  Number of vertex attributes received per cycle from cmoStreamController.
                                            ArchConf.pas.inputBusLat,       //  Latency of the vertex bus from the cmoStreamController.
                                            ArchConf.pas.paQueueSize,       //  Number of vertices in the assembly queue.
                                            ArchConf.clp.startLatency,      //  Start latency of the clipper test units.
                                            "cmoPrimitiveAssembler", NULL);
    //  Add Primitive Assembly mdu to the mdu array.
    MduArray.push_back(PrimAssembler);
    if (multiClock)
        GpuDomainMduArray.push_back(PrimAssembler);        

    //  Create and initialize the cmoClipper.
    clipper = new cmoClipper( ArchConf.clp.trianglesCycle,    //  Triangles received from Primitive Assembly per cycle.
                            ArchConf.clp.clipperUnits,      //  Number of clipper test units.
                            ArchConf.clp.startLatency,      //  Start latency for triangle clipping.
                            ArchConf.clp.execLatency,       //  Triangle clipping latency.
                            ArchConf.clp.clipBufferSize,    //  Clipped triangle buffer size.
                            ArchConf.ras.setupStartLat,     //  Start latency of the triangle setup units.
                            ArchConf.ras.trInputLat,        //  Latency of the triangle bus with Triangle Setup.
                            "cmoClipper", NULL);

    //  Add cmoClipper mdu to the mdu array.
    MduArray.push_back(clipper);
    if (multiClock)
        GpuDomainMduArray.push_back(clipper);        

    //  Create and initialize a rasterizer behaviorModel for cmoRasterizer.
    bmRaster = new bmoRasterizer( ArchConf.ras.emuStoredTriangles,    //  Active triangles.
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
    
    U32 threadGroup;  //  Set to the number of shader elements processed together and that must
                      //  also be issued or received to/from the shader processors as a group.
                         
    //  Check if vector shader model has been enabled.
    threadGroup = (ArchConf.ush.useVectorShader) ? ArchConf.ush.vectorLength : ArchConf.ush.threadGroup;
    //  Check if the micropolygon cmRasterizer.has been selected.
    if (ArchConf.ras.useMicroPolRast)
    {
        CG_ASSERT("Micropolygon rasterizer not implemented.");
    }
    else
    {
        CG_INFO("Using the Classic cmoRasterizer Unit.");
        //  Create and initialize the classic rasterizer unit.
        Raster = new cmoRasterizer( *bmRaster,                       //  cmoRasterizer behaviorModel.
                                ArchConf.ras.trianglesCycle,        //  Number of triangles received from PA/CLP per cycle.
                                ArchConf.ras.setupFIFOSize,         //  Triangle Setup FIFO.
                                ArchConf.ras.setupUnits,            //  Number of triangle setup units.
                                ArchConf.ras.setupLat,              //  Latency of the triangle setup units.
                                ArchConf.ras.setupStartLat,         //  Start latency of the triangle setup units.
                                ArchConf.ras.trInputLat,            //  Latency of the triangle input bus (from PA/CLP).
                                ArchConf.ras.trOutputLat,           //  Latency of the triangle output bus (between TSetup and TTraversal).
                                ArchConf.ras.shadedSetup,           //  Triangle setup performed in the unified shader unit.
                                ArchConf.ras.triangleShQSz,         //  Triangle shader queue in Triangle Setup.
                                ArchConf.ras.stampsCycle,           //  Stamps per cycle.
                                ArchConf.ras.samplesCycle,          //  MSAA samples per fragment and cycle that can be generated.
                                ArchConf.ras.overScanWidth,         //  Over scan tile width (scan tiles).
                                ArchConf.ras.overScanHeight,        //  Over scan tile height (scan tiles).
                                ArchConf.ras.scanWidth,             //  Scan tile width (pixels).
                                ArchConf.ras.scanHeight,            //  Scan tile height (pixels).
                                ArchConf.ras.genWidth,              //  Generation tile width (pixels).
                                ArchConf.ras.genHeight,             //  Generation tile height (pixels).
                                ArchConf.ras.rastBatch,             //  Triangles per rasterization batch (recursive traversal).
                                ArchConf.ras.batchQueueSize,        //  Triangles in the batch queue (triangle traversal).
                                ArchConf.ras.recursive,             //  Rasterization method (T: recursive, F: scanline).
                                ArchConf.ras.disableHZ,             //  Disables Hierarchical Z.
                                ArchConf.ras.stampsHZBlock,         //  Stamps (quads) per HZ block.
                                ArchConf.ras.hzBufferSize,          //  Size of the HZ Buffer Level 0.
                                ArchConf.ras.hzCacheLines,          //  Lines in the HZ cache.
                                ArchConf.ras.hzCacheLineSize,       //  Blocks in a HZ cache line.
                                ArchConf.ras.earlyZQueueSz,         //  Size of the early HZ test stamp queue.
                                ArchConf.ras.hzAccessLatency,       //  Access latency to the HZ Buffer Level 0.
                                ArchConf.ras.hzUpdateLatency,       //  Hierarchical Update signal latency.
                                ArchConf.ras.hzBlockClearCycle,     //  Clear HZ blocks per cycle.
                                ArchConf.ras.numInterpolators,      //  Interpolator units available.
                                ArchConf.ras.shInputQSize,          //  Shader input queue size (per shader unit).
                                ArchConf.ras.shOutputQSize,         //  Shader output queue size (per shader unit).
                                ArchConf.ras.shInputBatchSize,      //  Number of consecutive shader inputs assigned to a shader unit work batch.
                                ArchConf.ras.tiledShDistro,         //  Enable/disable distribution of fragment inputs to the shader units on a tile basis.
                                ArchConf.gpu.numVShaders,           //  Number of virtual vertex shaders simulated by the cmoRasterizer.
                                vshPrefix,                      //  Prefixes for the virtual vertex shader units.
                                ArchConf.gpu.numFShaders,           //  Number of fragment shaders attached to the cmoRasterizer.
                                fshPrefix,                      //  Prefixes for the fragment shader units.
                                ArchConf.ush.inputsCycle,           //  Shader inputs per cycle and shader.
                                ArchConf.ush.outputsCycle,          //  Shader outputs per cycle and shader.
                                threadGroup,                    //  Threads in a shader processing group.
                                ArchConf.ush.outputLatency,         //  Shader output signal maximum latency.
                                ArchConf.ras.vInputQSize,           //  Vertex input queue size.
                                ArchConf.ras.vShadedQSize,          //  Shaded vertex queue size.
                                ArchConf.ras.trInputQSize,          //  Triangle input queue size.
                                ArchConf.ras.trOutputQSize,         //  Triangle output queue size.
                                ArchConf.ras.genStampQSize,         //  Stamps in the generated stamp queue (Fragment FIFO) (per stamp unit).
                                ArchConf.ras.testStampQSize,        //  Stamps in the early z tested stamp queue (Fragment FIFO).
                                ArchConf.ras.intStampQSize,         //  Stamps in the interpolated stamp queue (Fragment FIFO).
                                ArchConf.ras.shadedStampQSize,      //  Stamps in the shaded stamp queue (Fragment FIFO) (per stamp unit).
                                ArchConf.gpu.numStampUnits,         //  Number of stamp units attached to the cmoRasterizer.
                                suPrefix,                       //  Prefixes for the stamp units.
                                "Rasterizer_Unified", NULL);

        //  Add cmoRasterizer mdu to the mdu array.
        MduArray.push_back(Raster);
        if (multiClock)
            GpuDomainMduArray.push_back(Raster);
    }
    CG_ASSERT_COND((ArchConf.ush.textureUnits != 0), "The unified shader requires at least one texture unit attached to it.");

    //  Allocate behaviorModel and mdu arrays for the fragment shaders.
    bmTexture       = new bmoTextureProcessor*[ArchConf.gpu.numFShaders];
    cmTexture       = new cmoTextureProcessor*[ArchConf.gpu.numFShaders * ArchConf.ush.textureUnits];
    cmUnifiedShader = new cmoUnifiedShader*[ArchConf.gpu.numFShaders];
    CG_ASSERT_COND((bmTexture       != NULL), "Error allocating texture behaviorModel array.");
    CG_ASSERT_COND((cmTexture       != NULL), "Error allocating texture unit array.");
    CG_ASSERT_COND((cmUnifiedShader != NULL), "Error allocating unified shader array.");
    //  Create the fragment shader boxes and emulators.
    for(U32 i = 0; i < ArchConf.gpu.numFShaders; i++)
    {
        //  Create texture behaviorModel for FS1.
        bmTexture[i] = new bmoTextureProcessor(
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

        //  Create the attached texture units.
        for(U32 j = 0; j < ArchConf.ush.textureUnits; j++)
        {
            //  Creat Behavior Model name.
            sprintf(mduName, "TextureUnitFSh%d-%d", i, j);
            sprintf(tuPrefix, "%sTU%d", fshPrefix[i], j);

            //  Create and initialize texture unit mdu for FS1.
            cmTexture[i * ArchConf.ush.textureUnits + j] = new cmoTextureProcessor(
                &ArchConf,
                bmTexture[i],                     //  Texture Behavior Model.
                //STAMP_FRAGMENTS,                //  Fragments per stamp.
                //ArchConf.ush.textRequestRate,       //  Texture request rate.
                //ArchConf.ush.textReqQSize,          //  Texture Access Request queue size.
                //ArchConf.ush.textAccessQSize,       //  Texture Access queue size.
                //ArchConf.ush.textResultQSize,       //  Texture Result queue size.
                //ArchConf.ush.textWaitWSize,         //  Texture Wait Read window size.
                //ArchConf.ush.twoLevelCache,         //  Enable two level or single level texture cache version.
                //ArchConf.ush.txCacheLineSz,         //  Size of the Texture Cache lines in bytes (L0 for two level version).
                //ArchConf.ush.txCacheWays,           //  Number of ways in the Texture Cache (L0 for two level version).
                //ArchConf.ush.txCacheLines,          //  Number of lines in the Texture Cache (L0 for two level version).
                //ArchConf.ush.txCachePortWidth,      //  Size of the Texture Cache ports.
                //ArchConf.ush.txCacheReqQSz,         //  Size of the Texture Cache request queue.
                //ArchConf.ush.txCacheInQSz,          //  Size of the Texture Cache input request queue (L0 for two level version).
                //ArchConf.ush.txCacheMissCycle,      //  Missses per cycle supported by the Texture Cache.
                //ArchConf.ush.txCacheDecomprLatency, //  Compressed texture line decompression latency.
                //ArchConf.ush.txCacheLineSzL1,       //  Size of the Texture Cache lines in bytes (L1 for two level version).
                //ArchConf.ush.txCacheWaysL1,         //  Number of ways in the Texture Cache (L1 for two level version).
                //ArchConf.ush.txCacheLinesL1,        //  Number of lines in the Texture Cache (L1 for two level version).
                //ArchConf.ush.txCacheInQSzL1,        //  Size of the Texture Cache input request queue (L1 for two level version).
                //ArchConf.ush.addressALULat,         //  Latency of the address ALU.
                //ArchConf.ush.filterLat,             //  Latency of the filter unit.
                mduName,                        //  Unit name.
                tuPrefix,                       //  Unit prefix.
                NULL                            //  Parent mdu.
                );

            //  Add Texture Unit mdu to the mdu array.
            MduArray.push_back(cmTexture[i * ArchConf.ush.textureUnits + j]);
            if (multiClock)
                GpuDomainMduArray.push_back(cmTexture[i * ArchConf.ush.textureUnits + j]);
        }

        cmUnifiedShader[i] = new cmoUnifiedShader(
            &ArchConf,
            //bmTexture[i],
            i
        );
        cmUnifiedShader[i]->SetTP(bmTexture[i]);

        if (ArchConf.ush.useVectorShader)
        {
            //  Add cmoShaderFetchVector mdu to the mdu array.
            MduArray.push_back(cmUnifiedShader[i]->vecShFetch);
            if (multiClock && shaderClockDomain)
                ShaderDomainMduArray.push_back(cmUnifiedShader[i]->vecShFetch);
            else if (multiClock && !shaderClockDomain)
                GpuDomainMduArray.push_back(cmUnifiedShader[i]->vecShFetch);
            //  Add cmoShaderDecExeVector mdu to the mdu array.
            MduArray.push_back(cmUnifiedShader[i]->vecShDecExec);
            if (multiClock && shaderClockDomain)
                ShaderDomainMduArray.push_back(cmUnifiedShader[i]->vecShDecExec);
            else if (multiClock && !shaderClockDomain)
                GpuDomainMduArray.push_back(cmUnifiedShader[i]->vecShDecExec);     
        }
        else
        {
            MduArray.push_back(cmUnifiedShader[i]->fshFetch);//  Add cmoShaderFetch mdu to the mdu array.
            MduArray.push_back(cmUnifiedShader[i]->fshDecExec);//  Add cmoShaderDecExe mdu to the mdu array.
        }

    }

    //  Allocate behaviorModel and mdu arrays for the stamp units.
    bmFragOp = new bmoFragmentOperator*[ArchConf.gpu.numStampUnits];
    zStencilV2 = new cmoDepthStencilTester*[ArchConf.gpu.numStampUnits];
    colorWriteV2 = new cmoColorWriter*[ArchConf.gpu.numStampUnits];
    //  Check allocation.
    CG_ASSERT_COND((bmFragOp      != NULL), "Error allocating fragment behaviorModel array.");
    CG_ASSERT_COND((zStencilV2   != NULL), "Error allocating Z Stencil Test array.");
    CG_ASSERT_COND((colorWriteV2 != NULL), "Error allocating Color Write array.");

    //  Create the fragment shader boxes and emulators.
    for(U32 i = 0; i < ArchConf.gpu.numStampUnits; i++)
    {
        //  Create fragment operations behaviorModel.
        bmFragOp[i] = new bmoFragmentOperator( STAMP_FRAGMENTS ); //  Fragments per stamp.
        sprintf(mduName,"ZStencilTest%d", i); //  Creat Behavior Model name.
        //  Create Z Stencil Test mdu.
        zStencilV2[i] = new cmoDepthStencilTester(
            ArchConf.zst.stampsCycle,           //  Stamps per cycle.
            ArchConf.ras.overScanWidth,         //  Over scan tile width (scan tiles).
            ArchConf.ras.overScanHeight,        //  Over scan tile height (scan tiles).
            ArchConf.ras.scanWidth,             //  Scan tile width (pixels).
            ArchConf.ras.scanHeight,            //  Scan tile height (pixels).
            ArchConf.ras.genWidth,              //  Generation tile width (pixels).
            ArchConf.ras.genHeight,             //  Generation tile height (pixels).
            ArchConf.zst.disableCompr,          //  Disables Z compression.
            ArchConf.zst.zCacheWays,            //  Z cache set associativity.
            ArchConf.zst.zCacheLines,           //  Cache lines (per way).
            ArchConf.zst.zCacheLineStamps,      //  Stamps per cache line.
            ArchConf.zst.zCachePortWidth,       //  Width in bytes of the Z cache ports.
            ArchConf.zst.extraReadPort,         //  Additional read port in the cache.
            ArchConf.zst.extraWritePort,        //  Additional write port in the cache.
            ArchConf.zst.zCacheReqQSz,          //  Z Cache memory request queue size.
            ArchConf.zst.zCacheInQSz,           //  Z Cache input queue.
            ArchConf.zst.zCacheOutQSz,          //  Z cache output queue.
            ArchConf.gpu.numStampUnits,         //  Number of stamp units in the GPU.
            ArchConf.zst.blockStateMemSz,       //  Z blocks in state memory (max res 2048 x 2048 with 64 pixel lines.
            ArchConf.zst.blockClearCycle,       //  State memory blocks cleared per cycle.
            ArchConf.zst.comprLatency,          //  Z block compression cycles.
            ArchConf.zst.decomprLatency,        //  Z block decompression cycles.
            ArchConf.zst.inputQueueSize,        //  Input stamps queue.
            ArchConf.zst.fetchQueueSize,        //  Fetched stamp queue.
            ArchConf.zst.readQueueSize,         //  Read stamp queue.
            ArchConf.zst.opQueueSize,           //  Operated stamp queue.
            ArchConf.zst.writeQueueSize,        //  Written stamp queue.
            ArchConf.zst.zTestRate,             //  Z test rate for stamps (cycles between stamps).
            ArchConf.zst.zOpLatency,            //  Z operation latency.
            ArchConf.ras.disableHZ,             //  Disables HZ.
            ArchConf.ras.hzUpdateLatency,       //  Hierarchical Z update signal latency.
            ArchConf.dac.blockUpdateLat,        //  Z Stencil block update signal latency.
            *bmFragOp[i],                    //  Reference to the Fragment Operation Behavior Model.
            *bmRaster,                       //  Reference to the cmoRasterizer Behavior Model.
            mduName,                        //  cmoMduBase name.
            suPrefix[i],                    //  Unit prefix.
            NULL                            //  cmoMduBase parent mdu.
            );

        //  Add Z Stencil Test mdu to the mdu array.
        MduArray.push_back(zStencilV2[i]);
        if (multiClock)
            GpuDomainMduArray.push_back(zStencilV2[i]);
            
        // Creat Behavior Model name.
        sprintf(mduName,"ColorWrite%d", i);

        //  Create color write mdu.
        colorWriteV2[i] = new cmoColorWriter(
            ArchConf.cwr.stampsCycle,           //  Stamps per cycle.
            ArchConf.ras.overScanWidth,         //  Over scan tile width (scan tiles).
            ArchConf.ras.overScanHeight,        //  Over scan tile height (scan tiles).
            ArchConf.ras.scanWidth,             //  Scan tile width (pixels).
            ArchConf.ras.scanHeight,            //  Scan tile height (pixels).
            ArchConf.ras.genWidth,              //  Generation tile width (pixels).
            ArchConf.ras.genHeight,             //  Generation tile height (pixels).
            ArchConf.cwr.disableCompr,          //  Disables Color compression.
            ArchConf.cwr.cCacheWays,            //  Color cache set associativity.
            ArchConf.cwr.cCacheLines,           //  Cache lines (per way).
            ArchConf.cwr.cCacheLineStamps,      //  Stamps per cache line.
            ArchConf.cwr.cCachePortWidth,       //  Width in bytes of the Color cache ports.
            ArchConf.cwr.extraReadPort,         //  Additional read port in the cache.
            ArchConf.cwr.extraWritePort,        //  Additional write port in the cache.
            ArchConf.cwr.cCacheReqQSz,          //  Color Cache memory request queue size.
            ArchConf.cwr.cCacheInQSz,           //  Color Cache input queue.
            ArchConf.cwr.cCacheOutQSz,          //  Color cache output queue.
            ArchConf.gpu.numStampUnits,         //  Number of stamp units in the GPU.
            ArchConf.cwr.blockStateMemSz,       //  Color blocks in state memory (max res 2048 x 2048 with 64 pixel lines.
            ArchConf.cwr.blockClearCycle,       //  State memory blocks cleared per cycle.
            ArchConf.cwr.comprLatency,          //  Color block compression cycles.
            ArchConf.cwr.decomprLatency,        //  Color block decompression cycles.
            ArchConf.cwr.inputQueueSize,        //  Input stamps queue.
            ArchConf.cwr.fetchQueueSize,        //  Fetched stamp queue.
            ArchConf.cwr.readQueueSize,         //  Read stamp queue.
            ArchConf.cwr.opQueueSize,           //  Operated stamp queue.
            ArchConf.cwr.writeQueueSize,        //  Written stamp queue.
            ArchConf.cwr.blendRate,             //  Rate for stamp blending (cycles between stamps).
            ArchConf.cwr.blendOpLatency,        //  Blend operation latency.
            ArchConf.dac.blockUpdateLat,        //  Color block update signal latency.
            ArchConf.sim.fragmentMap ? ArchConf.sim.fragmentMapMode : DISABLE_MAP,  //  Fragment map mode.
            *bmFragOp[i],                    //  Reference to the Fragment Operation Behavior Model.
            mduName,                        //  cmoMduBase name.
            suPrefix[i],                    //  Unit prefix.
            NULL                            //  cmoMduBase parent mdu.
            );

        //  Add Color Write mdu to the mdu array.
        MduArray.push_back(colorWriteV2[i]);
        if (multiClock)
            GpuDomainMduArray.push_back(colorWriteV2[i]);
    }

    //  Create cmoDisplayController mdu.
    dac = new cmoDisplayController(
        ArchConf.ras.overScanWidth,         //  Over scan tile width (scan tiles).
        ArchConf.ras.overScanHeight,        //  Over scan tile height (scan tiles).
        ArchConf.ras.scanWidth,             //  Scan tile width (pixels).
        ArchConf.ras.scanHeight,            //  Scan tile height (pixels).
        ArchConf.ras.genWidth,              //  Generation tile width (pixels).
        ArchConf.ras.genHeight,             //  Generation tile height (pixels).
        ArchConf.ush.texBlockDim,          //  Texture block dimension (texels): 2^n x 2^n.
        ArchConf.ush.texSuperBlockDim,         //  Texture superblock dimension (blocks): 2^m x 2^m.
        ArchConf.dac.blockSize,             //  Size of an uncompressed block in bytes.
        ArchConf.dac.blockUpdateLat,        //  Block state update signal latency.
        ArchConf.dac.blocksCycle,           //  Block states updated per cycle.
        ArchConf.dac.blockReqQSz,           //  Number of block request queue entries.
        ArchConf.dac.decomprLatency,        //  Block decompression latency.
        ArchConf.gpu.numStampUnits,         //  Number of stamp units attached to the cmoDisplayController.
        suPrefix,                       //  Prefixes for the stamp units.
        ArchConf.sim.startFrame,                //  Number of the first frame to dump.
        ArchConf.dac.refreshRate,           //  Frame refresh/dumping in cycles.
        ArchConf.dac.synchedRefresh,        //  Swap synchronized with frame refresh/dumping.
        ArchConf.dac.refreshFrame,          //  Enable frame refresh/dump.
        ArchConf.dac.saveBlitSource,        //  Save blit source data to a PPM file.
        "cmoDisplayController",                          //  cmoMduBase name.
        NULL                            //  Parent mdu.
    );

    //  Add cmoDisplayController mdu to the mdu array.
    MduArray.push_back(dac);
    if (multiClock)
        GpuDomainMduArray.push_back(dac);
    //  Check that all the signals are well defined.

}

cmoGpuTop::~cmoGpuTop()
{
    for(U32 i = 0; i < MduArray.size(); i++)
        delete MduArray[i];
}



}   // namespace arch

