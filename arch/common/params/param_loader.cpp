#include "param_loader.hpp"
#include "ArchConfig.h"   // for cgsArchConfig and nested structs
#include <filesystem>
#include <cstring>
#include <iostream>

// ---------- Singleton instance ----------
ArchParams& ArchParams::instance() {
    static ArchParams obj;
    return obj;
}

// ---------- Explicit initializer ----------
void ArchParams::init(const std::string& csv_path, const std::string& arch_name) {
    ArchParams& inst = instance();
    inst.read_csv_file(csv_path, arch_name);
    std::cout << "[ArchParams] Loaded " << inst.param_map_.size()
              << " params from " << csv_path
              << " (arch=" << arch_name << ")" << std::endl;
}

// ---------- Private constructor ----------
ArchParams::ArchParams() : col_num_(-1), initialized_(false) {}

// ---------- CSV reader ----------
void ArchParams::read_csv_file(const std::string& file_path, const std::string& arch_name) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("[ArchParams] Cannot open CSV file: " + file_path);
    }

    file_path_ = file_path;
    current_arch_ = arch_name;
    param_map_.clear();
    col_num_ = -1;

    std::string line;

    // Line 1: ARCH_VERSION row — find the column index for the requested arch.
    //         Format: ARCH_VERSION,CG1GPU.ini,CG1GPUx1.ini,...,explanation
    if (!std::getline(file, line)) {
        throw std::runtime_error("[ArchParams] CSV file is empty: " + file_path);
    }

    // If the first row is a header row ("module_param,..."), skip it and read next.
    if (line.substr(0, 12) == "module_param") {
        if (!std::getline(file, line)) {
            throw std::runtime_error("[ArchParams] CSV file has header but no ARCH_VERSION row");
        }
    }

    auto header = split(line, ',');
    for (int i = 0; i < static_cast<int>(header.size()); i++) {
        if (header[i] == arch_name) {
            col_num_ = i;
            break;
        }
    }
    if (col_num_ < 0) {
        throw std::runtime_error("[ArchParams] Arch '" + arch_name + "' not found in CSV: " + file_path);
    }

    // Remaining lines: param_name,val1,val2,...,explanation
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        // Skip commented-out params (lines starting with #)
        if (line[0] == '#') continue;

        auto cols = split(line, ',');
        if (cols.empty()) continue;

        const std::string& param_name = cols[0];
        if (param_name.empty() || param_name == "ARCH_VERSION") continue;

        std::string value;
        if (col_num_ < static_cast<int>(cols.size())) {
            value = cols[col_num_];
        }
        // Store even if empty (caller can handle missing values)
        param_map_[param_name] = value;
    }

    initialized_ = true;
}

// ---------- Raw string lookup ----------
const char* ArchParams::operator[](const std::string& param_name) const {
    auto it = param_map_.find(param_name);
    return (it != param_map_.end()) ? it->second.c_str() : nullptr;
}

// ---------- Runtime override ----------
void ArchParams::set(const std::string& param_name, const std::string& value) {
    param_map_[param_name] = value;
}

// ---------- Build a cgsArchConfig from current singleton state ----------
cgsArchConfig ArchParams::toArchConfig() const {
    cgsArchConfig arch_conf{};
    populateArchConfig(&arch_conf);
    return arch_conf;
}

// ---------- Reverse-populate: set param_map_ from legacy struct ----------
void ArchParams::initFromArchConfig(const cgsArchConfig& a) {
    ArchParams& inst = instance();
    inst.param_map_.clear();
    inst.current_arch_ = "legacy_ini";
    inst.file_path_    = "(from cgsArchConfig)";

    auto S = [&](const std::string& k, const char* v)      { if (v) inst.param_map_[k] = v; };
    auto B = [&](const std::string& k, bool v)              { inst.param_map_[k] = v ? "TRUE" : "FALSE"; };
    auto U = [&](const std::string& k, uint32_t v)          { inst.param_map_[k] = std::to_string(v); };
    auto U6 = [&](const std::string& k, uint64_t v)         { inst.param_map_[k] = std::to_string(v); };
    auto F = [&](const std::string& k, float v)             { inst.param_map_[k] = std::to_string(v); };

    // [SIMULATOR]
    S("SIMULATOR_InputFile",           a.sim.inputFile);
    U6("SIMULATOR_SimCycles",          a.sim.simCycles);
    U("SIMULATOR_SimFrames",           a.sim.simFrames);
    S("SIMULATOR_SignalDumpFile",      a.sim.signalDumpFile);
    S("SIMULATOR_StatsFile",           a.sim.statsFile);
    S("SIMULATOR_StatsFilePerFrame",   a.sim.statsFilePerFrame);
    S("SIMULATOR_StatsFilePerBatch",   a.sim.statsFilePerBatch);
    U("SIMULATOR_StartFrame",         a.sim.startFrame);
    U6("SIMULATOR_StartSignalDump",    a.sim.startDump);
    U6("SIMULATOR_SignalDumpCycles",   a.sim.dumpCycles);
    U("SIMULATOR_StatisticsRate",      a.sim.statsRate);
    B("SIMULATOR_DumpSignalTrace",     a.sim.dumpSignalTrace);
    B("SIMULATOR_Statistics",          a.sim.statistics);
    B("SIMULATOR_PerCycleStatistics",  a.sim.perCycleStatistics);
    B("SIMULATOR_PerFrameStatistics",  a.sim.perFrameStatistics);
    B("SIMULATOR_PerBatchStatistics",  a.sim.perBatchStatistics);
    B("SIMULATOR_DetectStalls",        a.sim.detectStalls);
    B("SIMULATOR_GenerateFragmentMap", a.sim.fragmentMap);
    U("SIMULATOR_FragmentMapMode",     a.sim.fragmentMapMode);
    B("SIMULATOR_DoubleBuffer",        a.sim.colorDoubleBuffer);
    B("SIMULATOR_ForceMSAA",           a.sim.forceMSAA);
    U("SIMULATOR_MSAASamples",         a.sim.msaaSamples);
    B("SIMULATOR_ForceFP16ColorBuffer",a.sim.forceFP16ColorBuffer);
    B("SIMULATOR_EnableDriverShaderTranslation", a.sim.enableDriverShaderTranslation);
    U("SIMULATOR_ObjectSize0",         a.sim.objectSize0);
    U("SIMULATOR_BucketSize0",         a.sim.bucketSize0);
    U("SIMULATOR_ObjectSize1",         a.sim.objectSize1);
    U("SIMULATOR_BucketSize1",         a.sim.bucketSize1);
    U("SIMULATOR_ObjectSize2",         a.sim.objectSize2);
    U("SIMULATOR_BucketSize2",         a.sim.bucketSize2);

    // [GPU]
    U("GPU_NumVertexShaders",          a.gpu.numVShaders);
    U("GPU_NumFragmentShaders",        a.gpu.numFShaders);
    U("GPU_NumStampPipes",             a.gpu.numStampUnits);
    U("GPU_GPUClock",                  a.gpu.gpuClock);
    U("GPU_ShaderClock",               a.gpu.shaderClock);
    U("GPU_MemoryClock",               a.gpu.memoryClock);

    // [COMMANDPROCESSOR]
    B("COMMANDPROCESSOR_PipelinedBatchRendering", a.com.pipelinedBatches);
    B("COMMANDPROCESSOR_DumpShaderPrograms",      a.com.dumpShaderPrograms);

    // [MEMORYCONTROLLER]
    U("MEMORYCONTROLLER_MemorySize",            a.mem.memSize);
    U("MEMORYCONTROLLER_MemoryClockMultiplier", a.mem.clockMultiplier);
    U("MEMORYCONTROLLER_MemoryFrequency",       a.mem.memoryFrequency);
    U("MEMORYCONTROLLER_MemoryBusWidth",        a.mem.busWidth);
    U("MEMORYCONTROLLER_MemoryBuses",           a.mem.memBuses);
    B("MEMORYCONTROLLER_SharedBanks",           a.mem.sharedBanks);
    U("MEMORYCONTROLLER_BankGranurality",       a.mem.bankGranurality);
    U("MEMORYCONTROLLER_BurstLength",           a.mem.burstLength);
    U("MEMORYCONTROLLER_ReadLatency",           a.mem.readLatency);
    U("MEMORYCONTROLLER_WriteLatency",          a.mem.writeLatency);
    U("MEMORYCONTROLLER_WriteToReadLatency",    a.mem.writeToReadLat);
    U("MEMORYCONTROLLER_MemoryPageSize",        a.mem.memPageSize);
    U("MEMORYCONTROLLER_OpenPages",             a.mem.openPages);
    U("MEMORYCONTROLLER_PageOpenLatency",       a.mem.pageOpenLat);
    U("MEMORYCONTROLLER_MaxConsecutiveReads",   a.mem.maxConsecutiveReads);
    U("MEMORYCONTROLLER_MaxConsecutiveWrites",  a.mem.maxConsecutiveWrites);
    U("MEMORYCONTROLLER_CommandProcessorBusWidth", a.mem.comProcBus);
    U("MEMORYCONTROLLER_StreamerFetchBusWidth",    a.mem.strFetchBus);
    U("MEMORYCONTROLLER_StreamerLoaderBusWidth",   a.mem.strLoaderBus);
    U("MEMORYCONTROLLER_ZStencilBusWidth",         a.mem.zStencilBus);
    U("MEMORYCONTROLLER_ColorWriteBusWidth",       a.mem.cWriteBus);
    U("MEMORYCONTROLLER_DisplayControllerBusWidth",a.mem.dacBus);
    U("MEMORYCONTROLLER_TextureUnitBusWidth",      a.mem.textUnitBus);
    U("MEMORYCONTROLLER_MappedMemorySize",         a.mem.mappedMemSize);
    U("MEMORYCONTROLLER_ReadBufferLines",          a.mem.readBufferLines);
    U("MEMORYCONTROLLER_WriteBufferLines",         a.mem.writeBufferLines);
    U("MEMORYCONTROLLER_RequestQueueSize",         a.mem.reqQueueSz);
    U("MEMORYCONTROLLER_ServiceQueueSize",         a.mem.servQueueSz);
    B("MEMORYCONTROLLER_MemoryControllerV2",       a.mem.memoryControllerV2);
    U("MEMORYCONTROLLER_V2MemoryChannels",         a.mem.v2MemoryChannels);
    U("MEMORYCONTROLLER_V2BanksPerMemoryChannel",  a.mem.v2BanksPerMemoryChannel);
    U("MEMORYCONTROLLER_V2MemoryRowSize",          a.mem.v2MemoryRowSize);
    U("MEMORYCONTROLLER_V2BurstBytesPerCycle",     a.mem.v2BurstBytesPerCycle);
    U("MEMORYCONTROLLER_V2SplitterType",           a.mem.v2SplitterType);
    U("MEMORYCONTROLLER_V2ChannelInterleaving",    a.mem.v2ChannelInterleaving);
    U("MEMORYCONTROLLER_V2BankInterleaving",       a.mem.v2BankInterleaving);
    S("MEMORYCONTROLLER_V2ChannelInterleavingMask",a.mem.v2ChannelInterleavingMask);
    S("MEMORYCONTROLLER_V2BankInterleavingMask",   a.mem.v2BankInterleavingMask);
    B("MEMORYCONTROLLER_V2SecondInterleaving",     a.mem.v2SecondInterleaving);
    U("MEMORYCONTROLLER_V2SecondChannelInterleaving", a.mem.v2SecondChannelInterleaving);
    U("MEMORYCONTROLLER_V2SecondBankInterleaving",    a.mem.v2SecondBankInterleaving);
    S("MEMORYCONTROLLER_V2SecondChannelInterleavingMask", a.mem.v2SecondChannelInterleavingMask);
    S("MEMORYCONTROLLER_V2SecondBankInterleavingMask",    a.mem.v2SecondBankInterleavingMask);
    S("MEMORYCONTROLLER_V2BankSelectionPolicy",    a.mem.v2BankSelectionPolicy);
    U("MEMORYCONTROLLER_V2ChannelScheduler",       a.mem.v2ChannelScheduler);
    U("MEMORYCONTROLLER_V2PagePolicy",             a.mem.v2PagePolicy);
    U("MEMORYCONTROLLER_V2MaxChannelTransactions",           a.mem.v2MaxChannelTransactions);
    U("MEMORYCONTROLLER_V2DedicatedChannelReadTransactions", a.mem.v2DedicatedChannelReadTransactions);
    B("MEMORYCONTROLLER_V2UseChannelRequestFIFOPerBank",     a.mem.v2UseChannelRequestFIFOPerBank);
    U("MEMORYCONTROLLER_V2ChannelRequestFIFOPerBankSelection", a.mem.v2ChannelRequestFIFOPerBankSelection);
    B("MEMORYCONTROLLER_V2UseClassicSchedulerStates",   a.mem.v2UseClassicSchedulerStates);
    B("MEMORYCONTROLLER_V2UseSplitRequestBufferPerROP", a.mem.v2UseSplitRequestBufferPerROP);
    B("MEMORYCONTROLLER_V2MemoryTrace",            a.mem.v2MemoryTrace);
    U("MEMORYCONTROLLER_V2SwitchModePolicy",       a.mem.v2SwitchModePolicy);
    U("MEMORYCONTROLLER_V2ActiveManagerMode",      a.mem.v2ActiveManagerMode);
    U("MEMORYCONTROLLER_V2PrechargeManagerMode",   a.mem.v2PrechargeManagerMode);
    U("MEMORYCONTROLLER_V2ManagerSelectionAlgorithm", a.mem.v2ManagerSelectionAlgorithm);
    B("MEMORYCONTROLLER_V2DisableActiveManager",   a.mem.v2DisableActiveManager);
    B("MEMORYCONTROLLER_V2DisablePrechargeManager",a.mem.v2DisablePrechargeManager);
    S("MEMORYCONTROLLER_V2DebugString",            a.mem.v2DebugString);
    S("MEMORYCONTROLLER_V2MemoryType",             a.mem.v2MemoryType);
    S("MEMORYCONTROLLER_V2GDDR_Profile",           a.mem.v2GDDR_Profile);
    U("MEMORYCONTROLLER_V2GDDR_tRRD",  a.mem.v2GDDR_tRRD);
    U("MEMORYCONTROLLER_V2GDDR_tRCD",  a.mem.v2GDDR_tRCD);
    U("MEMORYCONTROLLER_V2GDDR_tWTR",  a.mem.v2GDDR_tWTR);
    U("MEMORYCONTROLLER_V2GDDR_tRTW",  a.mem.v2GDDR_tRTW);
    U("MEMORYCONTROLLER_V2GDDR_tWR",   a.mem.v2GDDR_tWR);
    U("MEMORYCONTROLLER_V2GDDR_tRP",   a.mem.v2GDDR_tRP);
    U("MEMORYCONTROLLER_V2GDDR_CAS",   a.mem.v2GDDR_CAS);
    U("MEMORYCONTROLLER_V2GDDR_WL",    a.mem.v2GDDR_WL);

    // [STREAMER]
    U("STREAMER_IndicesCycle",            a.str.indicesCycle);
    U("STREAMER_IndexBufferSize",         a.str.idxBufferSize);
    U("STREAMER_OutputFIFOSize",          a.str.outputFIFOSize);
    U("STREAMER_OutputMemorySize",        a.str.outputMemorySize);
    U("STREAMER_VerticesCycle",           a.str.verticesCycle);
    U("STREAMER_AttributesSentCycle",     a.str.attrSentCycle);
    U("STREAMER_StreamerLoaderUnits",     a.str.streamerLoaderUnits);
    U("STREAMER_SLIndicesCycle",          a.str.slIndicesCycle);
    U("STREAMER_SLInputRequestQueueSize", a.str.slInputReqQueueSize);
    U("STREAMER_SLAttributesCycle",       a.str.slAttributesCycle);
    U("STREAMER_SLInputCacheLines",       a.str.slInCacheLines);
    U("STREAMER_SLInputCacheLineSize",    a.str.slInCacheLineSz);
    U("STREAMER_SLInputCachePortWidth",   a.str.slInCachePortWidth);
    U("STREAMER_SLInputCacheRequestQueueSize", a.str.slInCacheReqQSz);
    U("STREAMER_SLInputCacheInputQueueSize",   a.str.slInCacheInputQSz);

    // [PRIMITIVEASSEMBLY]
    U("PRIMITIVEASSEMBLY_VerticesCycle",      a.pas.verticesCycle);
    U("PRIMITIVEASSEMBLY_TrianglesCycle",     a.pas.trianglesCycle);
    U("PRIMITIVEASSEMBLY_InputBusLatency",    a.pas.inputBusLat);
    U("PRIMITIVEASSEMBLY_AssemblyQueueSize",  a.pas.paQueueSize);

    // [CLIPPER]
    U("CLIPPER_TrianglesCycle",  a.clp.trianglesCycle);
    U("CLIPPER_ClipperUnits",    a.clp.clipperUnits);
    U("CLIPPER_StartLatency",    a.clp.startLatency);
    U("CLIPPER_ExecLatency",     a.clp.execLatency);
    U("CLIPPER_ClipBufferSize",  a.clp.clipBufferSize);

    // [RASTERIZER]
    U("RASTERIZER_TrianglesCycle",      a.ras.trianglesCycle);
    U("RASTERIZER_SetupFIFOSize",       a.ras.setupFIFOSize);
    U("RASTERIZER_SetupUnits",          a.ras.setupUnits);
    U("RASTERIZER_SetupLatency",        a.ras.setupLat);
    U("RASTERIZER_SetupStartLatency",   a.ras.setupStartLat);
    U("RASTERIZER_TriangleInputLatency",  a.ras.trInputLat);
    U("RASTERIZER_TriangleOutputLatency", a.ras.trOutputLat);
    B("RASTERIZER_TriangleSetupOnShader", a.ras.shadedSetup);
    U("RASTERIZER_TriangleShaderQueueSize", a.ras.batchQueueSize);
    U("RASTERIZER_StampsPerCycle",       a.ras.stampsCycle);
    U("RASTERIZER_MSAASamplesCycle",     a.ras.samplesCycle);
    U("RASTERIZER_OverScanWidth",        a.ras.overScanWidth);
    U("RASTERIZER_OverScanHeight",       a.ras.overScanHeight);
    U("RASTERIZER_ScanWidth",            a.ras.scanWidth);
    U("RASTERIZER_ScanHeight",           a.ras.scanHeight);
    U("RASTERIZER_GenWidth",             a.ras.genWidth);
    U("RASTERIZER_GenHeight",            a.ras.genHeight);
    U("RASTERIZER_RasterizationBatchSize", a.ras.rastBatch);
    U("RASTERIZER_BatchQueueSize",       a.ras.batchQueueSize);
    B("RASTERIZER_RecursiveMode",        a.ras.recursive);
    B("RASTERIZER_DisableHZ",            a.ras.disableHZ);
    U("RASTERIZER_StampsPerHZBlock",     a.ras.stampsHZBlock);
    U("RASTERIZER_HierarchicalZBufferSize", a.ras.hzBufferSize);
    U("RASTERIZER_HZCacheLines",         a.ras.hzCacheLines);
    U("RASTERIZER_HZCacheLineSize",      a.ras.hzCacheLineSize);
    U("RASTERIZER_EarlyZQueueSize",      a.ras.earlyZQueueSz);
    U("RASTERIZER_HZAccessLatency",      a.ras.hzAccessLatency);
    U("RASTERIZER_HZUpdateLatency",      a.ras.hzUpdateLatency);
    U("RASTERIZER_HZBlocksClearedPerCycle", a.ras.hzBlockClearCycle);
    U("RASTERIZER_NumInterpolators",     a.ras.numInterpolators);
    U("RASTERIZER_ShaderInputQueueSize", a.ras.shInputQSize);
    U("RASTERIZER_ShaderOutputQueueSize",a.ras.shOutputQSize);
    U("RASTERIZER_ShaderInputBatchSize", a.ras.shInputBatchSize);
    B("RASTERIZER_TiledShaderDistribution", a.ras.tiledShDistro);
    U("RASTERIZER_VertexInputQueueSize", a.ras.vInputQSize);
    U("RASTERIZER_ShadedVertexQueueSize",a.ras.vShadedQSize);
    U("RASTERIZER_TriangleInputQueueSize",  a.ras.trInputQSize);
    U("RASTERIZER_TriangleOutputQueueSize", a.ras.trOutputQSize);
    U("RASTERIZER_GeneratedStampQueueSize",   a.ras.genStampQSize);
    U("RASTERIZER_EarlyZTestedStampQueueSize",a.ras.testStampQSize);
    U("RASTERIZER_InterpolatedStampQueueSize",a.ras.intStampQSize);
    U("RASTERIZER_ShadedStampQueueSize",      a.ras.shadedStampQSize);
    U("RASTERIZER_EmulatorStoredTriangles",   a.ras.emuStoredTriangles);
    B("RASTERIZER_UseMicroPolygonRasterizer", a.ras.useMicroPolRast);
    B("RASTERIZER_PreBoundTriangles",    a.ras.preBoundTriangles);
    B("RASTERIZER_CullMicroTriangles",   a.ras.cullMicroTriangles);
    U("RASTERIZER_TriangleBoundOutputLatency", a.ras.trBndOutLat);
    U("RASTERIZER_TriangleBoundOpLatency",     a.ras.trBndOpLat);
    U("RASTERIZER_LargeTriangleFIFOSize",      a.ras.largeTriMinSz);
    U("RASTERIZER_MicroTriangleFIFOSize",      a.ras.trBndMicroTriFIFOSz);
    B("RASTERIZER_UseBoundingBoxOptimization",  a.ras.useBBOptimization);
    U("RASTERIZER_SubPixelPrecisionBits",       a.ras.subPixelPrecision);
    F("RASTERIZER_LargeTriangleMinSize",        a.ras.largeTriMinSz);
    B("RASTERIZER_ProcessMicroTrianglesAsFragments", a.ras.microTrisAsFragments);
    U("RASTERIZER_MicroTriangleSizeLimit",      a.ras.microTriSzLimit);
    S("RASTERIZER_MicroTriangleFlowPath",       a.ras.microTriFlowPath);
    B("RASTERIZER_DumpTriangleBurstSizeHistogram", a.ras.dumpBurstHist);

    // [UNIFIEDSHADER]
    U("UNIFIEDSHADER_ExecutableThreads",  a.ush.numThreads);
    U("UNIFIEDSHADER_InputBuffers",       a.ush.numInputBuffers);
    U("UNIFIEDSHADER_ThreadResources",    a.ush.numResources);
    U("UNIFIEDSHADER_ThreadRate",         a.ush.threadRate);
    U("UNIFIEDSHADER_FetchRate",          a.ush.fetchRate);
    B("UNIFIEDSHADER_ScalarALU",          a.ush.scalarALU);
    U("UNIFIEDSHADER_ThreadGroup",        a.ush.threadGroup);
    B("UNIFIEDSHADER_LockedExecutionMode",a.ush.lockedMode);
    B("UNIFIEDSHADER_VectorShader",       a.ush.useVectorShader);
    U("UNIFIEDSHADER_VectorThreads",      a.ush.vectorThreads);
    U("UNIFIEDSHADER_VectorResources",    a.ush.vectorResources);
    U("UNIFIEDSHADER_VectorLength",       a.ush.vectorLength);
    U("UNIFIEDSHADER_VectorALUWidth",     a.ush.vectorALUWidth);
    S("UNIFIEDSHADER_VectorALUConfig",    a.ush.vectorALUConfig);
    B("UNIFIEDSHADER_VectorWaitOnStall",  a.ush.vectorWaitOnStall);
    B("UNIFIEDSHADER_VectorExplicitBlock",a.ush.vectorExplicitBlock);
    B("UNIFIEDSHADER_VertexAttributeLoadFromShader", a.ush.vAttrLoadFromShader);
    B("UNIFIEDSHADER_ThreadWindow",       a.ush.threadWindow);
    U("UNIFIEDSHADER_FetchDelay",         a.ush.fetchDelay);
    B("UNIFIEDSHADER_SwapOnBlock",        a.ush.swapOnBlock);
    B("UNIFIEDSHADER_FixedLatencyALU",    a.ush.fixedLatencyALU);
    U("UNIFIEDSHADER_InputsPerCycle",     a.ush.inputsCycle);
    U("UNIFIEDSHADER_OutputsPerCycle",    a.ush.outputsCycle);
    U("UNIFIEDSHADER_OutputLatency",      a.ush.outputLatency);
    U("UNIFIEDSHADER_TextureUnits",       a.ush.textureUnits);
    U("UNIFIEDSHADER_TextureRequestRate", a.ush.textRequestRate);
    U("UNIFIEDSHADER_TextureRequestGroup",a.ush.textRequestGroup);
    U("UNIFIEDSHADER_AddressALULatency",  a.ush.addressALULat);
    U("UNIFIEDSHADER_FilterALULatency",   a.ush.filterLat);
    U("UNIFIEDSHADER_TextureBlockDimension",      a.ush.texBlockDim);
    U("UNIFIEDSHADER_TextureSuperBlockDimension",  a.ush.texSuperBlockDim);
    U("UNIFIEDSHADER_TextureRequestQueueSize",     a.ush.textReqQSize);
    U("UNIFIEDSHADER_TextureAccessQueue",          a.ush.textAccessQSize);
    U("UNIFIEDSHADER_TextureResultQueue",          a.ush.textResultQSize);
    U("UNIFIEDSHADER_TextureWaitReadWindow",       a.ush.textWaitWSize);
    B("UNIFIEDSHADER_TwoLevelTextureCache",        a.ush.twoLevelCache);
    U("UNIFIEDSHADER_TextureCacheLineSize",        a.ush.txCacheLineSz);
    U("UNIFIEDSHADER_TextureCacheWays",            a.ush.txCacheWays);
    U("UNIFIEDSHADER_TextureCacheLines",           a.ush.txCacheLines);
    U("UNIFIEDSHADER_TextureCachePortWidth",       a.ush.txCachePortWidth);
    U("UNIFIEDSHADER_TextureCacheRequestQueueSize",a.ush.txCacheReqQSz);
    U("UNIFIEDSHADER_TextureCacheInputQueue",      a.ush.txCacheInQSz);
    U("UNIFIEDSHADER_TextureCacheMissesPerCycle",  a.ush.txCacheMissCycle);
    U("UNIFIEDSHADER_TextureCacheDecompressLatency", a.ush.txCacheDecomprLatency);
    U("UNIFIEDSHADER_TextureCacheLineSizeL1",      a.ush.txCacheLineSzL1);
    U("UNIFIEDSHADER_TextureCacheWaysL1",          a.ush.txCacheWaysL1);
    U("UNIFIEDSHADER_TextureCacheLinesL1",         a.ush.txCacheLinesL1);
    U("UNIFIEDSHADER_TextureCacheInputQueueL1",    a.ush.txCacheInQSzL1);
    U("UNIFIEDSHADER_AnisotropyAlgorithm",         a.ush.anisoAlgo);
    B("UNIFIEDSHADER_ForceMaxAnisotropy",          a.ush.forceMaxAniso);
    U("UNIFIEDSHADER_MaxAnisotropy",               a.ush.maxAnisotropy);
    U("UNIFIEDSHADER_TrilinearPrecision",          a.ush.triPrecision);
    U("UNIFIEDSHADER_BrilinearThreshold",          a.ush.briThreshold);
    U("UNIFIEDSHADER_AnisoRoundPrecision",         a.ush.anisoRoundPrec);
    U("UNIFIEDSHADER_AnisoRoundThreshold",         a.ush.anisoRoundThres);
    B("UNIFIEDSHADER_AnisoRatioMultOfTwo",         a.ush.anisoRatioMultOf2);

    // [ZSTENCILTEST]
    U("ZSTENCILTEST_StampsPerCycle",       a.zst.stampsCycle);
    U("ZSTENCILTEST_BytesPerPixel",        a.zst.bytesPixel);
    B("ZSTENCILTEST_DisableCompression",   a.zst.disableCompr);
    U("ZSTENCILTEST_ZCacheWays",           a.zst.zCacheWays);
    U("ZSTENCILTEST_ZCacheLines",          a.zst.zCacheLines);
    U("ZSTENCILTEST_ZCacheStampsPerLine",  a.zst.zCacheLineStamps);
    U("ZSTENCILTEST_ZCachePortWidth",      a.zst.zCachePortWidth);
    B("ZSTENCILTEST_ZCacheExtraReadPort",  a.zst.extraReadPort);
    B("ZSTENCILTEST_ZCacheExtraWritePort", a.zst.extraWritePort);
    U("ZSTENCILTEST_ZCacheRequestQueueSize", a.zst.zCacheReqQSz);
    U("ZSTENCILTEST_ZCacheInputQueueSize",   a.zst.zCacheInQSz);
    U("ZSTENCILTEST_ZCacheOutputQueueSize",  a.zst.zCacheOutQSz);
    U("ZSTENCILTEST_BlockStateMemorySize",   a.zst.blockStateMemSz);
    U("ZSTENCILTEST_BlocksClearedPerCycle",  a.zst.blockClearCycle);
    U("ZSTENCILTEST_CompressionUnitLatency", a.zst.comprLatency);
    U("ZSTENCILTEST_DecompressionUnitLatency", a.zst.decomprLatency);
    U("ZSTENCILTEST_InputQueueSize",  a.zst.inputQueueSize);
    U("ZSTENCILTEST_FetchQueueSize",  a.zst.fetchQueueSize);
    U("ZSTENCILTEST_ReadQueueSize",   a.zst.readQueueSize);
    U("ZSTENCILTEST_OpQueueSize",     a.zst.opQueueSize);
    U("ZSTENCILTEST_WriteQueueSize",  a.zst.writeQueueSize);
    U("ZSTENCILTEST_ZALUTestRate",    a.zst.zTestRate);
    U("ZSTENCILTEST_ZALULatency",     a.zst.zOpLatency);
    U("ZSTENCILTEST_CompressionAlgorithm", a.zst.comprAlgo);

    // [COLORWRITE]
    U("COLORWRITE_StampsPerCycle",       a.cwr.stampsCycle);
    U("COLORWRITE_BytesPerPixel",        a.cwr.bytesPixel);
    B("COLORWRITE_DisableCompression",   a.cwr.disableCompr);
    U("COLORWRITE_ColorCacheWays",       a.cwr.cCacheWays);
    U("COLORWRITE_ColorCacheLines",      a.cwr.cCacheLines);
    U("COLORWRITE_ColorCacheStampsPerLine", a.cwr.cCacheLineStamps);
    U("COLORWRITE_ColorCachePortWidth",     a.cwr.cCachePortWidth);
    B("COLORWRITE_ColorCacheExtraReadPort", a.cwr.extraReadPort);
    B("COLORWRITE_ColorCacheExtraWritePort",a.cwr.extraWritePort);
    U("COLORWRITE_ColorCacheRequestQueueSize", a.cwr.cCacheReqQSz);
    U("COLORWRITE_ColorCacheInputQueueSize",   a.cwr.cCacheInQSz);
    U("COLORWRITE_ColorCacheOutputQueueSize",  a.cwr.cCacheOutQSz);
    U("COLORWRITE_BlockStateMemorySize",   a.cwr.blockStateMemSz);
    U("COLORWRITE_BlocksClearedPerCycle",  a.cwr.blockClearCycle);
    U("COLORWRITE_CompressionUnitLatency", a.cwr.comprLatency);
    U("COLORWRITE_DecompressionUnitLatency", a.cwr.decomprLatency);
    U("COLORWRITE_InputQueueSize",  a.cwr.inputQueueSize);
    U("COLORWRITE_FetchQueueSize",  a.cwr.fetchQueueSize);
    U("COLORWRITE_ReadQueueSize",   a.cwr.readQueueSize);
    U("COLORWRITE_OpQueueSize",     a.cwr.opQueueSize);
    U("COLORWRITE_WriteQueueSize",  a.cwr.writeQueueSize);
    U("COLORWRITE_BlendALURate",    a.cwr.blendRate);
    U("COLORWRITE_BlendALULatency", a.cwr.blendOpLatency);
    U("COLORWRITE_CompressionAlgorithm", a.cwr.comprAlgo);

    // [DISPLAYCONTROLLER]
    U("DISPLAYCONTROLLER_BytesPerPixel",        a.dac.bytesPixel);
    U("DISPLAYCONTROLLER_BlockSize",            a.dac.blockSize);
    U("DISPLAYCONTROLLER_BlockUpdateLatency",   a.dac.blockUpdateLat);
    U("DISPLAYCONTROLLER_BlocksUpdatedPerCycle",a.dac.blocksCycle);
    U("DISPLAYCONTROLLER_BlockRequestQueueSize",a.dac.blockReqQSz);
    U("DISPLAYCONTROLLER_DecompressionUnitLatency", a.dac.decomprLatency);
    U("DISPLAYCONTROLLER_RefreshRate",          a.dac.refreshRate);
    B("DISPLAYCONTROLLER_SynchedRefresh",       a.dac.synchedRefresh);
    B("DISPLAYCONTROLLER_RefreshFrame",         a.dac.refreshFrame);
    B("DISPLAYCONTROLLER_SaveBlitSourceData",   a.dac.saveBlitSource);

    inst.initialized_ = true;
    std::cout << "[ArchParams] Reverse-populated " << inst.param_map_.size()
              << " params from cgsArchConfig" << std::endl;
}

// ---------- Helper: strdup from string (caller owns memory) ----------
static char* str_dup(const std::string& s) {
    char* p = new char[s.size() + 1];
    std::strcpy(p, s.c_str());
    return p;
}

// ---------- Populate legacy cgsArchConfig struct from CSV params ----------
void ArchParams::populateArchConfig(cgsArchConfig* arch_conf) const {
    if (!initialized_ || !arch_conf) return;

    auto s = [&](const std::string& key) -> std::string {
        auto it = param_map_.find(key);
        return (it != param_map_.end()) ? it->second : "";
    };
    auto b = [&](const std::string& key, bool def = false) -> bool {
        std::string v = s(key);
        if (v.empty()) return def;
        std::string upper = v;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
        if (upper == "TRUE") return true;
        if (upper == "FALSE") return false;
        return def;
    };
    auto u32 = [&](const std::string& key, uint32_t def = 0) -> uint32_t {
        std::string v = s(key);
        if (v.empty()) return def;
        try { return static_cast<uint32_t>(std::stoul(v, nullptr, 0)); }
        catch (...) { return def; }
    };
    auto u64 = [&](const std::string& key, uint64_t def = 0) -> uint64_t {
        std::string v = s(key);
        if (v.empty()) return def;
        try { return std::stoull(v, nullptr, 0); }
        catch (...) { return def; }
    };
    auto f64 = [&](const std::string& key, double def = 0.0) -> double {
        std::string v = s(key);
        if (v.empty()) return def;
        try { return std::stod(v); }
        catch (...) { return def; }
    };

    // ===== [SIMULATOR] =====
    arch_conf->sim.inputFile      = str_dup(s("SIMULATOR_InputFile"));
    arch_conf->sim.simCycles      = u64("SIMULATOR_SimCycles");
    arch_conf->sim.simFrames      = u32("SIMULATOR_SimFrames");
    arch_conf->sim.signalDumpFile = str_dup(s("SIMULATOR_SignalDumpFile"));
    arch_conf->sim.statsFile      = str_dup(s("SIMULATOR_StatsFile"));
    arch_conf->sim.statsFilePerFrame = str_dup(s("SIMULATOR_StatsFilePerFrame"));
    arch_conf->sim.statsFilePerBatch = str_dup(s("SIMULATOR_StatsFilePerBatch"));
    arch_conf->sim.startFrame     = u32("SIMULATOR_StartFrame");
    arch_conf->sim.startDump      = u64("SIMULATOR_StartSignalDump");
    arch_conf->sim.dumpCycles     = u64("SIMULATOR_SignalDumpCycles");
    arch_conf->sim.statsRate      = u32("SIMULATOR_StatisticsRate");
    arch_conf->sim.dumpSignalTrace = b("SIMULATOR_DumpSignalTrace");
    arch_conf->sim.statistics      = b("SIMULATOR_Statistics");
    arch_conf->sim.perCycleStatistics = b("SIMULATOR_PerCycleStatistics");
    arch_conf->sim.perFrameStatistics = b("SIMULATOR_PerFrameStatistics");
    arch_conf->sim.perBatchStatistics = b("SIMULATOR_PerBatchStatistics");
    arch_conf->sim.detectStalls    = b("SIMULATOR_DetectStalls");
    arch_conf->sim.fragmentMap     = b("SIMULATOR_GenerateFragmentMap");
    arch_conf->sim.fragmentMapMode = u32("SIMULATOR_FragmentMapMode");
    arch_conf->sim.colorDoubleBuffer = b("SIMULATOR_DoubleBuffer");
    arch_conf->sim.forceMSAA       = b("SIMULATOR_ForceMSAA");
    arch_conf->sim.msaaSamples     = u32("SIMULATOR_MSAASamples", 4);
    arch_conf->sim.forceFP16ColorBuffer = b("SIMULATOR_ForceFP16ColorBuffer");
    arch_conf->sim.enableDriverShaderTranslation = b("SIMULATOR_EnableDriverShaderTranslation", true);
    arch_conf->sim.objectSize0     = u32("SIMULATOR_ObjectSize0", 512);
    arch_conf->sim.bucketSize0     = u32("SIMULATOR_BucketSize0", 262144);
    arch_conf->sim.objectSize1     = u32("SIMULATOR_ObjectSize1", 4096);
    arch_conf->sim.bucketSize1     = u32("SIMULATOR_BucketSize1", 32768);
    arch_conf->sim.objectSize2     = u32("SIMULATOR_ObjectSize2", 64);
    arch_conf->sim.bucketSize2     = u32("SIMULATOR_BucketSize2", 65536);

    // ===== [GPU] =====
    arch_conf->gpu.numVShaders    = u32("GPU_NumVertexShaders", 8);
    arch_conf->gpu.numFShaders    = u32("GPU_NumFragmentShaders", 4);
    arch_conf->gpu.numStampUnits  = u32("GPU_NumStampPipes", 4);
    arch_conf->gpu.gpuClock       = u32("GPU_GPUClock", 500);
    arch_conf->gpu.shaderClock    = u32("GPU_ShaderClock", 500);
    arch_conf->gpu.memoryClock    = u32("GPU_MemoryClock", 500);

    // ===== [COMMANDPROCESSOR] =====
    arch_conf->com.pipelinedBatches   = b("COMMANDPROCESSOR_PipelinedBatchRendering", true);
    arch_conf->com.dumpShaderPrograms = b("COMMANDPROCESSOR_DumpShaderPrograms");

    // ===== [MEMORYCONTROLLER] =====
    arch_conf->mem.memSize          = u32("MEMORYCONTROLLER_MemorySize", 256);
    arch_conf->mem.clockMultiplier  = u32("MEMORYCONTROLLER_MemoryClockMultiplier", 1);
    arch_conf->mem.memoryFrequency  = u32("MEMORYCONTROLLER_MemoryFrequency", 1);
    arch_conf->mem.busWidth         = u32("MEMORYCONTROLLER_MemoryBusWidth", 64);
    arch_conf->mem.memBuses         = u32("MEMORYCONTROLLER_MemoryBuses", 4);
    arch_conf->mem.sharedBanks      = b("MEMORYCONTROLLER_SharedBanks");
    arch_conf->mem.bankGranurality  = u32("MEMORYCONTROLLER_BankGranurality", 1024);
    arch_conf->mem.burstLength      = u32("MEMORYCONTROLLER_BurstLength", 8);
    arch_conf->mem.readLatency      = u32("MEMORYCONTROLLER_ReadLatency", 10);
    arch_conf->mem.writeLatency     = u32("MEMORYCONTROLLER_WriteLatency", 5);
    arch_conf->mem.writeToReadLat = u32("MEMORYCONTROLLER_WriteToReadLatency", 5);
    arch_conf->mem.memPageSize      = u32("MEMORYCONTROLLER_MemoryPageSize", 4096);
    arch_conf->mem.openPages        = u32("MEMORYCONTROLLER_OpenPages", 8);
    arch_conf->mem.pageOpenLat  = u32("MEMORYCONTROLLER_PageOpenLatency", 13);
    arch_conf->mem.maxConsecutiveReads  = u32("MEMORYCONTROLLER_MaxConsecutiveReads", 16);
    arch_conf->mem.maxConsecutiveWrites = u32("MEMORYCONTROLLER_MaxConsecutiveWrites", 16);
    arch_conf->mem.comProcBus       = u32("MEMORYCONTROLLER_CommandProcessorBusWidth", 8);
    arch_conf->mem.strFetchBus      = u32("MEMORYCONTROLLER_StreamerFetchBusWidth", 64);
    arch_conf->mem.strLoaderBus     = u32("MEMORYCONTROLLER_StreamerLoaderBusWidth", 64);
    arch_conf->mem.zStencilBus      = u32("MEMORYCONTROLLER_ZStencilBusWidth", 64);
    arch_conf->mem.cWriteBus    = u32("MEMORYCONTROLLER_ColorWriteBusWidth", 64);
    arch_conf->mem.dacBus           = u32("MEMORYCONTROLLER_DisplayControllerBusWidth", 64);
    arch_conf->mem.textUnitBus      = u32("MEMORYCONTROLLER_TextureUnitBusWidth", 64);
    arch_conf->mem.mappedMemSize    = u32("MEMORYCONTROLLER_MappedMemorySize", 16);
    arch_conf->mem.readBufferLines  = u32("MEMORYCONTROLLER_ReadBufferLines", 256);
    arch_conf->mem.writeBufferLines = u32("MEMORYCONTROLLER_WriteBufferLines", 256);
    arch_conf->mem.reqQueueSz     = u32("MEMORYCONTROLLER_RequestQueueSize", 512);
    arch_conf->mem.servQueueSz = u32("MEMORYCONTROLLER_ServiceQueueSize", 64);
    // MCv2
    arch_conf->mem.memoryControllerV2 = b("MEMORYCONTROLLER_MemoryControllerV2", true);
    arch_conf->mem.v2MemoryChannels   = u32("MEMORYCONTROLLER_V2MemoryChannels", 8);
    arch_conf->mem.v2BanksPerMemoryChannel = u32("MEMORYCONTROLLER_V2BanksPerMemoryChannel", 8);
    arch_conf->mem.v2MemoryRowSize    = u32("MEMORYCONTROLLER_V2MemoryRowSize", 2048);
    arch_conf->mem.v2BurstBytesPerCycle = u32("MEMORYCONTROLLER_V2BurstBytesPerCycle", 8);
    arch_conf->mem.v2SplitterType     = u32("MEMORYCONTROLLER_V2SplitterType", 1);
    arch_conf->mem.v2ChannelInterleaving = u32("MEMORYCONTROLLER_V2ChannelInterleaving");
    arch_conf->mem.v2BankInterleaving   = u32("MEMORYCONTROLLER_V2BankInterleaving");
    arch_conf->mem.v2ChannelInterleavingMask = str_dup(s("MEMORYCONTROLLER_V2ChannelInterleavingMask"));
    arch_conf->mem.v2BankInterleavingMask = str_dup(s("MEMORYCONTROLLER_V2BankInterleavingMask"));
    arch_conf->mem.v2SecondInterleaving = b("MEMORYCONTROLLER_V2SecondInterleaving");
    arch_conf->mem.v2SecondChannelInterleaving = u32("MEMORYCONTROLLER_V2SecondChannelInterleaving");
    arch_conf->mem.v2SecondBankInterleaving = u32("MEMORYCONTROLLER_V2SecondBankInterleaving");
    arch_conf->mem.v2SecondChannelInterleavingMask = str_dup(s("MEMORYCONTROLLER_V2SecondChannelInterleavingMask"));
    arch_conf->mem.v2SecondBankInterleavingMask = str_dup(s("MEMORYCONTROLLER_V2SecondBankInterleavingMask"));
    arch_conf->mem.v2BankSelectionPolicy = str_dup(s("MEMORYCONTROLLER_V2BankSelectionPolicy"));
    arch_conf->mem.v2ChannelScheduler   = u32("MEMORYCONTROLLER_V2ChannelScheduler", 2);
    arch_conf->mem.v2PagePolicy         = u32("MEMORYCONTROLLER_V2PagePolicy");
    arch_conf->mem.v2MaxChannelTransactions = u32("MEMORYCONTROLLER_V2MaxChannelTransactions", 64);
    arch_conf->mem.v2DedicatedChannelReadTransactions = u32("MEMORYCONTROLLER_V2DedicatedChannelReadTransactions");
    arch_conf->mem.v2UseChannelRequestFIFOPerBank = b("MEMORYCONTROLLER_V2UseChannelRequestFIFOPerBank", true);
    arch_conf->mem.v2ChannelRequestFIFOPerBankSelection = u32("MEMORYCONTROLLER_V2ChannelRequestFIFOPerBankSelection", 1);
    arch_conf->mem.v2UseClassicSchedulerStates = b("MEMORYCONTROLLER_V2UseClassicSchedulerStates");
    arch_conf->mem.v2UseSplitRequestBufferPerROP = b("MEMORYCONTROLLER_V2UseSplitRequestBufferPerROP");
    arch_conf->mem.v2MemoryTrace = b("MEMORYCONTROLLER_V2MemoryTrace");
    arch_conf->mem.v2SwitchModePolicy = u32("MEMORYCONTROLLER_V2SwitchModePolicy", 1);
    arch_conf->mem.v2ActiveManagerMode = u32("MEMORYCONTROLLER_V2ActiveManagerMode", 1);
    arch_conf->mem.v2PrechargeManagerMode = u32("MEMORYCONTROLLER_V2PrechargeManagerMode");
    arch_conf->mem.v2ManagerSelectionAlgorithm = u32("MEMORYCONTROLLER_V2ManagerSelectionAlgorithm");
    arch_conf->mem.v2DisableActiveManager = b("MEMORYCONTROLLER_V2DisableActiveManager");
    arch_conf->mem.v2DisablePrechargeManager = b("MEMORYCONTROLLER_V2DisablePrechargeManager");
    arch_conf->mem.v2DebugString = str_dup(s("MEMORYCONTROLLER_V2DebugString"));
    arch_conf->mem.v2MemoryType = str_dup(s("MEMORYCONTROLLER_V2MemoryType"));
    arch_conf->mem.v2GDDR_Profile = str_dup(s("MEMORYCONTROLLER_V2GDDR_Profile"));
    arch_conf->mem.v2GDDR_tRRD = u32("MEMORYCONTROLLER_V2GDDR_tRRD", 9);
    arch_conf->mem.v2GDDR_tRCD = u32("MEMORYCONTROLLER_V2GDDR_tRCD", 13);
    arch_conf->mem.v2GDDR_tWTR = u32("MEMORYCONTROLLER_V2GDDR_tWTR", 5);
    arch_conf->mem.v2GDDR_tRTW = u32("MEMORYCONTROLLER_V2GDDR_tRTW", 2);
    arch_conf->mem.v2GDDR_tWR  = u32("MEMORYCONTROLLER_V2GDDR_tWR", 10);
    arch_conf->mem.v2GDDR_tRP  = u32("MEMORYCONTROLLER_V2GDDR_tRP", 14);
    arch_conf->mem.v2GDDR_CAS  = u32("MEMORYCONTROLLER_V2GDDR_CAS", 10);
    arch_conf->mem.v2GDDR_WL   = u32("MEMORYCONTROLLER_V2GDDR_WL", 5);

    // ===== [STREAMER] =====
    arch_conf->str.indicesCycle   = u32("STREAMER_IndicesCycle", 2);
    arch_conf->str.idxBufferSize     = u32("STREAMER_IndexBufferSize", 2048);
    arch_conf->str.outputFIFOSize = u32("STREAMER_OutputFIFOSize", 512);
    arch_conf->str.outputMemorySize  = u32("STREAMER_OutputMemorySize", 512);
    arch_conf->str.verticesCycle  = u32("STREAMER_VerticesCycle", 2);
    arch_conf->str.attrSentCycle  = u32("STREAMER_AttributesSentCycle", 4);
    arch_conf->str.streamerLoaderUnits = u32("STREAMER_StreamerLoaderUnits", 1);
    arch_conf->str.slIndicesCycle = u32("STREAMER_SLIndicesCycle", 2);
    arch_conf->str.slInputReqQueueSize = u32("STREAMER_SLInputRequestQueueSize", 128);
    arch_conf->str.slAttributesCycle = u32("STREAMER_SLAttributesCycle", 8);
    arch_conf->str.slInCacheLines = u32("STREAMER_SLInputCacheLines", 32);
    arch_conf->str.slInCacheLineSz = u32("STREAMER_SLInputCacheLineSize", 256);
    arch_conf->str.slInCachePortWidth = u32("STREAMER_SLInputCachePortWidth", 16);
    arch_conf->str.slInCacheReqQSz = u32("STREAMER_SLInputCacheRequestQueueSize", 8);
    arch_conf->str.slInCacheInputQSz = u32("STREAMER_SLInputCacheInputQueueSize", 8);

    // ===== [PRIMITIVEASSEMBLY] =====
    arch_conf->pas.verticesCycle     = u32("PRIMITIVEASSEMBLY_VerticesCycle", 2);
    arch_conf->pas.trianglesCycle    = u32("PRIMITIVEASSEMBLY_TrianglesCycle", 2);
    arch_conf->pas.inputBusLat       = u32("PRIMITIVEASSEMBLY_InputBusLatency", 10);
    arch_conf->pas.paQueueSize       = u32("PRIMITIVEASSEMBLY_AssemblyQueueSize", 32);

    // ===== [CLIPPER] =====
    arch_conf->clp.trianglesCycle    = u32("CLIPPER_TrianglesCycle", 2);
    arch_conf->clp.clipperUnits      = u32("CLIPPER_ClipperUnits", 2);
    arch_conf->clp.startLatency          = u32("CLIPPER_StartLatency", 1);
    arch_conf->clp.execLatency           = u32("CLIPPER_ExecLatency", 6);
    arch_conf->clp.clipBufferSize    = u32("CLIPPER_ClipBufferSize", 32);

    // ===== [RASTERIZER] =====
    arch_conf->ras.trianglesCycle    = u32("RASTERIZER_TrianglesCycle", 2);
    arch_conf->ras.setupFIFOSize     = u32("RASTERIZER_SetupFIFOSize", 32);
    arch_conf->ras.setupUnits        = u32("RASTERIZER_SetupUnits", 2);
    arch_conf->ras.setupLat          = u32("RASTERIZER_SetupLatency", 10);
    arch_conf->ras.setupStartLat     = u32("RASTERIZER_SetupStartLatency", 4);
    arch_conf->ras.trInputLat       = u32("RASTERIZER_TriangleInputLatency", 2);
    arch_conf->ras.trOutputLat      = u32("RASTERIZER_TriangleOutputLatency", 2);
    arch_conf->ras.shadedSetup     = b("RASTERIZER_TriangleSetupOnShader");
    arch_conf->ras.batchQueueSize     = u32("RASTERIZER_TriangleShaderQueueSize", 8);
    arch_conf->ras.stampsCycle        = u32("RASTERIZER_StampsPerCycle", 4);
    arch_conf->ras.samplesCycle   = u32("RASTERIZER_MSAASamplesCycle", 2);
    arch_conf->ras.overScanWidth      = u32("RASTERIZER_OverScanWidth", 4);
    arch_conf->ras.overScanHeight     = u32("RASTERIZER_OverScanHeight", 4);
    arch_conf->ras.scanWidth          = u32("RASTERIZER_ScanWidth", 16);
    arch_conf->ras.scanHeight         = u32("RASTERIZER_ScanHeight", 16);
    arch_conf->ras.genWidth           = u32("RASTERIZER_GenWidth", 8);
    arch_conf->ras.genHeight          = u32("RASTERIZER_GenHeight", 8);
    arch_conf->ras.rastBatch      = u32("RASTERIZER_RasterizationBatchSize", 4);
    arch_conf->ras.batchQueueSize     = u32("RASTERIZER_BatchQueueSize", 16);
    arch_conf->ras.recursive          = b("RASTERIZER_RecursiveMode", true);
    arch_conf->ras.disableHZ          = b("RASTERIZER_DisableHZ");
    arch_conf->ras.stampsHZBlock   = u32("RASTERIZER_StampsPerHZBlock", 16);
    arch_conf->ras.hzBufferSize       = u32("RASTERIZER_HierarchicalZBufferSize", 262144);
    arch_conf->ras.hzCacheLines       = u32("RASTERIZER_HZCacheLines", 8);
    arch_conf->ras.hzCacheLineSize    = u32("RASTERIZER_HZCacheLineSize", 16);
    arch_conf->ras.earlyZQueueSz    = u32("RASTERIZER_EarlyZQueueSize", 256);
    arch_conf->ras.hzAccessLatency        = u32("RASTERIZER_HZAccessLatency", 5);
    arch_conf->ras.hzUpdateLatency        = u32("RASTERIZER_HZUpdateLatency", 4);
    arch_conf->ras.hzBlockClearCycle  = u32("RASTERIZER_HZBlocksClearedPerCycle", 256);
    arch_conf->ras.numInterpolators   = u32("RASTERIZER_NumInterpolators", 4);
    arch_conf->ras.shInputQSize       = u32("RASTERIZER_ShaderInputQueueSize", 128);
    arch_conf->ras.shOutputQSize      = u32("RASTERIZER_ShaderOutputQueueSize", 128);
    arch_conf->ras.shInputBatchSize   = u32("RASTERIZER_ShaderInputBatchSize", 64);
    arch_conf->ras.tiledShDistro      = b("RASTERIZER_TiledShaderDistribution", true);
    arch_conf->ras.vInputQSize        = u32("RASTERIZER_VertexInputQueueSize", 32);
    arch_conf->ras.vShadedQSize       = u32("RASTERIZER_ShadedVertexQueueSize", 512);
    arch_conf->ras.trInputQSize    = u32("RASTERIZER_TriangleInputQueueSize", 32);
    arch_conf->ras.trOutputQSize   = u32("RASTERIZER_TriangleOutputQueueSize", 32);
    arch_conf->ras.genStampQSize      = u32("RASTERIZER_GeneratedStampQueueSize", 256);
    arch_conf->ras.testStampQSize    = u32("RASTERIZER_EarlyZTestedStampQueueSize", 32);
    arch_conf->ras.intStampQSize        = u32("RASTERIZER_InterpolatedStampQueueSize", 16);
    arch_conf->ras.shadedStampQSize   = u32("RASTERIZER_ShadedStampQueueSize", 640);
    arch_conf->ras.emuStoredTriangles = u32("RASTERIZER_EmulatorStoredTriangles", 64);
    // Micropolygon rasterizer
    arch_conf->ras.useMicroPolRast    = b("RASTERIZER_UseMicroPolygonRasterizer");
    arch_conf->ras.preBoundTriangles  = b("RASTERIZER_PreBoundTriangles", true);
    arch_conf->ras.cullMicroTriangles = b("RASTERIZER_CullMicroTriangles");
    arch_conf->ras.trBndOutLat     = u32("RASTERIZER_TriangleBoundOutputLatency", 2);
    arch_conf->ras.trBndOpLat      = u32("RASTERIZER_TriangleBoundOpLatency", 2);
    arch_conf->ras.largeTriMinSz   = u32("RASTERIZER_LargeTriangleFIFOSize", 64);
    arch_conf->ras.trBndMicroTriFIFOSz   = u32("RASTERIZER_MicroTriangleFIFOSize", 64);
    arch_conf->ras.useBBOptimization  = b("RASTERIZER_UseBoundingBoxOptimization", true);
    arch_conf->ras.subPixelPrecision  = u32("RASTERIZER_SubPixelPrecisionBits", 8);
    arch_conf->ras.largeTriMinSz    = static_cast<float>(f64("RASTERIZER_LargeTriangleMinSize", 16.0));
    arch_conf->ras.microTrisAsFragments = b("RASTERIZER_ProcessMicroTrianglesAsFragments", true);
    arch_conf->ras.microTriSzLimit    = u32("RASTERIZER_MicroTriangleSizeLimit");
    arch_conf->ras.microTriFlowPath   = str_dup(s("RASTERIZER_MicroTriangleFlowPath"));
    arch_conf->ras.dumpBurstHist   = b("RASTERIZER_DumpTriangleBurstSizeHistogram");

    // ===== [UNIFIEDSHADER] =====
    arch_conf->ush.numThreads     = u32("UNIFIEDSHADER_ExecutableThreads", 2048);
    arch_conf->ush.numInputBuffers = u32("UNIFIEDSHADER_InputBuffers", 16);
    arch_conf->ush.numResources   = u32("UNIFIEDSHADER_ThreadResources", 8192);
    arch_conf->ush.threadRate     = u32("UNIFIEDSHADER_ThreadRate", 4);
    arch_conf->ush.fetchRate      = u32("UNIFIEDSHADER_FetchRate", 2);
    arch_conf->ush.scalarALU      = b("UNIFIEDSHADER_ScalarALU", true);
    arch_conf->ush.threadGroup    = u32("UNIFIEDSHADER_ThreadGroup", 16);
    arch_conf->ush.lockedMode     = b("UNIFIEDSHADER_LockedExecutionMode", true);
    // Vector shader
    arch_conf->ush.useVectorShader   = b("UNIFIEDSHADER_VectorShader", true);
    arch_conf->ush.vectorThreads  = u32("UNIFIEDSHADER_VectorThreads", 128);
    arch_conf->ush.vectorResources = u32("UNIFIEDSHADER_VectorResources", 512);
    arch_conf->ush.vectorLength   = u32("UNIFIEDSHADER_VectorLength", 16);
    arch_conf->ush.vectorALUWidth = u32("UNIFIEDSHADER_VectorALUWidth", 4);
    arch_conf->ush.vectorALUConfig = str_dup(s("UNIFIEDSHADER_VectorALUConfig"));
    arch_conf->ush.vectorWaitOnStall = b("UNIFIEDSHADER_VectorWaitOnStall");
    arch_conf->ush.vectorExplicitBlock = b("UNIFIEDSHADER_VectorExplicitBlock");
    // Common shader
    arch_conf->ush.vAttrLoadFromShader = b("UNIFIEDSHADER_VertexAttributeLoadFromShader");
    arch_conf->ush.threadWindow   = b("UNIFIEDSHADER_ThreadWindow", true);
    arch_conf->ush.fetchDelay     = u32("UNIFIEDSHADER_FetchDelay", 4);
    arch_conf->ush.swapOnBlock    = b("UNIFIEDSHADER_SwapOnBlock");
    arch_conf->ush.fixedLatencyALU    = b("UNIFIEDSHADER_FixedLatencyALU");
    arch_conf->ush.inputsCycle    = u32("UNIFIEDSHADER_InputsPerCycle", 4);
    arch_conf->ush.outputsCycle   = u32("UNIFIEDSHADER_OutputsPerCycle", 4);
    arch_conf->ush.outputLatency  = u32("UNIFIEDSHADER_OutputLatency", 11);
    arch_conf->ush.textureUnits   = u32("UNIFIEDSHADER_TextureUnits", 1);
    arch_conf->ush.textRequestRate = u32("UNIFIEDSHADER_TextureRequestRate", 1);
    arch_conf->ush.textRequestGroup = u32("UNIFIEDSHADER_TextureRequestGroup", 64);
    // Texture unit
    arch_conf->ush.addressALULat  = u32("UNIFIEDSHADER_AddressALULatency", 6);
    arch_conf->ush.filterLat   = u32("UNIFIEDSHADER_FilterALULatency", 4);
    arch_conf->ush.texBlockDim    = u32("UNIFIEDSHADER_TextureBlockDimension", 2);
    arch_conf->ush.texSuperBlockDim = u32("UNIFIEDSHADER_TextureSuperBlockDimension", 4);
    arch_conf->ush.textReqQSize   = u32("UNIFIEDSHADER_TextureRequestQueueSize", 512);
    arch_conf->ush.textAccessQSize = u32("UNIFIEDSHADER_TextureAccessQueue", 256);
    arch_conf->ush.textResultQSize = u32("UNIFIEDSHADER_TextureResultQueue", 4);
    arch_conf->ush.textWaitWSize = u32("UNIFIEDSHADER_TextureWaitReadWindow", 32);
    arch_conf->ush.twoLevelCache  = b("UNIFIEDSHADER_TwoLevelTextureCache", true);
    arch_conf->ush.txCacheLineSz = u32("UNIFIEDSHADER_TextureCacheLineSize", 64);
    arch_conf->ush.txCacheWays  = u32("UNIFIEDSHADER_TextureCacheWays", 8);
    arch_conf->ush.txCacheLines = u32("UNIFIEDSHADER_TextureCacheLines", 8);
    arch_conf->ush.txCachePortWidth = u32("UNIFIEDSHADER_TextureCachePortWidth", 4);
    arch_conf->ush.txCacheReqQSz = u32("UNIFIEDSHADER_TextureCacheRequestQueueSize", 32);
    arch_conf->ush.txCacheInQSz = u32("UNIFIEDSHADER_TextureCacheInputQueue", 32);
    arch_conf->ush.txCacheMissCycle = u32("UNIFIEDSHADER_TextureCacheMissesPerCycle", 8);
    arch_conf->ush.txCacheDecomprLatency = u32("UNIFIEDSHADER_TextureCacheDecompressLatency", 1);
    arch_conf->ush.txCacheLineSzL1 = u32("UNIFIEDSHADER_TextureCacheLineSizeL1", 64);
    arch_conf->ush.txCacheWaysL1 = u32("UNIFIEDSHADER_TextureCacheWaysL1", 16);
    arch_conf->ush.txCacheLinesL1 = u32("UNIFIEDSHADER_TextureCacheLinesL1", 16);
    arch_conf->ush.txCacheInQSzL1 = u32("UNIFIEDSHADER_TextureCacheInputQueueL1", 32);
    arch_conf->ush.anisoAlgo      = u32("UNIFIEDSHADER_AnisotropyAlgorithm");
    arch_conf->ush.forceMaxAniso  = b("UNIFIEDSHADER_ForceMaxAnisotropy");
    arch_conf->ush.maxAnisotropy  = u32("UNIFIEDSHADER_MaxAnisotropy", 16);
    arch_conf->ush.triPrecision  = u32("UNIFIEDSHADER_TrilinearPrecision", 8);
    arch_conf->ush.briThreshold = u32("UNIFIEDSHADER_BrilinearThreshold");
    arch_conf->ush.anisoRoundPrec = u32("UNIFIEDSHADER_AnisoRoundPrecision", 8);
    arch_conf->ush.anisoRoundThres = u32("UNIFIEDSHADER_AnisoRoundThreshold");
    arch_conf->ush.anisoRatioMultOf2 = b("UNIFIEDSHADER_AnisoRatioMultOfTwo");

    // ===== [ZSTENCILTEST] =====
    arch_conf->zst.stampsCycle    = u32("ZSTENCILTEST_StampsPerCycle", 1);
    arch_conf->zst.bytesPixel     = u32("ZSTENCILTEST_BytesPerPixel", 4);
    arch_conf->zst.disableCompr = b("ZSTENCILTEST_DisableCompression");
    arch_conf->zst.zCacheWays     = u32("ZSTENCILTEST_ZCacheWays", 4);
    arch_conf->zst.zCacheLines    = u32("ZSTENCILTEST_ZCacheLines", 16);
    arch_conf->zst.zCacheLineStamps = u32("ZSTENCILTEST_ZCacheStampsPerLine", 16);
    arch_conf->zst.zCachePortWidth = u32("ZSTENCILTEST_ZCachePortWidth", 32);
    arch_conf->zst.extraReadPort = b("ZSTENCILTEST_ZCacheExtraReadPort", true);
    arch_conf->zst.extraWritePort = b("ZSTENCILTEST_ZCacheExtraWritePort", true);
    arch_conf->zst.zCacheReqQSz = u32("ZSTENCILTEST_ZCacheRequestQueueSize", 8);
    arch_conf->zst.zCacheInQSz  = u32("ZSTENCILTEST_ZCacheInputQueueSize", 8);
    arch_conf->zst.zCacheOutQSz = u32("ZSTENCILTEST_ZCacheOutputQueueSize", 8);
    arch_conf->zst.blockStateMemSz = u32("ZSTENCILTEST_BlockStateMemorySize", 262144);
    arch_conf->zst.blockClearCycle = u32("ZSTENCILTEST_BlocksClearedPerCycle", 1024);
    arch_conf->zst.comprLatency   = u32("ZSTENCILTEST_CompressionUnitLatency", 8);
    arch_conf->zst.decomprLatency = u32("ZSTENCILTEST_DecompressionUnitLatency", 8);
    arch_conf->zst.inputQueueSize     = u32("ZSTENCILTEST_InputQueueSize", 8);
    arch_conf->zst.fetchQueueSize     = u32("ZSTENCILTEST_FetchQueueSize", 256);
    arch_conf->zst.readQueueSize      = u32("ZSTENCILTEST_ReadQueueSize", 16);
    arch_conf->zst.opQueueSize        = u32("ZSTENCILTEST_OpQueueSize", 4);
    arch_conf->zst.writeQueueSize     = u32("ZSTENCILTEST_WriteQueueSize", 8);
    arch_conf->zst.zTestRate      = u32("ZSTENCILTEST_ZALUTestRate", 1);
    arch_conf->zst.zOpLatency   = u32("ZSTENCILTEST_ZALULatency", 2);
    arch_conf->zst.comprAlgo = u32("ZSTENCILTEST_CompressionAlgorithm");

    // ===== [COLORWRITE] =====
    arch_conf->cwr.stampsCycle    = u32("COLORWRITE_StampsPerCycle", 1);
    arch_conf->cwr.bytesPixel     = u32("COLORWRITE_BytesPerPixel", 4);
    arch_conf->cwr.disableCompr = b("COLORWRITE_DisableCompression");
    arch_conf->cwr.cCacheWays = u32("COLORWRITE_ColorCacheWays", 4);
    arch_conf->cwr.cCacheLines = u32("COLORWRITE_ColorCacheLines", 16);
    arch_conf->cwr.cCacheLineStamps = u32("COLORWRITE_ColorCacheStampsPerLine", 16);
    arch_conf->cwr.cCachePortWidth = u32("COLORWRITE_ColorCachePortWidth", 32);
    arch_conf->cwr.extraReadPort = b("COLORWRITE_ColorCacheExtraReadPort", true);
    arch_conf->cwr.extraWritePort = b("COLORWRITE_ColorCacheExtraWritePort", true);
    arch_conf->cwr.cCacheReqQSz = u32("COLORWRITE_ColorCacheRequestQueueSize", 8);
    arch_conf->cwr.cCacheInQSz = u32("COLORWRITE_ColorCacheInputQueueSize", 8);
    arch_conf->cwr.cCacheOutQSz = u32("COLORWRITE_ColorCacheOutputQueueSize", 8);
    arch_conf->cwr.blockStateMemSz = u32("COLORWRITE_BlockStateMemorySize", 262144);
    arch_conf->cwr.blockClearCycle = u32("COLORWRITE_BlocksClearedPerCycle", 1024);
    arch_conf->cwr.comprLatency   = u32("COLORWRITE_CompressionUnitLatency", 8);
    arch_conf->cwr.decomprLatency = u32("COLORWRITE_DecompressionUnitLatency", 8);
    arch_conf->cwr.inputQueueSize     = u32("COLORWRITE_InputQueueSize", 8);
    arch_conf->cwr.fetchQueueSize     = u32("COLORWRITE_FetchQueueSize", 256);
    arch_conf->cwr.readQueueSize      = u32("COLORWRITE_ReadQueueSize", 16);
    arch_conf->cwr.opQueueSize        = u32("COLORWRITE_OpQueueSize", 4);
    arch_conf->cwr.writeQueueSize     = u32("COLORWRITE_WriteQueueSize", 8);
    arch_conf->cwr.blendRate      = u32("COLORWRITE_BlendALURate", 1);
    arch_conf->cwr.blendOpLatency   = u32("COLORWRITE_BlendALULatency", 2);
    arch_conf->cwr.comprAlgo = u32("COLORWRITE_CompressionAlgorithm");

    // ===== [DISPLAYCONTROLLER] =====
    arch_conf->dac.bytesPixel     = u32("DISPLAYCONTROLLER_BytesPerPixel", 4);
    arch_conf->dac.blockSize      = u32("DISPLAYCONTROLLER_BlockSize", 256);
    arch_conf->dac.blockUpdateLat = u32("DISPLAYCONTROLLER_BlockUpdateLatency", 1);
    arch_conf->dac.blocksCycle    = u32("DISPLAYCONTROLLER_BlocksUpdatedPerCycle", 1024);
    arch_conf->dac.blockReqQSz  = u32("DISPLAYCONTROLLER_BlockRequestQueueSize", 32);
    arch_conf->dac.decomprLatency = u32("DISPLAYCONTROLLER_DecompressionUnitLatency", 1);
    arch_conf->dac.refreshRate    = u32("DISPLAYCONTROLLER_RefreshRate", 5000000);
    arch_conf->dac.synchedRefresh = b("DISPLAYCONTROLLER_SynchedRefresh", true);
    arch_conf->dac.refreshFrame   = b("DISPLAYCONTROLLER_RefreshFrame", true);
    arch_conf->dac.saveBlitSource = b("DISPLAYCONTROLLER_SaveBlitSourceData");

    // Signals not handled via CSV — set to null / zero
    arch_conf->sig = nullptr;
    arch_conf->numSignals = 0;
}
