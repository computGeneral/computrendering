#include "param_loader.hpp"
#include "archParams.h"   // for cgsArchConfig and nested structs
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
    S("SIMULATOR_INPUT_FILE",           a.sim.inputFile);
    U6("SIMULATOR_SIM_CYCLES",          a.sim.simCycles);
    U("SIMULATOR_SIM_FRAMES",           a.sim.simFrames);
    S("SIMULATOR_SIGNAL_DUMP_FILE",      a.sim.signalDumpFile);
    S("SIMULATOR_STATS_FILE",           a.sim.statsFile);
    S("SIMULATOR_STATS_FILE_PER_FRAME",   a.sim.statsFilePerFrame);
    S("SIMULATOR_STATS_FILE_PER_BATCH",   a.sim.statsFilePerBatch);
    U("SIMULATOR_START_FRAME",         a.sim.startFrame);
    U6("SIMULATOR_START_SIGNAL_DUMP",    a.sim.startDump);
    U6("SIMULATOR_SIGNAL_DUMP_CYCLES",   a.sim.dumpCycles);
    U("SIMULATOR_STATISTICS_RATE",      a.sim.statsRate);
    B("SIMULATOR_DUMP_SIGNAL_TRACE",     a.sim.dumpSignalTrace);
    B("SIMULATOR_STATISTICS",          a.sim.statistics);
    B("SIMULATOR_PER_CYCLE_STATISTICS",  a.sim.perCycleStatistics);
    B("SIMULATOR_PER_FRAME_STATISTICS",  a.sim.perFrameStatistics);
    B("SIMULATOR_PER_BATCH_STATISTICS",  a.sim.perBatchStatistics);
    B("SIMULATOR_DETECT_STALLS",        a.sim.detectStalls);
    B("SIMULATOR_GENERATE_FRAGMENT_MAP", a.sim.fragmentMap);
    U("SIMULATOR_FRAGMENT_MAP_MODE",     a.sim.fragmentMapMode);
    B("SIMULATOR_DOUBLE_BUFFER",        a.sim.colorDoubleBuffer);
    B("SIMULATOR_FORCE_MSAA",           a.sim.forceMSAA);
    U("SIMULATOR_MSAA_SAMPLES",         a.sim.msaaSamples);
    B("SIMULATOR_FORCE_FP16_COLOR_BUFFER",a.sim.forceFP16ColorBuffer);
    B("SIMULATOR_ENABLE_DRIVER_SHADER_TRANSLATION", a.sim.enableDriverShaderTranslation);
    U("SIMULATOR_OBJECT_SIZE0",         a.sim.objectSize0);
    U("SIMULATOR_BUCKET_SIZE0",         a.sim.bucketSize0);
    U("SIMULATOR_OBJECT_SIZE1",         a.sim.objectSize1);
    U("SIMULATOR_BUCKET_SIZE1",         a.sim.bucketSize1);
    U("SIMULATOR_OBJECT_SIZE2",         a.sim.objectSize2);
    U("SIMULATOR_BUCKET_SIZE2",         a.sim.bucketSize2);

    // [GPU]
    U("GPU_NUM_VERTEX_SHADERS",          a.gpu.numVShaders);
    U("GPU_NUM_FRAGMENT_SHADERS",        a.gpu.numFShaders);
    U("GPU_NUM_STAMP_PIPES",             a.gpu.numStampUnits);
    U("GPU_GPU_CLOCK",                  a.gpu.gpuClock);
    U("GPU_SHADER_CLOCK",               a.gpu.shaderClock);
    U("GPU_MEMORY_CLOCK",               a.gpu.memoryClock);

    // [COMMANDPROCESSOR]
    B("COMMANDPROCESSOR_PIPELINED_BATCH_RENDERING", a.com.pipelinedBatches);
    B("COMMANDPROCESSOR_DUMP_SHADER_PROGRAMS",      a.com.dumpShaderPrograms);

    // [MEMORYCONTROLLER]
    U("MEMORYCONTROLLER_MEMORY_SIZE",            a.mem.memSize);
    U("MEMORYCONTROLLER_MEMORY_CLOCK_MULTIPLIER", a.mem.clockMultiplier);
    U("MEMORYCONTROLLER_MEMORY_FREQUENCY",       a.mem.memoryFrequency);
    U("MEMORYCONTROLLER_MEMORY_BUS_WIDTH",        a.mem.busWidth);
    U("MEMORYCONTROLLER_MEMORY_BUSES",           a.mem.memBuses);
    B("MEMORYCONTROLLER_SHARED_BANKS",           a.mem.sharedBanks);
    U("MEMORYCONTROLLER_BANK_GRANURALITY",       a.mem.bankGranurality);
    U("MEMORYCONTROLLER_BURST_LENGTH",           a.mem.burstLength);
    U("MEMORYCONTROLLER_READ_LATENCY",           a.mem.readLatency);
    U("MEMORYCONTROLLER_WRITE_LATENCY",          a.mem.writeLatency);
    U("MEMORYCONTROLLER_WRITE_TO_READ_LATENCY",    a.mem.writeToReadLat);
    U("MEMORYCONTROLLER_MEMORY_PAGE_SIZE",        a.mem.memPageSize);
    U("MEMORYCONTROLLER_OPEN_PAGES",             a.mem.openPages);
    U("MEMORYCONTROLLER_PAGE_OPEN_LATENCY",       a.mem.pageOpenLat);
    U("MEMORYCONTROLLER_MAX_CONSECUTIVE_READS",   a.mem.maxConsecutiveReads);
    U("MEMORYCONTROLLER_MAX_CONSECUTIVE_WRITES",  a.mem.maxConsecutiveWrites);
    U("MEMORYCONTROLLER_COMMAND_PROCESSOR_BUS_WIDTH", a.mem.comProcBus);
    U("MEMORYCONTROLLER_STREAMER_FETCH_BUS_WIDTH",    a.mem.strFetchBus);
    U("MEMORYCONTROLLER_STREAMER_LOADER_BUS_WIDTH",   a.mem.strLoaderBus);
    U("MEMORYCONTROLLER_Z_STENCIL_BUS_WIDTH",         a.mem.zStencilBus);
    U("MEMORYCONTROLLER_COLOR_WRITE_BUS_WIDTH",       a.mem.cWriteBus);
    U("MEMORYCONTROLLER_DISPLAY_CONTROLLER_BUS_WIDTH",a.mem.dacBus);
    U("MEMORYCONTROLLER_TEXTURE_UNIT_BUS_WIDTH",      a.mem.textUnitBus);
    U("MEMORYCONTROLLER_MAPPED_MEMORY_SIZE",         a.mem.mappedMemSize);
    U("MEMORYCONTROLLER_READ_BUFFER_LINES",          a.mem.readBufferLines);
    U("MEMORYCONTROLLER_WRITE_BUFFER_LINES",         a.mem.writeBufferLines);
    U("MEMORYCONTROLLER_REQUEST_QUEUE_SIZE",         a.mem.reqQueueSz);
    U("MEMORYCONTROLLER_SERVICE_QUEUE_SIZE",         a.mem.servQueueSz);
    B("MEMORYCONTROLLER_MEMORY_CONTROLLER_V2",       a.mem.memoryControllerV2);
    U("MEMORYCONTROLLER_V2_MEMORY_CHANNELS",         a.mem.v2MemoryChannels);
    U("MEMORYCONTROLLER_V2_BANKS_PER_MEMORY_CHANNEL",  a.mem.v2BanksPerMemoryChannel);
    U("MEMORYCONTROLLER_V2_MEMORY_ROW_SIZE",          a.mem.v2MemoryRowSize);
    U("MEMORYCONTROLLER_V2_BURST_BYTES_PER_CYCLE",     a.mem.v2BurstBytesPerCycle);
    U("MEMORYCONTROLLER_V2_SPLITTER_TYPE",           a.mem.v2SplitterType);
    U("MEMORYCONTROLLER_V2_CHANNEL_INTERLEAVING",    a.mem.v2ChannelInterleaving);
    U("MEMORYCONTROLLER_V2_BANK_INTERLEAVING",       a.mem.v2BankInterleaving);
    S("MEMORYCONTROLLER_V2_CHANNEL_INTERLEAVING_MASK",a.mem.v2ChannelInterleavingMask);
    S("MEMORYCONTROLLER_V2_BANK_INTERLEAVING_MASK",   a.mem.v2BankInterleavingMask);
    B("MEMORYCONTROLLER_V2_SECOND_INTERLEAVING",     a.mem.v2SecondInterleaving);
    U("MEMORYCONTROLLER_V2_SECOND_CHANNEL_INTERLEAVING", a.mem.v2SecondChannelInterleaving);
    U("MEMORYCONTROLLER_V2_SECOND_BANK_INTERLEAVING",    a.mem.v2SecondBankInterleaving);
    S("MEMORYCONTROLLER_V2_SECOND_CHANNEL_INTERLEAVING_MASK", a.mem.v2SecondChannelInterleavingMask);
    S("MEMORYCONTROLLER_V2_SECOND_BANK_INTERLEAVING_MASK",    a.mem.v2SecondBankInterleavingMask);
    S("MEMORYCONTROLLER_V2_BANK_SELECTION_POLICY",    a.mem.v2BankSelectionPolicy);
    U("MEMORYCONTROLLER_V2_CHANNEL_SCHEDULER",       a.mem.v2ChannelScheduler);
    U("MEMORYCONTROLLER_V2_PAGE_POLICY",             a.mem.v2PagePolicy);
    U("MEMORYCONTROLLER_V2_MAX_CHANNEL_TRANSACTIONS",           a.mem.v2MaxChannelTransactions);
    U("MEMORYCONTROLLER_V2_DEDICATED_CHANNEL_READ_TRANSACTIONS", a.mem.v2DedicatedChannelReadTransactions);
    B("MEMORYCONTROLLER_V2_USE_CHANNEL_REQUEST_FIFO_PER_BANK",     a.mem.v2UseChannelRequestFIFOPerBank);
    U("MEMORYCONTROLLER_V2_CHANNEL_REQUEST_FIFO_PER_BANK_SELECTION", a.mem.v2ChannelRequestFIFOPerBankSelection);
    B("MEMORYCONTROLLER_V2_USE_CLASSIC_SCHEDULER_STATES",   a.mem.v2UseClassicSchedulerStates);
    B("MEMORYCONTROLLER_V2_USE_SPLIT_REQUEST_BUFFER_PER_ROP", a.mem.v2UseSplitRequestBufferPerROP);
    B("MEMORYCONTROLLER_V2_MEMORY_TRACE",            a.mem.v2MemoryTrace);
    U("MEMORYCONTROLLER_V2_SWITCH_MODE_POLICY",       a.mem.v2SwitchModePolicy);
    U("MEMORYCONTROLLER_V2_ACTIVE_MANAGER_MODE",      a.mem.v2ActiveManagerMode);
    U("MEMORYCONTROLLER_V2_PRECHARGE_MANAGER_MODE",   a.mem.v2PrechargeManagerMode);
    U("MEMORYCONTROLLER_V2_MANAGER_SELECTION_ALGORITHM", a.mem.v2ManagerSelectionAlgorithm);
    B("MEMORYCONTROLLER_V2_DISABLE_ACTIVE_MANAGER",   a.mem.v2DisableActiveManager);
    B("MEMORYCONTROLLER_V2_DISABLE_PRECHARGE_MANAGER",a.mem.v2DisablePrechargeManager);
    S("MEMORYCONTROLLER_V2_DEBUG_STRING",            a.mem.v2DebugString);
    S("MEMORYCONTROLLER_V2_MEMORY_TYPE",             a.mem.v2MemoryType);
    S("MEMORYCONTROLLER_V2_GDDR_PROFILE",           a.mem.v2GDDR_Profile);
    U("MEMORYCONTROLLER_V2_GDDR_T_RRD",  a.mem.v2GDDR_tRRD);
    U("MEMORYCONTROLLER_V2_GDDR_T_RCD",  a.mem.v2GDDR_tRCD);
    U("MEMORYCONTROLLER_V2_GDDR_T_WTR",  a.mem.v2GDDR_tWTR);
    U("MEMORYCONTROLLER_V2_GDDR_T_RTW",  a.mem.v2GDDR_tRTW);
    U("MEMORYCONTROLLER_V2_GDDR_T_WR",   a.mem.v2GDDR_tWR);
    U("MEMORYCONTROLLER_V2_GDDR_T_RP",   a.mem.v2GDDR_tRP);
    U("MEMORYCONTROLLER_V2_GDDR_CAS",   a.mem.v2GDDR_CAS);
    U("MEMORYCONTROLLER_V2_GDDR_WL",    a.mem.v2GDDR_WL);

    // [STREAMER]
    U("STREAMER_INDICES_CYCLE",            a.str.indicesCycle);
    U("STREAMER_INDEX_BUFFER_SIZE",         a.str.idxBufferSize);
    U("STREAMER_OUTPUT_FIFO_SIZE",          a.str.outputFIFOSize);
    U("STREAMER_OUTPUT_MEMORY_SIZE",        a.str.outputMemorySize);
    U("STREAMER_VERTICES_CYCLE",           a.str.verticesCycle);
    U("STREAMER_ATTRIBUTES_SENT_CYCLE",     a.str.attrSentCycle);
    U("STREAMER_STREAMER_LOADER_UNITS",     a.str.streamerLoaderUnits);
    U("STREAMER_SL_INDICES_CYCLE",          a.str.slIndicesCycle);
    U("STREAMER_SL_INPUT_REQUEST_QUEUE_SIZE", a.str.slInputReqQueueSize);
    U("STREAMER_SL_ATTRIBUTES_CYCLE",       a.str.slAttributesCycle);
    U("STREAMER_SL_INPUT_CACHE_LINES",       a.str.slInCacheLines);
    U("STREAMER_SL_INPUT_CACHE_LINE_SIZE",    a.str.slInCacheLineSz);
    U("STREAMER_SL_INPUT_CACHE_PORT_WIDTH",   a.str.slInCachePortWidth);
    U("STREAMER_SL_INPUT_CACHE_REQUEST_QUEUE_SIZE", a.str.slInCacheReqQSz);
    U("STREAMER_SL_INPUT_CACHE_INPUT_QUEUE_SIZE",   a.str.slInCacheInputQSz);

    // [PRIMITIVEASSEMBLY]
    U("PRIMITIVEASSEMBLY_VERTICES_CYCLE",      a.pas.verticesCycle);
    U("PRIMITIVEASSEMBLY_TRIANGLES_CYCLE",     a.pas.trianglesCycle);
    U("PRIMITIVEASSEMBLY_INPUT_BUS_LATENCY",    a.pas.inputBusLat);
    U("PRIMITIVEASSEMBLY_ASSEMBLY_QUEUE_SIZE",  a.pas.paQueueSize);

    // [CLIPPER]
    U("CLIPPER_TRIANGLES_CYCLE",  a.clp.trianglesCycle);
    U("CLIPPER_CLIPPER_UNITS",    a.clp.clipperUnits);
    U("CLIPPER_START_LATENCY",    a.clp.startLatency);
    U("CLIPPER_EXEC_LATENCY",     a.clp.execLatency);
    U("CLIPPER_CLIP_BUFFER_SIZE",  a.clp.clipBufferSize);

    // [RASTERIZER]
    U("RASTERIZER_TRIANGLES_CYCLE",      a.ras.trianglesCycle);
    U("RASTERIZER_SETUP_FIFO_SIZE",       a.ras.setupFIFOSize);
    U("RASTERIZER_SETUP_UNITS",          a.ras.setupUnits);
    U("RASTERIZER_SETUP_LATENCY",        a.ras.setupLat);
    U("RASTERIZER_SETUP_START_LATENCY",   a.ras.setupStartLat);
    U("RASTERIZER_TRIANGLE_INPUT_LATENCY",  a.ras.trInputLat);
    U("RASTERIZER_TRIANGLE_OUTPUT_LATENCY", a.ras.trOutputLat);
    B("RASTERIZER_TRIANGLE_SETUP_ON_SHADER", a.ras.shadedSetup);
    U("RASTERIZER_TRIANGLE_SHADER_QUEUE_SIZE", a.ras.batchQueueSize);
    U("RASTERIZER_STAMPS_PER_CYCLE",       a.ras.stampsCycle);
    U("RASTERIZER_MSAA_SAMPLES_CYCLE",     a.ras.samplesCycle);
    U("RASTERIZER_OVER_SCAN_WIDTH",        a.ras.overScanWidth);
    U("RASTERIZER_OVER_SCAN_HEIGHT",       a.ras.overScanHeight);
    U("RASTERIZER_SCAN_WIDTH",            a.ras.scanWidth);
    U("RASTERIZER_SCAN_HEIGHT",           a.ras.scanHeight);
    U("RASTERIZER_GEN_WIDTH",             a.ras.genWidth);
    U("RASTERIZER_GEN_HEIGHT",            a.ras.genHeight);
    U("RASTERIZER_RASTERIZATION_BATCH_SIZE", a.ras.rastBatch);
    U("RASTERIZER_BATCH_QUEUE_SIZE",       a.ras.batchQueueSize);
    B("RASTERIZER_RECURSIVE_MODE",        a.ras.recursive);
    B("RASTERIZER_DISABLE_HZ",            a.ras.disableHZ);
    U("RASTERIZER_STAMPS_PER_HZ_BLOCK",     a.ras.stampsHZBlock);
    U("RASTERIZER_HIERARCHICAL_Z_BUFFER_SIZE", a.ras.hzBufferSize);
    U("RASTERIZER_HZ_CACHE_LINES",         a.ras.hzCacheLines);
    U("RASTERIZER_HZ_CACHE_LINE_SIZE",      a.ras.hzCacheLineSize);
    U("RASTERIZER_EARLY_Z_QUEUE_SIZE",      a.ras.earlyZQueueSz);
    U("RASTERIZER_HZ_ACCESS_LATENCY",      a.ras.hzAccessLatency);
    U("RASTERIZER_HZ_UPDATE_LATENCY",      a.ras.hzUpdateLatency);
    U("RASTERIZER_HZ_BLOCKS_CLEARED_PER_CYCLE", a.ras.hzBlockClearCycle);
    U("RASTERIZER_NUM_INTERPOLATORS",     a.ras.numInterpolators);
    U("RASTERIZER_SHADER_INPUT_QUEUE_SIZE", a.ras.shInputQSize);
    U("RASTERIZER_SHADER_OUTPUT_QUEUE_SIZE",a.ras.shOutputQSize);
    U("RASTERIZER_SHADER_INPUT_BATCH_SIZE", a.ras.shInputBatchSize);
    B("RASTERIZER_TILED_SHADER_DISTRIBUTION", a.ras.tiledShDistro);
    U("RASTERIZER_VERTEX_INPUT_QUEUE_SIZE", a.ras.vInputQSize);
    U("RASTERIZER_SHADED_VERTEX_QUEUE_SIZE",a.ras.vShadedQSize);
    U("RASTERIZER_TRIANGLE_INPUT_QUEUE_SIZE",  a.ras.trInputQSize);
    U("RASTERIZER_TRIANGLE_OUTPUT_QUEUE_SIZE", a.ras.trOutputQSize);
    U("RASTERIZER_GENERATED_STAMP_QUEUE_SIZE",   a.ras.genStampQSize);
    U("RASTERIZER_EARLY_Z_TESTED_STAMP_QUEUE_SIZE",a.ras.testStampQSize);
    U("RASTERIZER_INTERPOLATED_STAMP_QUEUE_SIZE",a.ras.intStampQSize);
    U("RASTERIZER_SHADED_STAMP_QUEUE_SIZE",      a.ras.shadedStampQSize);
    U("RASTERIZER_EMULATOR_STORED_TRIANGLES",   a.ras.emuStoredTriangles);
    B("RASTERIZER_USE_MICRO_POLYGON_RASTERIZER", a.ras.useMicroPolRast);
    B("RASTERIZER_PRE_BOUND_TRIANGLES",    a.ras.preBoundTriangles);
    B("RASTERIZER_CULL_MICRO_TRIANGLES",   a.ras.cullMicroTriangles);
    U("RASTERIZER_TRIANGLE_BOUND_OUTPUT_LATENCY", a.ras.trBndOutLat);
    U("RASTERIZER_TRIANGLE_BOUND_OP_LATENCY",     a.ras.trBndOpLat);
    U("RASTERIZER_LARGE_TRIANGLE_FIFO_SIZE",      a.ras.largeTriMinSz);
    U("RASTERIZER_MICRO_TRIANGLE_FIFO_SIZE",      a.ras.trBndMicroTriFIFOSz);
    B("RASTERIZER_USE_BOUNDING_BOX_OPTIMIZATION",  a.ras.useBBOptimization);
    U("RASTERIZER_SUB_PIXEL_PRECISION_BITS",       a.ras.subPixelPrecision);
    F("RASTERIZER_LARGE_TRIANGLE_MIN_SIZE",        a.ras.largeTriMinSz);
    B("RASTERIZER_PROCESS_MICRO_TRIANGLES_AS_FRAGMENTS", a.ras.microTrisAsFragments);
    U("RASTERIZER_MICRO_TRIANGLE_SIZE_LIMIT",      a.ras.microTriSzLimit);
    S("RASTERIZER_MICRO_TRIANGLE_FLOW_PATH",       a.ras.microTriFlowPath);
    B("RASTERIZER_DUMP_TRIANGLE_BURST_SIZE_HISTOGRAM", a.ras.dumpBurstHist);

    // [UNIFIEDSHADER]
    U("UNIFIEDSHADER_EXECUTABLE_THREADS",  a.ush.numThreads);
    U("UNIFIEDSHADER_INPUT_BUFFERS",       a.ush.numInputBuffers);
    U("UNIFIEDSHADER_THREAD_RESOURCES",    a.ush.numResources);
    U("UNIFIEDSHADER_THREAD_RATE",         a.ush.threadRate);
    U("UNIFIEDSHADER_FETCH_RATE",          a.ush.fetchRate);
    B("UNIFIEDSHADER_SCALAR_ALU",          a.ush.scalarALU);
    U("UNIFIEDSHADER_THREAD_GROUP",        a.ush.threadGroup);
    B("UNIFIEDSHADER_LOCKED_EXECUTION_MODE",a.ush.lockedMode);
    B("UNIFIEDSHADER_VECTOR_SHADER",       a.ush.useVectorShader);
    U("UNIFIEDSHADER_VECTOR_THREADS",      a.ush.vectorThreads);
    U("UNIFIEDSHADER_VECTOR_RESOURCES",    a.ush.vectorResources);
    U("UNIFIEDSHADER_VECTOR_LENGTH",       a.ush.vectorLength);
    U("UNIFIEDSHADER_VECTOR_ALU_WIDTH",     a.ush.vectorALUWidth);
    S("UNIFIEDSHADER_VECTOR_ALU_CONFIG",    a.ush.vectorALUConfig);
    B("UNIFIEDSHADER_VECTOR_WAIT_ON_STALL",  a.ush.vectorWaitOnStall);
    B("UNIFIEDSHADER_VECTOR_EXPLICIT_BLOCK",a.ush.vectorExplicitBlock);
    B("UNIFIEDSHADER_VERTEX_ATTRIBUTE_LOAD_FROM_SHADER", a.ush.vAttrLoadFromShader);
    B("UNIFIEDSHADER_THREAD_WINDOW",       a.ush.threadWindow);
    U("UNIFIEDSHADER_FETCH_DELAY",         a.ush.fetchDelay);
    B("UNIFIEDSHADER_SWAP_ON_BLOCK",        a.ush.swapOnBlock);
    B("UNIFIEDSHADER_FIXED_LATENCY_ALU",    a.ush.fixedLatencyALU);
    U("UNIFIEDSHADER_INPUTS_PER_CYCLE",     a.ush.inputsCycle);
    U("UNIFIEDSHADER_OUTPUTS_PER_CYCLE",    a.ush.outputsCycle);
    U("UNIFIEDSHADER_OUTPUT_LATENCY",      a.ush.outputLatency);
    U("UNIFIEDSHADER_TEXTURE_UNITS",       a.ush.textureUnits);
    U("UNIFIEDSHADER_TEXTURE_REQUEST_RATE", a.ush.textRequestRate);
    U("UNIFIEDSHADER_TEXTURE_REQUEST_GROUP",a.ush.textRequestGroup);
    U("UNIFIEDSHADER_ADDRESS_ALU_LATENCY",  a.ush.addressALULat);
    U("UNIFIEDSHADER_FILTER_ALU_LATENCY",   a.ush.filterLat);
    U("UNIFIEDSHADER_TEXTURE_BLOCK_DIMENSION",      a.ush.texBlockDim);
    U("UNIFIEDSHADER_TEXTURE_SUPER_BLOCK_DIMENSION",  a.ush.texSuperBlockDim);
    U("UNIFIEDSHADER_TEXTURE_REQUEST_QUEUE_SIZE",     a.ush.textReqQSize);
    U("UNIFIEDSHADER_TEXTURE_ACCESS_QUEUE",          a.ush.textAccessQSize);
    U("UNIFIEDSHADER_TEXTURE_RESULT_QUEUE",          a.ush.textResultQSize);
    U("UNIFIEDSHADER_TEXTURE_WAIT_READ_WINDOW",       a.ush.textWaitWSize);
    B("UNIFIEDSHADER_TWO_LEVEL_TEXTURE_CACHE",        a.ush.twoLevelCache);
    U("UNIFIEDSHADER_TEXTURE_CACHE_LINE_SIZE",        a.ush.txCacheLineSz);
    U("UNIFIEDSHADER_TEXTURE_CACHE_WAYS",            a.ush.txCacheWays);
    U("UNIFIEDSHADER_TEXTURE_CACHE_LINES",           a.ush.txCacheLines);
    U("UNIFIEDSHADER_TEXTURE_CACHE_PORT_WIDTH",       a.ush.txCachePortWidth);
    U("UNIFIEDSHADER_TEXTURE_CACHE_REQUEST_QUEUE_SIZE",a.ush.txCacheReqQSz);
    U("UNIFIEDSHADER_TEXTURE_CACHE_INPUT_QUEUE",      a.ush.txCacheInQSz);
    U("UNIFIEDSHADER_TEXTURE_CACHE_MISSES_PER_CYCLE",  a.ush.txCacheMissCycle);
    U("UNIFIEDSHADER_TEXTURE_CACHE_DECOMPRESS_LATENCY", a.ush.txCacheDecomprLatency);
    U("UNIFIEDSHADER_TEXTURE_CACHE_LINE_SIZE_L1",      a.ush.txCacheLineSzL1);
    U("UNIFIEDSHADER_TEXTURE_CACHE_WAYS_L1",          a.ush.txCacheWaysL1);
    U("UNIFIEDSHADER_TEXTURE_CACHE_LINES_L1",         a.ush.txCacheLinesL1);
    U("UNIFIEDSHADER_TEXTURE_CACHE_INPUT_QUEUE_L1",    a.ush.txCacheInQSzL1);
    U("UNIFIEDSHADER_ANISOTROPY_ALGORITHM",         a.ush.anisoAlgo);
    B("UNIFIEDSHADER_FORCE_MAX_ANISOTROPY",          a.ush.forceMaxAniso);
    U("UNIFIEDSHADER_MAX_ANISOTROPY",               a.ush.maxAnisotropy);
    U("UNIFIEDSHADER_TRILINEAR_PRECISION",          a.ush.triPrecision);
    U("UNIFIEDSHADER_BRILINEAR_THRESHOLD",          a.ush.briThreshold);
    U("UNIFIEDSHADER_ANISO_ROUND_PRECISION",         a.ush.anisoRoundPrec);
    U("UNIFIEDSHADER_ANISO_ROUND_THRESHOLD",         a.ush.anisoRoundThres);
    B("UNIFIEDSHADER_ANISO_RATIO_MULT_OF_TWO",         a.ush.anisoRatioMultOf2);

    // [ZSTENCILTEST]
    U("ZSTENCILTEST_STAMPS_PER_CYCLE",       a.zst.stampsCycle);
    U("ZSTENCILTEST_BYTES_PER_PIXEL",        a.zst.bytesPixel);
    B("ZSTENCILTEST_DISABLE_COMPRESSION",   a.zst.disableCompr);
    U("ZSTENCILTEST_Z_CACHE_WAYS",           a.zst.zCacheWays);
    U("ZSTENCILTEST_Z_CACHE_LINES",          a.zst.zCacheLines);
    U("ZSTENCILTEST_Z_CACHE_STAMPS_PER_LINE",  a.zst.zCacheLineStamps);
    U("ZSTENCILTEST_Z_CACHE_PORT_WIDTH",      a.zst.zCachePortWidth);
    B("ZSTENCILTEST_Z_CACHE_EXTRA_READ_PORT",  a.zst.extraReadPort);
    B("ZSTENCILTEST_Z_CACHE_EXTRA_WRITE_PORT", a.zst.extraWritePort);
    U("ZSTENCILTEST_Z_CACHE_REQUEST_QUEUE_SIZE", a.zst.zCacheReqQSz);
    U("ZSTENCILTEST_Z_CACHE_INPUT_QUEUE_SIZE",   a.zst.zCacheInQSz);
    U("ZSTENCILTEST_Z_CACHE_OUTPUT_QUEUE_SIZE",  a.zst.zCacheOutQSz);
    U("ZSTENCILTEST_BLOCK_STATE_MEMORY_SIZE",   a.zst.blockStateMemSz);
    U("ZSTENCILTEST_BLOCKS_CLEARED_PER_CYCLE",  a.zst.blockClearCycle);
    U("ZSTENCILTEST_COMPRESSION_UNIT_LATENCY", a.zst.comprLatency);
    U("ZSTENCILTEST_DECOMPRESSION_UNIT_LATENCY", a.zst.decomprLatency);
    U("ZSTENCILTEST_INPUT_QUEUE_SIZE",  a.zst.inputQueueSize);
    U("ZSTENCILTEST_FETCH_QUEUE_SIZE",  a.zst.fetchQueueSize);
    U("ZSTENCILTEST_READ_QUEUE_SIZE",   a.zst.readQueueSize);
    U("ZSTENCILTEST_OP_QUEUE_SIZE",     a.zst.opQueueSize);
    U("ZSTENCILTEST_WRITE_QUEUE_SIZE",  a.zst.writeQueueSize);
    U("ZSTENCILTEST_ZALU_TEST_RATE",    a.zst.zTestRate);
    U("ZSTENCILTEST_ZALU_LATENCY",     a.zst.zOpLatency);
    U("ZSTENCILTEST_COMPRESSION_ALGORITHM", a.zst.comprAlgo);

    // [COLORWRITE]
    U("COLORWRITE_STAMPS_PER_CYCLE",       a.cwr.stampsCycle);
    U("COLORWRITE_BYTES_PER_PIXEL",        a.cwr.bytesPixel);
    B("COLORWRITE_DISABLE_COMPRESSION",   a.cwr.disableCompr);
    U("COLORWRITE_COLOR_CACHE_WAYS",       a.cwr.cCacheWays);
    U("COLORWRITE_COLOR_CACHE_LINES",      a.cwr.cCacheLines);
    U("COLORWRITE_COLOR_CACHE_STAMPS_PER_LINE", a.cwr.cCacheLineStamps);
    U("COLORWRITE_COLOR_CACHE_PORT_WIDTH",     a.cwr.cCachePortWidth);
    B("COLORWRITE_COLOR_CACHE_EXTRA_READ_PORT", a.cwr.extraReadPort);
    B("COLORWRITE_COLOR_CACHE_EXTRA_WRITE_PORT",a.cwr.extraWritePort);
    U("COLORWRITE_COLOR_CACHE_REQUEST_QUEUE_SIZE", a.cwr.cCacheReqQSz);
    U("COLORWRITE_COLOR_CACHE_INPUT_QUEUE_SIZE",   a.cwr.cCacheInQSz);
    U("COLORWRITE_COLOR_CACHE_OUTPUT_QUEUE_SIZE",  a.cwr.cCacheOutQSz);
    U("COLORWRITE_BLOCK_STATE_MEMORY_SIZE",   a.cwr.blockStateMemSz);
    U("COLORWRITE_BLOCKS_CLEARED_PER_CYCLE",  a.cwr.blockClearCycle);
    U("COLORWRITE_COMPRESSION_UNIT_LATENCY", a.cwr.comprLatency);
    U("COLORWRITE_DECOMPRESSION_UNIT_LATENCY", a.cwr.decomprLatency);
    U("COLORWRITE_INPUT_QUEUE_SIZE",  a.cwr.inputQueueSize);
    U("COLORWRITE_FETCH_QUEUE_SIZE",  a.cwr.fetchQueueSize);
    U("COLORWRITE_READ_QUEUE_SIZE",   a.cwr.readQueueSize);
    U("COLORWRITE_OP_QUEUE_SIZE",     a.cwr.opQueueSize);
    U("COLORWRITE_WRITE_QUEUE_SIZE",  a.cwr.writeQueueSize);
    U("COLORWRITE_BLEND_ALU_RATE",    a.cwr.blendRate);
    U("COLORWRITE_BLEND_ALU_LATENCY", a.cwr.blendOpLatency);
    U("COLORWRITE_COMPRESSION_ALGORITHM", a.cwr.comprAlgo);

    // [DISPLAYCONTROLLER]
    U("DISPLAYCONTROLLER_BYTES_PER_PIXEL",        a.dac.bytesPixel);
    U("DISPLAYCONTROLLER_BLOCK_SIZE",            a.dac.blockSize);
    U("DISPLAYCONTROLLER_BLOCK_UPDATE_LATENCY",   a.dac.blockUpdateLat);
    U("DISPLAYCONTROLLER_BLOCKS_UPDATED_PER_CYCLE",a.dac.blocksCycle);
    U("DISPLAYCONTROLLER_BLOCK_REQUEST_QUEUE_SIZE",a.dac.blockReqQSz);
    U("DISPLAYCONTROLLER_DECOMPRESSION_UNIT_LATENCY", a.dac.decomprLatency);
    U("DISPLAYCONTROLLER_REFRESH_RATE",          a.dac.refreshRate);
    B("DISPLAYCONTROLLER_SYNCHED_REFRESH",       a.dac.synchedRefresh);
    B("DISPLAYCONTROLLER_REFRESH_FRAME",         a.dac.refreshFrame);
    B("DISPLAYCONTROLLER_SAVE_BLIT_SOURCE_DATA",   a.dac.saveBlitSource);

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
    arch_conf->sim.inputFile      = str_dup(s("SIMULATOR_INPUT_FILE"));
    arch_conf->sim.simCycles      = u64("SIMULATOR_SIM_CYCLES");
    arch_conf->sim.simFrames      = u32("SIMULATOR_SIM_FRAMES");
    arch_conf->sim.signalDumpFile = str_dup(s("SIMULATOR_SIGNAL_DUMP_FILE"));
    arch_conf->sim.statsFile      = str_dup(s("SIMULATOR_STATS_FILE"));
    arch_conf->sim.statsFilePerFrame = str_dup(s("SIMULATOR_STATS_FILE_PER_FRAME"));
    arch_conf->sim.statsFilePerBatch = str_dup(s("SIMULATOR_STATS_FILE_PER_BATCH"));
    arch_conf->sim.startFrame     = u32("SIMULATOR_START_FRAME");
    arch_conf->sim.startDump      = u64("SIMULATOR_START_SIGNAL_DUMP");
    arch_conf->sim.dumpCycles     = u64("SIMULATOR_SIGNAL_DUMP_CYCLES");
    arch_conf->sim.statsRate      = u32("SIMULATOR_STATISTICS_RATE");
    arch_conf->sim.dumpSignalTrace = b("SIMULATOR_DUMP_SIGNAL_TRACE");
    arch_conf->sim.statistics      = b("SIMULATOR_STATISTICS");
    arch_conf->sim.perCycleStatistics = b("SIMULATOR_PER_CYCLE_STATISTICS");
    arch_conf->sim.perFrameStatistics = b("SIMULATOR_PER_FRAME_STATISTICS");
    arch_conf->sim.perBatchStatistics = b("SIMULATOR_PER_BATCH_STATISTICS");
    arch_conf->sim.detectStalls    = b("SIMULATOR_DETECT_STALLS");
    arch_conf->sim.fragmentMap     = b("SIMULATOR_GENERATE_FRAGMENT_MAP");
    arch_conf->sim.fragmentMapMode = u32("SIMULATOR_FRAGMENT_MAP_MODE");
    arch_conf->sim.colorDoubleBuffer = b("SIMULATOR_DOUBLE_BUFFER");
    arch_conf->sim.forceMSAA       = b("SIMULATOR_FORCE_MSAA");
    arch_conf->sim.msaaSamples     = u32("SIMULATOR_MSAA_SAMPLES", 4);
    arch_conf->sim.forceFP16ColorBuffer = b("SIMULATOR_FORCE_FP16_COLOR_BUFFER");
    arch_conf->sim.enableDriverShaderTranslation = b("SIMULATOR_ENABLE_DRIVER_SHADER_TRANSLATION", true);
    arch_conf->sim.objectSize0     = u32("SIMULATOR_OBJECT_SIZE0", 512);
    arch_conf->sim.bucketSize0     = u32("SIMULATOR_BUCKET_SIZE0", 262144);
    arch_conf->sim.objectSize1     = u32("SIMULATOR_OBJECT_SIZE1", 4096);
    arch_conf->sim.bucketSize1     = u32("SIMULATOR_BUCKET_SIZE1", 32768);
    arch_conf->sim.objectSize2     = u32("SIMULATOR_OBJECT_SIZE2", 64);
    arch_conf->sim.bucketSize2     = u32("SIMULATOR_BUCKET_SIZE2", 65536);

    // ===== [GPU] =====
    arch_conf->gpu.numVShaders    = u32("GPU_NUM_VERTEX_SHADERS", 8);
    arch_conf->gpu.numFShaders    = u32("GPU_NUM_FRAGMENT_SHADERS", 4);
    arch_conf->gpu.numStampUnits  = u32("GPU_NUM_STAMP_PIPES", 4);
    arch_conf->gpu.gpuClock       = u32("GPU_GPU_CLOCK", 500);
    arch_conf->gpu.shaderClock    = u32("GPU_SHADER_CLOCK", 500);
    arch_conf->gpu.memoryClock    = u32("GPU_MEMORY_CLOCK", 500);

    // ===== [COMMANDPROCESSOR] =====
    arch_conf->com.pipelinedBatches   = b("COMMANDPROCESSOR_PIPELINED_BATCH_RENDERING", true);
    arch_conf->com.dumpShaderPrograms = b("COMMANDPROCESSOR_DUMP_SHADER_PROGRAMS");

    // ===== [MEMORYCONTROLLER] =====
    arch_conf->mem.memSize          = u32("MEMORYCONTROLLER_MEMORY_SIZE", 256);
    arch_conf->mem.clockMultiplier  = u32("MEMORYCONTROLLER_MEMORY_CLOCK_MULTIPLIER", 1);
    arch_conf->mem.memoryFrequency  = u32("MEMORYCONTROLLER_MEMORY_FREQUENCY", 1);
    arch_conf->mem.busWidth         = u32("MEMORYCONTROLLER_MEMORY_BUS_WIDTH", 64);
    arch_conf->mem.memBuses         = u32("MEMORYCONTROLLER_MEMORY_BUSES", 4);
    arch_conf->mem.sharedBanks      = b("MEMORYCONTROLLER_SHARED_BANKS");
    arch_conf->mem.bankGranurality  = u32("MEMORYCONTROLLER_BANK_GRANURALITY", 1024);
    arch_conf->mem.burstLength      = u32("MEMORYCONTROLLER_BURST_LENGTH", 8);
    arch_conf->mem.readLatency      = u32("MEMORYCONTROLLER_READ_LATENCY", 10);
    arch_conf->mem.writeLatency     = u32("MEMORYCONTROLLER_WRITE_LATENCY", 5);
    arch_conf->mem.writeToReadLat = u32("MEMORYCONTROLLER_WRITE_TO_READ_LATENCY", 5);
    arch_conf->mem.memPageSize      = u32("MEMORYCONTROLLER_MEMORY_PAGE_SIZE", 4096);
    arch_conf->mem.openPages        = u32("MEMORYCONTROLLER_OPEN_PAGES", 8);
    arch_conf->mem.pageOpenLat  = u32("MEMORYCONTROLLER_PAGE_OPEN_LATENCY", 13);
    arch_conf->mem.maxConsecutiveReads  = u32("MEMORYCONTROLLER_MAX_CONSECUTIVE_READS", 16);
    arch_conf->mem.maxConsecutiveWrites = u32("MEMORYCONTROLLER_MAX_CONSECUTIVE_WRITES", 16);
    arch_conf->mem.comProcBus       = u32("MEMORYCONTROLLER_COMMAND_PROCESSOR_BUS_WIDTH", 8);
    arch_conf->mem.strFetchBus      = u32("MEMORYCONTROLLER_STREAMER_FETCH_BUS_WIDTH", 64);
    arch_conf->mem.strLoaderBus     = u32("MEMORYCONTROLLER_STREAMER_LOADER_BUS_WIDTH", 64);
    arch_conf->mem.zStencilBus      = u32("MEMORYCONTROLLER_Z_STENCIL_BUS_WIDTH", 64);
    arch_conf->mem.cWriteBus    = u32("MEMORYCONTROLLER_COLOR_WRITE_BUS_WIDTH", 64);
    arch_conf->mem.dacBus           = u32("MEMORYCONTROLLER_DISPLAY_CONTROLLER_BUS_WIDTH", 64);
    arch_conf->mem.textUnitBus      = u32("MEMORYCONTROLLER_TEXTURE_UNIT_BUS_WIDTH", 64);
    arch_conf->mem.mappedMemSize    = u32("MEMORYCONTROLLER_MAPPED_MEMORY_SIZE", 16);
    arch_conf->mem.readBufferLines  = u32("MEMORYCONTROLLER_READ_BUFFER_LINES", 256);
    arch_conf->mem.writeBufferLines = u32("MEMORYCONTROLLER_WRITE_BUFFER_LINES", 256);
    arch_conf->mem.reqQueueSz     = u32("MEMORYCONTROLLER_REQUEST_QUEUE_SIZE", 512);
    arch_conf->mem.servQueueSz = u32("MEMORYCONTROLLER_SERVICE_QUEUE_SIZE", 64);
    // MCv2
    arch_conf->mem.memoryControllerV2 = b("MEMORYCONTROLLER_MEMORY_CONTROLLER_V2", true);
    arch_conf->mem.v2MemoryChannels   = u32("MEMORYCONTROLLER_V2_MEMORY_CHANNELS", 8);
    arch_conf->mem.v2BanksPerMemoryChannel = u32("MEMORYCONTROLLER_V2_BANKS_PER_MEMORY_CHANNEL", 8);
    arch_conf->mem.v2MemoryRowSize    = u32("MEMORYCONTROLLER_V2_MEMORY_ROW_SIZE", 2048);
    arch_conf->mem.v2BurstBytesPerCycle = u32("MEMORYCONTROLLER_V2_BURST_BYTES_PER_CYCLE", 8);
    arch_conf->mem.v2SplitterType     = u32("MEMORYCONTROLLER_V2_SPLITTER_TYPE", 1);
    arch_conf->mem.v2ChannelInterleaving = u32("MEMORYCONTROLLER_V2_CHANNEL_INTERLEAVING");
    arch_conf->mem.v2BankInterleaving   = u32("MEMORYCONTROLLER_V2_BANK_INTERLEAVING");
    arch_conf->mem.v2ChannelInterleavingMask = str_dup(s("MEMORYCONTROLLER_V2_CHANNEL_INTERLEAVING_MASK"));
    arch_conf->mem.v2BankInterleavingMask = str_dup(s("MEMORYCONTROLLER_V2_BANK_INTERLEAVING_MASK"));
    arch_conf->mem.v2SecondInterleaving = b("MEMORYCONTROLLER_V2_SECOND_INTERLEAVING");
    arch_conf->mem.v2SecondChannelInterleaving = u32("MEMORYCONTROLLER_V2_SECOND_CHANNEL_INTERLEAVING");
    arch_conf->mem.v2SecondBankInterleaving = u32("MEMORYCONTROLLER_V2_SECOND_BANK_INTERLEAVING");
    arch_conf->mem.v2SecondChannelInterleavingMask = str_dup(s("MEMORYCONTROLLER_V2_SECOND_CHANNEL_INTERLEAVING_MASK"));
    arch_conf->mem.v2SecondBankInterleavingMask = str_dup(s("MEMORYCONTROLLER_V2_SECOND_BANK_INTERLEAVING_MASK"));
    arch_conf->mem.v2BankSelectionPolicy = str_dup(s("MEMORYCONTROLLER_V2_BANK_SELECTION_POLICY"));
    arch_conf->mem.v2ChannelScheduler   = u32("MEMORYCONTROLLER_V2_CHANNEL_SCHEDULER", 2);
    arch_conf->mem.v2PagePolicy         = u32("MEMORYCONTROLLER_V2_PAGE_POLICY");
    arch_conf->mem.v2MaxChannelTransactions = u32("MEMORYCONTROLLER_V2_MAX_CHANNEL_TRANSACTIONS", 64);
    arch_conf->mem.v2DedicatedChannelReadTransactions = u32("MEMORYCONTROLLER_V2_DEDICATED_CHANNEL_READ_TRANSACTIONS");
    arch_conf->mem.v2UseChannelRequestFIFOPerBank = b("MEMORYCONTROLLER_V2_USE_CHANNEL_REQUEST_FIFO_PER_BANK", true);
    arch_conf->mem.v2ChannelRequestFIFOPerBankSelection = u32("MEMORYCONTROLLER_V2_CHANNEL_REQUEST_FIFO_PER_BANK_SELECTION", 1);
    arch_conf->mem.v2UseClassicSchedulerStates = b("MEMORYCONTROLLER_V2_USE_CLASSIC_SCHEDULER_STATES");
    arch_conf->mem.v2UseSplitRequestBufferPerROP = b("MEMORYCONTROLLER_V2_USE_SPLIT_REQUEST_BUFFER_PER_ROP");
    arch_conf->mem.v2MemoryTrace = b("MEMORYCONTROLLER_V2_MEMORY_TRACE");
    arch_conf->mem.v2SwitchModePolicy = u32("MEMORYCONTROLLER_V2_SWITCH_MODE_POLICY", 1);
    arch_conf->mem.v2ActiveManagerMode = u32("MEMORYCONTROLLER_V2_ACTIVE_MANAGER_MODE", 1);
    arch_conf->mem.v2PrechargeManagerMode = u32("MEMORYCONTROLLER_V2_PRECHARGE_MANAGER_MODE");
    arch_conf->mem.v2ManagerSelectionAlgorithm = u32("MEMORYCONTROLLER_V2_MANAGER_SELECTION_ALGORITHM");
    arch_conf->mem.v2DisableActiveManager = b("MEMORYCONTROLLER_V2_DISABLE_ACTIVE_MANAGER");
    arch_conf->mem.v2DisablePrechargeManager = b("MEMORYCONTROLLER_V2_DISABLE_PRECHARGE_MANAGER");
    arch_conf->mem.v2DebugString = str_dup(s("MEMORYCONTROLLER_V2_DEBUG_STRING"));
    arch_conf->mem.v2MemoryType = str_dup(s("MEMORYCONTROLLER_V2_MEMORY_TYPE"));
    arch_conf->mem.v2GDDR_Profile = str_dup(s("MEMORYCONTROLLER_V2_GDDR_PROFILE"));
    arch_conf->mem.v2GDDR_tRRD = u32("MEMORYCONTROLLER_V2_GDDR_T_RRD", 9);
    arch_conf->mem.v2GDDR_tRCD = u32("MEMORYCONTROLLER_V2_GDDR_T_RCD", 13);
    arch_conf->mem.v2GDDR_tWTR = u32("MEMORYCONTROLLER_V2_GDDR_T_WTR", 5);
    arch_conf->mem.v2GDDR_tRTW = u32("MEMORYCONTROLLER_V2_GDDR_T_RTW", 2);
    arch_conf->mem.v2GDDR_tWR  = u32("MEMORYCONTROLLER_V2_GDDR_T_WR", 10);
    arch_conf->mem.v2GDDR_tRP  = u32("MEMORYCONTROLLER_V2_GDDR_T_RP", 14);
    arch_conf->mem.v2GDDR_CAS  = u32("MEMORYCONTROLLER_V2_GDDR_CAS", 10);
    arch_conf->mem.v2GDDR_WL   = u32("MEMORYCONTROLLER_V2_GDDR_WL", 5);

    // ===== [STREAMER] =====
    arch_conf->str.indicesCycle   = u32("STREAMER_INDICES_CYCLE", 2);
    arch_conf->str.idxBufferSize     = u32("STREAMER_INDEX_BUFFER_SIZE", 2048);
    arch_conf->str.outputFIFOSize = u32("STREAMER_OUTPUT_FIFO_SIZE", 512);
    arch_conf->str.outputMemorySize  = u32("STREAMER_OUTPUT_MEMORY_SIZE", 512);
    arch_conf->str.verticesCycle  = u32("STREAMER_VERTICES_CYCLE", 2);
    arch_conf->str.attrSentCycle  = u32("STREAMER_ATTRIBUTES_SENT_CYCLE", 4);
    arch_conf->str.streamerLoaderUnits = u32("STREAMER_STREAMER_LOADER_UNITS", 1);
    arch_conf->str.slIndicesCycle = u32("STREAMER_SL_INDICES_CYCLE", 2);
    arch_conf->str.slInputReqQueueSize = u32("STREAMER_SL_INPUT_REQUEST_QUEUE_SIZE", 128);
    arch_conf->str.slAttributesCycle = u32("STREAMER_SL_ATTRIBUTES_CYCLE", 8);
    arch_conf->str.slInCacheLines = u32("STREAMER_SL_INPUT_CACHE_LINES", 32);
    arch_conf->str.slInCacheLineSz = u32("STREAMER_SL_INPUT_CACHE_LINE_SIZE", 256);
    arch_conf->str.slInCachePortWidth = u32("STREAMER_SL_INPUT_CACHE_PORT_WIDTH", 16);
    arch_conf->str.slInCacheReqQSz = u32("STREAMER_SL_INPUT_CACHE_REQUEST_QUEUE_SIZE", 8);
    arch_conf->str.slInCacheInputQSz = u32("STREAMER_SL_INPUT_CACHE_INPUT_QUEUE_SIZE", 8);

    // ===== [PRIMITIVEASSEMBLY] =====
    arch_conf->pas.verticesCycle     = u32("PRIMITIVEASSEMBLY_VERTICES_CYCLE", 2);
    arch_conf->pas.trianglesCycle    = u32("PRIMITIVEASSEMBLY_TRIANGLES_CYCLE", 2);
    arch_conf->pas.inputBusLat       = u32("PRIMITIVEASSEMBLY_INPUT_BUS_LATENCY", 10);
    arch_conf->pas.paQueueSize       = u32("PRIMITIVEASSEMBLY_ASSEMBLY_QUEUE_SIZE", 32);

    // ===== [CLIPPER] =====
    arch_conf->clp.trianglesCycle    = u32("CLIPPER_TRIANGLES_CYCLE", 2);
    arch_conf->clp.clipperUnits      = u32("CLIPPER_CLIPPER_UNITS", 2);
    arch_conf->clp.startLatency          = u32("CLIPPER_START_LATENCY", 1);
    arch_conf->clp.execLatency           = u32("CLIPPER_EXEC_LATENCY", 6);
    arch_conf->clp.clipBufferSize    = u32("CLIPPER_CLIP_BUFFER_SIZE", 32);

    // ===== [RASTERIZER] =====
    arch_conf->ras.trianglesCycle    = u32("RASTERIZER_TRIANGLES_CYCLE", 2);
    arch_conf->ras.setupFIFOSize     = u32("RASTERIZER_SETUP_FIFO_SIZE", 32);
    arch_conf->ras.setupUnits        = u32("RASTERIZER_SETUP_UNITS", 2);
    arch_conf->ras.setupLat          = u32("RASTERIZER_SETUP_LATENCY", 10);
    arch_conf->ras.setupStartLat     = u32("RASTERIZER_SETUP_START_LATENCY", 4);
    arch_conf->ras.trInputLat       = u32("RASTERIZER_TRIANGLE_INPUT_LATENCY", 2);
    arch_conf->ras.trOutputLat      = u32("RASTERIZER_TRIANGLE_OUTPUT_LATENCY", 2);
    arch_conf->ras.shadedSetup     = b("RASTERIZER_TRIANGLE_SETUP_ON_SHADER");
    arch_conf->ras.batchQueueSize     = u32("RASTERIZER_TRIANGLE_SHADER_QUEUE_SIZE", 8);
    arch_conf->ras.stampsCycle        = u32("RASTERIZER_STAMPS_PER_CYCLE", 4);
    arch_conf->ras.samplesCycle   = u32("RASTERIZER_MSAA_SAMPLES_CYCLE", 2);
    arch_conf->ras.overScanWidth      = u32("RASTERIZER_OVER_SCAN_WIDTH", 4);
    arch_conf->ras.overScanHeight     = u32("RASTERIZER_OVER_SCAN_HEIGHT", 4);
    arch_conf->ras.scanWidth          = u32("RASTERIZER_SCAN_WIDTH", 16);
    arch_conf->ras.scanHeight         = u32("RASTERIZER_SCAN_HEIGHT", 16);
    arch_conf->ras.genWidth           = u32("RASTERIZER_GEN_WIDTH", 8);
    arch_conf->ras.genHeight          = u32("RASTERIZER_GEN_HEIGHT", 8);
    arch_conf->ras.rastBatch      = u32("RASTERIZER_RASTERIZATION_BATCH_SIZE", 4);
    arch_conf->ras.batchQueueSize     = u32("RASTERIZER_BATCH_QUEUE_SIZE", 16);
    arch_conf->ras.recursive          = b("RASTERIZER_RECURSIVE_MODE", true);
    arch_conf->ras.disableHZ          = b("RASTERIZER_DISABLE_HZ");
    arch_conf->ras.stampsHZBlock   = u32("RASTERIZER_STAMPS_PER_HZ_BLOCK", 16);
    arch_conf->ras.hzBufferSize       = u32("RASTERIZER_HIERARCHICAL_Z_BUFFER_SIZE", 262144);
    arch_conf->ras.hzCacheLines       = u32("RASTERIZER_HZ_CACHE_LINES", 8);
    arch_conf->ras.hzCacheLineSize    = u32("RASTERIZER_HZ_CACHE_LINE_SIZE", 16);
    arch_conf->ras.earlyZQueueSz    = u32("RASTERIZER_EARLY_Z_QUEUE_SIZE", 256);
    arch_conf->ras.hzAccessLatency        = u32("RASTERIZER_HZ_ACCESS_LATENCY", 5);
    arch_conf->ras.hzUpdateLatency        = u32("RASTERIZER_HZ_UPDATE_LATENCY", 4);
    arch_conf->ras.hzBlockClearCycle  = u32("RASTERIZER_HZ_BLOCKS_CLEARED_PER_CYCLE", 256);
    arch_conf->ras.numInterpolators   = u32("RASTERIZER_NUM_INTERPOLATORS", 4);
    arch_conf->ras.shInputQSize       = u32("RASTERIZER_SHADER_INPUT_QUEUE_SIZE", 128);
    arch_conf->ras.shOutputQSize      = u32("RASTERIZER_SHADER_OUTPUT_QUEUE_SIZE", 128);
    arch_conf->ras.shInputBatchSize   = u32("RASTERIZER_SHADER_INPUT_BATCH_SIZE", 64);
    arch_conf->ras.tiledShDistro      = b("RASTERIZER_TILED_SHADER_DISTRIBUTION", true);
    arch_conf->ras.vInputQSize        = u32("RASTERIZER_VERTEX_INPUT_QUEUE_SIZE", 32);
    arch_conf->ras.vShadedQSize       = u32("RASTERIZER_SHADED_VERTEX_QUEUE_SIZE", 512);
    arch_conf->ras.trInputQSize    = u32("RASTERIZER_TRIANGLE_INPUT_QUEUE_SIZE", 32);
    arch_conf->ras.trOutputQSize   = u32("RASTERIZER_TRIANGLE_OUTPUT_QUEUE_SIZE", 32);
    arch_conf->ras.genStampQSize      = u32("RASTERIZER_GENERATED_STAMP_QUEUE_SIZE", 256);
    arch_conf->ras.testStampQSize    = u32("RASTERIZER_EARLY_Z_TESTED_STAMP_QUEUE_SIZE", 32);
    arch_conf->ras.intStampQSize        = u32("RASTERIZER_INTERPOLATED_STAMP_QUEUE_SIZE", 16);
    arch_conf->ras.shadedStampQSize   = u32("RASTERIZER_SHADED_STAMP_QUEUE_SIZE", 640);
    arch_conf->ras.emuStoredTriangles = u32("RASTERIZER_EMULATOR_STORED_TRIANGLES", 64);
    // Micropolygon rasterizer
    arch_conf->ras.useMicroPolRast    = b("RASTERIZER_USE_MICRO_POLYGON_RASTERIZER");
    arch_conf->ras.preBoundTriangles  = b("RASTERIZER_PRE_BOUND_TRIANGLES", true);
    arch_conf->ras.cullMicroTriangles = b("RASTERIZER_CULL_MICRO_TRIANGLES");
    arch_conf->ras.trBndOutLat     = u32("RASTERIZER_TRIANGLE_BOUND_OUTPUT_LATENCY", 2);
    arch_conf->ras.trBndOpLat      = u32("RASTERIZER_TRIANGLE_BOUND_OP_LATENCY", 2);
    arch_conf->ras.largeTriMinSz   = u32("RASTERIZER_LARGE_TRIANGLE_FIFO_SIZE", 64);
    arch_conf->ras.trBndMicroTriFIFOSz   = u32("RASTERIZER_MICRO_TRIANGLE_FIFO_SIZE", 64);
    arch_conf->ras.useBBOptimization  = b("RASTERIZER_USE_BOUNDING_BOX_OPTIMIZATION", true);
    arch_conf->ras.subPixelPrecision  = u32("RASTERIZER_SUB_PIXEL_PRECISION_BITS", 8);
    arch_conf->ras.largeTriMinSz    = static_cast<float>(f64("RASTERIZER_LARGE_TRIANGLE_MIN_SIZE", 16.0));
    arch_conf->ras.microTrisAsFragments = b("RASTERIZER_PROCESS_MICRO_TRIANGLES_AS_FRAGMENTS", true);
    arch_conf->ras.microTriSzLimit    = u32("RASTERIZER_MICRO_TRIANGLE_SIZE_LIMIT");
    arch_conf->ras.microTriFlowPath   = str_dup(s("RASTERIZER_MICRO_TRIANGLE_FLOW_PATH"));
    arch_conf->ras.dumpBurstHist   = b("RASTERIZER_DUMP_TRIANGLE_BURST_SIZE_HISTOGRAM");

    // ===== [UNIFIEDSHADER] =====
    arch_conf->ush.numThreads     = u32("UNIFIEDSHADER_EXECUTABLE_THREADS", 2048);
    arch_conf->ush.numInputBuffers = u32("UNIFIEDSHADER_INPUT_BUFFERS", 16);
    arch_conf->ush.numResources   = u32("UNIFIEDSHADER_THREAD_RESOURCES", 8192);
    arch_conf->ush.threadRate     = u32("UNIFIEDSHADER_THREAD_RATE", 4);
    arch_conf->ush.fetchRate      = u32("UNIFIEDSHADER_FETCH_RATE", 2);
    arch_conf->ush.scalarALU      = b("UNIFIEDSHADER_SCALAR_ALU", true);
    arch_conf->ush.threadGroup    = u32("UNIFIEDSHADER_THREAD_GROUP", 16);
    arch_conf->ush.lockedMode     = b("UNIFIEDSHADER_LOCKED_EXECUTION_MODE", true);
    // Vector shader
    arch_conf->ush.useVectorShader   = b("UNIFIEDSHADER_VECTOR_SHADER", true);
    arch_conf->ush.vectorThreads  = u32("UNIFIEDSHADER_VECTOR_THREADS", 128);
    arch_conf->ush.vectorResources = u32("UNIFIEDSHADER_VECTOR_RESOURCES", 512);
    arch_conf->ush.vectorLength   = u32("UNIFIEDSHADER_VECTOR_LENGTH", 16);
    arch_conf->ush.vectorALUWidth = u32("UNIFIEDSHADER_VECTOR_ALU_WIDTH", 4);
    arch_conf->ush.vectorALUConfig = str_dup(s("UNIFIEDSHADER_VECTOR_ALU_CONFIG"));
    arch_conf->ush.vectorWaitOnStall = b("UNIFIEDSHADER_VECTOR_WAIT_ON_STALL");
    arch_conf->ush.vectorExplicitBlock = b("UNIFIEDSHADER_VECTOR_EXPLICIT_BLOCK");
    // Common shader
    arch_conf->ush.vAttrLoadFromShader = b("UNIFIEDSHADER_VERTEX_ATTRIBUTE_LOAD_FROM_SHADER");
    arch_conf->ush.threadWindow   = b("UNIFIEDSHADER_THREAD_WINDOW", true);
    arch_conf->ush.fetchDelay     = u32("UNIFIEDSHADER_FETCH_DELAY", 4);
    arch_conf->ush.swapOnBlock    = b("UNIFIEDSHADER_SWAP_ON_BLOCK");
    arch_conf->ush.fixedLatencyALU    = b("UNIFIEDSHADER_FIXED_LATENCY_ALU");
    arch_conf->ush.inputsCycle    = u32("UNIFIEDSHADER_INPUTS_PER_CYCLE", 4);
    arch_conf->ush.outputsCycle   = u32("UNIFIEDSHADER_OUTPUTS_PER_CYCLE", 4);
    arch_conf->ush.outputLatency  = u32("UNIFIEDSHADER_OUTPUT_LATENCY", 11);
    arch_conf->ush.textureUnits   = u32("UNIFIEDSHADER_TEXTURE_UNITS", 1);
    arch_conf->ush.textRequestRate = u32("UNIFIEDSHADER_TEXTURE_REQUEST_RATE", 1);
    arch_conf->ush.textRequestGroup = u32("UNIFIEDSHADER_TEXTURE_REQUEST_GROUP", 64);
    // Texture unit
    arch_conf->ush.addressALULat  = u32("UNIFIEDSHADER_ADDRESS_ALU_LATENCY", 6);
    arch_conf->ush.filterLat   = u32("UNIFIEDSHADER_FILTER_ALU_LATENCY", 4);
    arch_conf->ush.texBlockDim    = u32("UNIFIEDSHADER_TEXTURE_BLOCK_DIMENSION", 2);
    arch_conf->ush.texSuperBlockDim = u32("UNIFIEDSHADER_TEXTURE_SUPER_BLOCK_DIMENSION", 4);
    arch_conf->ush.textReqQSize   = u32("UNIFIEDSHADER_TEXTURE_REQUEST_QUEUE_SIZE", 512);
    arch_conf->ush.textAccessQSize = u32("UNIFIEDSHADER_TEXTURE_ACCESS_QUEUE", 256);
    arch_conf->ush.textResultQSize = u32("UNIFIEDSHADER_TEXTURE_RESULT_QUEUE", 4);
    arch_conf->ush.textWaitWSize = u32("UNIFIEDSHADER_TEXTURE_WAIT_READ_WINDOW", 32);
    arch_conf->ush.twoLevelCache  = b("UNIFIEDSHADER_TWO_LEVEL_TEXTURE_CACHE", true);
    arch_conf->ush.txCacheLineSz = u32("UNIFIEDSHADER_TEXTURE_CACHE_LINE_SIZE", 64);
    arch_conf->ush.txCacheWays  = u32("UNIFIEDSHADER_TEXTURE_CACHE_WAYS", 8);
    arch_conf->ush.txCacheLines = u32("UNIFIEDSHADER_TEXTURE_CACHE_LINES", 8);
    arch_conf->ush.txCachePortWidth = u32("UNIFIEDSHADER_TEXTURE_CACHE_PORT_WIDTH", 4);
    arch_conf->ush.txCacheReqQSz = u32("UNIFIEDSHADER_TEXTURE_CACHE_REQUEST_QUEUE_SIZE", 32);
    arch_conf->ush.txCacheInQSz = u32("UNIFIEDSHADER_TEXTURE_CACHE_INPUT_QUEUE", 32);
    arch_conf->ush.txCacheMissCycle = u32("UNIFIEDSHADER_TEXTURE_CACHE_MISSES_PER_CYCLE", 8);
    arch_conf->ush.txCacheDecomprLatency = u32("UNIFIEDSHADER_TEXTURE_CACHE_DECOMPRESS_LATENCY", 1);
    arch_conf->ush.txCacheLineSzL1 = u32("UNIFIEDSHADER_TEXTURE_CACHE_LINE_SIZE_L1", 64);
    arch_conf->ush.txCacheWaysL1 = u32("UNIFIEDSHADER_TEXTURE_CACHE_WAYS_L1", 16);
    arch_conf->ush.txCacheLinesL1 = u32("UNIFIEDSHADER_TEXTURE_CACHE_LINES_L1", 16);
    arch_conf->ush.txCacheInQSzL1 = u32("UNIFIEDSHADER_TEXTURE_CACHE_INPUT_QUEUE_L1", 32);
    arch_conf->ush.anisoAlgo      = u32("UNIFIEDSHADER_ANISOTROPY_ALGORITHM");
    arch_conf->ush.forceMaxAniso  = b("UNIFIEDSHADER_FORCE_MAX_ANISOTROPY");
    arch_conf->ush.maxAnisotropy  = u32("UNIFIEDSHADER_MAX_ANISOTROPY", 16);
    arch_conf->ush.triPrecision  = u32("UNIFIEDSHADER_TRILINEAR_PRECISION", 8);
    arch_conf->ush.briThreshold = u32("UNIFIEDSHADER_BRILINEAR_THRESHOLD");
    arch_conf->ush.anisoRoundPrec = u32("UNIFIEDSHADER_ANISO_ROUND_PRECISION", 8);
    arch_conf->ush.anisoRoundThres = u32("UNIFIEDSHADER_ANISO_ROUND_THRESHOLD");
    arch_conf->ush.anisoRatioMultOf2 = b("UNIFIEDSHADER_ANISO_RATIO_MULT_OF_TWO");

    // ===== [ZSTENCILTEST] =====
    arch_conf->zst.stampsCycle    = u32("ZSTENCILTEST_STAMPS_PER_CYCLE", 1);
    arch_conf->zst.bytesPixel     = u32("ZSTENCILTEST_BYTES_PER_PIXEL", 4);
    arch_conf->zst.disableCompr = b("ZSTENCILTEST_DISABLE_COMPRESSION");
    arch_conf->zst.zCacheWays     = u32("ZSTENCILTEST_Z_CACHE_WAYS", 4);
    arch_conf->zst.zCacheLines    = u32("ZSTENCILTEST_Z_CACHE_LINES", 16);
    arch_conf->zst.zCacheLineStamps = u32("ZSTENCILTEST_Z_CACHE_STAMPS_PER_LINE", 16);
    arch_conf->zst.zCachePortWidth = u32("ZSTENCILTEST_Z_CACHE_PORT_WIDTH", 32);
    arch_conf->zst.extraReadPort = b("ZSTENCILTEST_Z_CACHE_EXTRA_READ_PORT", true);
    arch_conf->zst.extraWritePort = b("ZSTENCILTEST_Z_CACHE_EXTRA_WRITE_PORT", true);
    arch_conf->zst.zCacheReqQSz = u32("ZSTENCILTEST_Z_CACHE_REQUEST_QUEUE_SIZE", 8);
    arch_conf->zst.zCacheInQSz  = u32("ZSTENCILTEST_Z_CACHE_INPUT_QUEUE_SIZE", 8);
    arch_conf->zst.zCacheOutQSz = u32("ZSTENCILTEST_Z_CACHE_OUTPUT_QUEUE_SIZE", 8);
    arch_conf->zst.blockStateMemSz = u32("ZSTENCILTEST_BLOCK_STATE_MEMORY_SIZE", 262144);
    arch_conf->zst.blockClearCycle = u32("ZSTENCILTEST_BLOCKS_CLEARED_PER_CYCLE", 1024);
    arch_conf->zst.comprLatency   = u32("ZSTENCILTEST_COMPRESSION_UNIT_LATENCY", 8);
    arch_conf->zst.decomprLatency = u32("ZSTENCILTEST_DECOMPRESSION_UNIT_LATENCY", 8);
    arch_conf->zst.inputQueueSize     = u32("ZSTENCILTEST_INPUT_QUEUE_SIZE", 8);
    arch_conf->zst.fetchQueueSize     = u32("ZSTENCILTEST_FETCH_QUEUE_SIZE", 256);
    arch_conf->zst.readQueueSize      = u32("ZSTENCILTEST_READ_QUEUE_SIZE", 16);
    arch_conf->zst.opQueueSize        = u32("ZSTENCILTEST_OP_QUEUE_SIZE", 4);
    arch_conf->zst.writeQueueSize     = u32("ZSTENCILTEST_WRITE_QUEUE_SIZE", 8);
    arch_conf->zst.zTestRate      = u32("ZSTENCILTEST_ZALU_TEST_RATE", 1);
    arch_conf->zst.zOpLatency   = u32("ZSTENCILTEST_ZALU_LATENCY", 2);
    arch_conf->zst.comprAlgo = u32("ZSTENCILTEST_COMPRESSION_ALGORITHM");

    // ===== [COLORWRITE] =====
    arch_conf->cwr.stampsCycle    = u32("COLORWRITE_STAMPS_PER_CYCLE", 1);
    arch_conf->cwr.bytesPixel     = u32("COLORWRITE_BYTES_PER_PIXEL", 4);
    arch_conf->cwr.disableCompr = b("COLORWRITE_DISABLE_COMPRESSION");
    arch_conf->cwr.cCacheWays = u32("COLORWRITE_COLOR_CACHE_WAYS", 4);
    arch_conf->cwr.cCacheLines = u32("COLORWRITE_COLOR_CACHE_LINES", 16);
    arch_conf->cwr.cCacheLineStamps = u32("COLORWRITE_COLOR_CACHE_STAMPS_PER_LINE", 16);
    arch_conf->cwr.cCachePortWidth = u32("COLORWRITE_COLOR_CACHE_PORT_WIDTH", 32);
    arch_conf->cwr.extraReadPort = b("COLORWRITE_COLOR_CACHE_EXTRA_READ_PORT", true);
    arch_conf->cwr.extraWritePort = b("COLORWRITE_COLOR_CACHE_EXTRA_WRITE_PORT", true);
    arch_conf->cwr.cCacheReqQSz = u32("COLORWRITE_COLOR_CACHE_REQUEST_QUEUE_SIZE", 8);
    arch_conf->cwr.cCacheInQSz = u32("COLORWRITE_COLOR_CACHE_INPUT_QUEUE_SIZE", 8);
    arch_conf->cwr.cCacheOutQSz = u32("COLORWRITE_COLOR_CACHE_OUTPUT_QUEUE_SIZE", 8);
    arch_conf->cwr.blockStateMemSz = u32("COLORWRITE_BLOCK_STATE_MEMORY_SIZE", 262144);
    arch_conf->cwr.blockClearCycle = u32("COLORWRITE_BLOCKS_CLEARED_PER_CYCLE", 1024);
    arch_conf->cwr.comprLatency   = u32("COLORWRITE_COMPRESSION_UNIT_LATENCY", 8);
    arch_conf->cwr.decomprLatency = u32("COLORWRITE_DECOMPRESSION_UNIT_LATENCY", 8);
    arch_conf->cwr.inputQueueSize     = u32("COLORWRITE_INPUT_QUEUE_SIZE", 8);
    arch_conf->cwr.fetchQueueSize     = u32("COLORWRITE_FETCH_QUEUE_SIZE", 256);
    arch_conf->cwr.readQueueSize      = u32("COLORWRITE_READ_QUEUE_SIZE", 16);
    arch_conf->cwr.opQueueSize        = u32("COLORWRITE_OP_QUEUE_SIZE", 4);
    arch_conf->cwr.writeQueueSize     = u32("COLORWRITE_WRITE_QUEUE_SIZE", 8);
    arch_conf->cwr.blendRate      = u32("COLORWRITE_BLEND_ALU_RATE", 1);
    arch_conf->cwr.blendOpLatency   = u32("COLORWRITE_BLEND_ALU_LATENCY", 2);
    arch_conf->cwr.comprAlgo = u32("COLORWRITE_COMPRESSION_ALGORITHM");

    // ===== [DISPLAYCONTROLLER] =====
    arch_conf->dac.bytesPixel     = u32("DISPLAYCONTROLLER_BYTES_PER_PIXEL", 4);
    arch_conf->dac.blockSize      = u32("DISPLAYCONTROLLER_BLOCK_SIZE", 256);
    arch_conf->dac.blockUpdateLat = u32("DISPLAYCONTROLLER_BLOCK_UPDATE_LATENCY", 1);
    arch_conf->dac.blocksCycle    = u32("DISPLAYCONTROLLER_BLOCKS_UPDATED_PER_CYCLE", 1024);
    arch_conf->dac.blockReqQSz  = u32("DISPLAYCONTROLLER_BLOCK_REQUEST_QUEUE_SIZE", 32);
    arch_conf->dac.decomprLatency = u32("DISPLAYCONTROLLER_DECOMPRESSION_UNIT_LATENCY", 1);
    arch_conf->dac.refreshRate    = u32("DISPLAYCONTROLLER_REFRESH_RATE", 5000000);
    arch_conf->dac.synchedRefresh = b("DISPLAYCONTROLLER_SYNCHED_REFRESH", true);
    arch_conf->dac.refreshFrame   = b("DISPLAYCONTROLLER_REFRESH_FRAME", true);
    arch_conf->dac.saveBlitSource = b("DISPLAYCONTROLLER_SAVE_BLIT_SOURCE_DATA");

    // Signals not handled via CSV — set to null / zero
    arch_conf->sig = nullptr;
    arch_conf->numSignals = 0;
}
