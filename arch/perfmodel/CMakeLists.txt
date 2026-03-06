add_library(perfmodel)

set_target_properties(perfmodel PROPERTIES FOLDER arch)

set( BASE
    common/base/MduBase.h
    common/base/GPUSignal.h
    common/base/MultiClockMdu.h
    common/base/SignalBinder.h
    common/base/Statistic.h
    common/base/StatisticsManager.h
    common/base/toolsQueue.h
    common/base/MduBase.cpp
    common/base/GPUSignal.cpp
    common/base/MultiClockMdu.cpp
    common/base/SignalBinder.cpp
    common/base/Statistic.cpp
    common/base/StatisticsManager.cpp
)

set( COMPRESS
    common/compress/Compressor.h
    common/compress/ColorCompressor.h
    common/compress/ColorCompressor.cpp
    common/compress/DepthCompressor.h
    common/compress/DepthCompressor.cpp
    common/compress/HiloCompressor.h
    common/compress/HiloCompressor.cpp
    common/compress/MsaaCompressor.h
    common/compress/MsaaCompressor.cpp
)

set( CACHE
    CacheSystem/CacheTemplate.h
    CacheSystem/CacheReplacement.h
    CacheSystem/CacheReplacement.cpp
    CacheSystem/ColorCacheV2.h
    CacheSystem/FetchCache.h
    CacheSystem/FetchCache64.h
    CacheSystem/InputCache.h
    CacheSystem/ROPCache.h
    CacheSystem/TextureCache.h
    CacheSystem/TextureCacheGen.h
    CacheSystem/TextureCacheL2.h
    CacheSystem/ZCacheV2.h
    CacheSystem/ColorCacheV2.cpp
    CacheSystem/FetchCache.cpp
    CacheSystem/FetchCache64.cpp
    CacheSystem/InputCache.cpp
    CacheSystem/ROPCache.cpp
    CacheSystem/TextureCache.cpp
    CacheSystem/TextureCacheGen.cpp
    CacheSystem/TextureCacheL2.cpp
    CacheSystem/ZCacheV2.cpp
)

set( CLIPPER
    Clipper/Clipper.h
    Clipper/ClipperCommand.h
    Clipper/ClipperStateInfo.h
    Clipper/ClipperStatusInfo.h
    Clipper/Clipper.cpp
    Clipper/ClipperCommand.cpp
    Clipper/ClipperStateInfo.cpp
    Clipper/ClipperStatusInfo.cpp
)

set( COMMANDPROC
    ../common/MetaStream.h
    ../common/MetaStream.cpp
    CommandProcessor/CommandProcessor.h
    CommandProcessor/CommandProcessor.cpp
)

set( DISPLAYCTRL
    DisplayController/Blitter.h
    DisplayController/DisplayController.h
    DisplayController/WakeUpQueue.h
    DisplayController/Blitter.cpp
    DisplayController/DisplayController.cpp
)

set( FRAGMENTOPERATOR
    FragmentOperator/BlendOperation.h
    FragmentOperator/ColorBlockStateInfo.h
    FragmentOperator/ColorWriteV2.h
    FragmentOperator/GenericROP.h
    FragmentOperator/ROPOperation.h
    FragmentOperator/ROPStatusInfo.h
    FragmentOperator/ZOperation.h
    FragmentOperator/ZStencilStatusInfo.h
    FragmentOperator/ZStencilTestV2.h
    FragmentOperator/BlendOperation.cpp
    FragmentOperator/ColorBlockStateInfo.cpp
    FragmentOperator/ColorWriteV2.cpp
    FragmentOperator/GenericROP.cpp
    FragmentOperator/ROPOperation.cpp
    FragmentOperator/ROPStatusInfo.cpp
    FragmentOperator/ZOperation.cpp
    FragmentOperator/ZStencilStatusInfo.cpp
    FragmentOperator/ZStencilTestV2.cpp
)

set( MEMORYCTRL
    MemoryControllerV2/BankQueueScheduler.h
    MemoryControllerV2/BankSelectionPolicy.h
    MemoryControllerV2/ChannelScheduler.h
    MemoryControllerV2/ChannelTransaction.h
    MemoryControllerV2/DDRBank.h
    MemoryControllerV2/DDRBurst.h
    MemoryControllerV2/DDRCommand.h
    MemoryControllerV2/DDRModule.h
    MemoryControllerV2/DDRModuleState.h
    MemoryControllerV2/DependencyBuffer.h
    MemoryControllerV2/DependencyQueue.h
    MemoryControllerV2/FifoScheduler.h
    MemoryControllerV2/FifoSchedulerBase.h
    MemoryControllerV2/GPUMemorySpecs.h
    MemoryControllerV2/MCSplitter.h
    MemoryControllerV2/MCSplitter2.h
    MemoryControllerV2/MemoryControllerV2.h
    MemoryControllerV2/MemoryRequest.h
    MemoryControllerV2/MemoryRequestSplitter.h
    MemoryControllerV2/MemoryTraceRecorder.h
    MemoryControllerV2/RWFifoScheduler.h
    MemoryControllerV2/SchedulerSelector.h
    MemoryControllerV2/SwitchOperationMode.h
    MemoryController/MemoryController.h
    MemoryController/MemoryControllerCommand.h
    MemoryController/MemoryControllerDefs.h
    MemoryController/MemorySpace.h
    MemoryController/MemoryTransaction.h
    MemoryControllerV2/BankQueueScheduler.cpp
    MemoryControllerV2/BankSelectionPolicy.cpp
    MemoryControllerV2/ChannelScheduler.cpp
    MemoryControllerV2/ChannelTransaction.cpp
    MemoryControllerV2/DDRBank.cpp
    MemoryControllerV2/DDRBurst.cpp
    MemoryControllerV2/DDRCommand.cpp
    MemoryControllerV2/DDRModule.cpp
    MemoryControllerV2/DDRModuleState.cpp
    MemoryControllerV2/DependencyBuffer.cpp
    MemoryControllerV2/DependencyQueue.cpp
    MemoryControllerV2/FifoScheduler.cpp
    MemoryControllerV2/FifoSchedulerBase.cpp
    MemoryControllerV2/MCSplitter.cpp
    MemoryControllerV2/MCSplitter2.cpp
    MemoryControllerV2/MemoryControllerV2.cpp
    MemoryControllerV2/MemoryRequest.cpp
    MemoryControllerV2/MemoryRequestSplitter.cpp
    MemoryControllerV2/MemoryTraceRecorder.cpp
    MemoryControllerV2/RWFifoScheduler.cpp
    MemoryControllerV2/SchedulerSelector.cpp
    MemoryControllerV2/SwitchOperationMode.cpp
    MemoryController/MemoryController.cpp
    MemoryController/MemoryControllerCommand.cpp
    MemoryController/MemoryTransaction.cpp
)

set( PRIMITIVEASM
    PrimitiveAssembler/PrimitiveAssembly.h
    PrimitiveAssembler/PrimitiveAssemblyCommand.h
    PrimitiveAssembler/PrimitiveAssemblyInput.h
    PrimitiveAssembler/PrimitiveAssemblyRequest.h
    PrimitiveAssembler/PrimitiveAssemblyStateInfo.h
    PrimitiveAssembler/PrimitiveAssembly.cpp
    PrimitiveAssembler/PrimitiveAssemblyCommand.cpp
    PrimitiveAssembler/PrimitiveAssemblyInput.cpp
    PrimitiveAssembler/PrimitiveAssemblyRequest.cpp
    PrimitiveAssembler/PrimitiveAssemblyStateInfo.cpp
)

set( RASTERIZER
    Rasterizer/FFIFOStateInfo.h
    Rasterizer/FragmentFIFO.h
    Rasterizer/FragmentInput.h
    Rasterizer/HierarchicalZ.h
    Rasterizer/HZAccess.h
    Rasterizer/HZStateInfo.h
    Rasterizer/HZUpdate.h
    Rasterizer/Interpolator.h
    Rasterizer/Rasterizer.h
    Rasterizer/RasterizerCommand.h
    Rasterizer/RasterizerState.h
    Rasterizer/RasterizerStateInfo.h
    Rasterizer/TileEvaluatorStateInfo.h
    Rasterizer/TileInput.h
    Rasterizer/TriangleSetup.h
    Rasterizer/TriangleSetupInput.h
    Rasterizer/TriangleSetupOutput.h
    Rasterizer/TriangleSetupRequest.h
    Rasterizer/TriangleSetupStateInfo.h
    Rasterizer/TriangleTraversal.h
    Rasterizer/FFIFOStateInfo.cpp
    Rasterizer/FragmentFIFO.cpp
    Rasterizer/FragmentInput.cpp
    Rasterizer/HierarchicalZ.cpp
    Rasterizer/HZAccess.cpp
    Rasterizer/HZStateInfo.cpp
    Rasterizer/HZUpdate.cpp
    Rasterizer/Interpolator.cpp
    Rasterizer/Rasterizer.cpp
    Rasterizer/RasterizerCommand.cpp
    Rasterizer/RasterizerStateInfo.cpp
    Rasterizer/TileEvaluatorStateInfo.cpp
    Rasterizer/TileInput.cpp
    Rasterizer/TriangleSetup.cpp
    Rasterizer/TriangleSetupInput.cpp
    Rasterizer/TriangleSetupOutput.cpp
    Rasterizer/TriangleSetupRequest.cpp
    Rasterizer/TriangleSetupStateInfo.cpp
    Rasterizer/TriangleTraversal.cpp
)

set( UNIFIEDSHADER
    UnifiedShader/ConsumerStateInfo.h
    UnifiedShader/FilterOperation.h
    UnifiedShader/ShaderArchParam.h
    UnifiedShader/ShaderCommand.h
    UnifiedShader/ShaderCommon.h
    UnifiedShader/ShaderDecodeCommand.h
    UnifiedShader/ShaderDecodeExecute.h
    UnifiedShader/ShaderDecodeStateInfo.h
    UnifiedShader/ShaderExecInstruction.h
    UnifiedShader/ShaderFetch.h
    UnifiedShader/ShaderInput.h
    UnifiedShader/ShaderState.h
    UnifiedShader/ShaderStateInfo.h
    UnifiedShader/TextureRequest.h
    UnifiedShader/TextureResult.h
    UnifiedShader/TextureProcessor.h
    UnifiedShader/TextureUnitStateInfo.h
    UnifiedShader/InstructionFetchVector.h
    UnifiedShader/ShaderDecodeExecuteVector.h
    UnifiedShader/ShaderFetchVector.h
    UnifiedShader/UnifiedShader.h
    UnifiedShader/ConsumerStateInfo.cpp
    UnifiedShader/FilterOperation.cpp
    UnifiedShader/ShaderArchParam.cpp
    UnifiedShader/ShaderCommand.cpp
    UnifiedShader/ShaderDecodeCommand.cpp
    UnifiedShader/ShaderDecodeExecute.cpp
    UnifiedShader/ShaderDecodeStateInfo.cpp
    UnifiedShader/ShaderExecInstruction.cpp
    UnifiedShader/ShaderFetch.cpp
    UnifiedShader/ShaderInput.cpp
    UnifiedShader/ShaderStateInfo.cpp
    UnifiedShader/TextureRequest.cpp
    UnifiedShader/TextureResult.cpp
    UnifiedShader/TextureProcessor.cpp
    UnifiedShader/TextureUnitStateInfo.cpp
    UnifiedShader/InstructionFetchVector.cpp
    UnifiedShader/ShaderDecodeExecuteVector.cpp
    UnifiedShader/ShaderFetchVector.cpp
    UnifiedShader/UnifiedShader.cpp
)

set( STREAMER
    StreamController/Streamer.h
    StreamController/StreamerCommand.h
    StreamController/StreamerCommit.h
    StreamController/StreamerControlCommand.h
    StreamController/StreamerFetch.h
    StreamController/StreamerLoader.h
    StreamController/StreamerOutputCache.h
    StreamController/StreamerStateInfo.h
    StreamController/Streamer.cpp
    StreamController/StreamerCommand.cpp
    StreamController/StreamerCommit.cpp
    StreamController/StreamerControlCommand.cpp
    StreamController/StreamerFetch.cpp
    StreamController/StreamerLoader.cpp
    StreamController/StreamerOutputCache.cpp
    StreamController/StreamerStateInfo.cpp
)

set( TOP
    GpuTop.h
    GpuTop.cpp
    perfmodel.h
    perfmodel.cpp)

set(EXTERNALSRC 
                "../utils/MemoryControllerSelector.cpp"
                "../../driver/utils/zfstream.cpp"
                "../../driver/utils/TraceDriver/TraceDriverMeta.cpp" # FIXME: update to cgoTraceDriverBase? move TraceDriverMeta to CG1GpuSim
                "../../driver/utils/MetaTraceGenerator/traceExtractor/RegisterWriteBufferMeta.cpp"# FIXME: update to cgoTraceDriverBase?
                ) 

source_group(common/base        FILES ${BASE})
source_group(common/comp        FILES ${COMPRESS})
source_group(cacheSystem        FILES ${CACHE})
source_group(commandProcessor   FILES ${COMMANDPROC})
source_group(memoryController   FILES ${MEMORYCTRL})
source_group(displayController  FILES ${DISPLAYCTRL})
source_group(fragmentOperator   FILES ${FRAGMENTOPERATOR})
source_group(clipper            FILES ${CLIPPER})
source_group(primitiveAssembler FILES ${PRIMITIVEASM})
source_group(rasterizer         FILES ${RASTERIZER})
source_group(streamController   FILES ${STREAMER})
source_group(unifiedShader      FILES ${UNIFIEDSHADER})
source_group(externalSrc        FILES ${EXTERNALSRC})
source_group(top                FILES ${TOP})

target_sources(perfmodel PUBLIC ${BASE}
                              ${COMPRESS}
                              ${CACHE}
                              ${COMMANDPROC}
                              ${MEMORYCTRL}
                              ${DISPLAYCTRL}
                              ${FRAGMENTOPERATOR}
                              ${CLIPPER}
                              ${PRIMITIVEASM}
                              ${RASTERIZER}
                              ${STREAMER}
                              ${UNIFIEDSHADER}
                              ${EXTERNALSRC}
                              ${TOP})

target_include_directories(perfmodel PUBLIC ${CMAKE_SOURCE_DIR}/arch/common
                                          ${CMAKE_SOURCE_DIR}/arch/common/params
                                          ${CMAKE_SOURCE_DIR}/arch/bhavmodel/common/base
                                          ${CMAKE_SOURCE_DIR}/arch/bhavmodel/common/math
                                          ${CMAKE_SOURCE_DIR}/arch/bhavmodel/Rasterizer
                                          ${CMAKE_SOURCE_DIR}/arch/bhavmodel/Clipper
                                          ${CMAKE_SOURCE_DIR}/arch/bhavmodel/FragmentOperator
                                          ${CMAKE_SOURCE_DIR}/arch/perfmodel/common/base
                                          ${CMAKE_SOURCE_DIR}/arch/perfmodel/common/compress
                                          ${CMAKE_SOURCE_DIR}/arch/perfmodel/CacheSystem
                                          ${CMAKE_SOURCE_DIR}/arch/perfmodel/Clipper
                                          ${CMAKE_SOURCE_DIR}/arch/perfmodel/CommandProcessor
                                          ${CMAKE_SOURCE_DIR}/arch/perfmodel/DisplayController
                                          ${CMAKE_SOURCE_DIR}/arch/perfmodel/FragmentOperator
                                          ${CMAKE_SOURCE_DIR}/arch/perfmodel/MemoryController
                                          ${CMAKE_SOURCE_DIR}/arch/perfmodel/MemoryControllerV2
                                          ${CMAKE_SOURCE_DIR}/arch/perfmodel/PrimitiveAssembler
                                          ${CMAKE_SOURCE_DIR}/arch/perfmodel/Rasterizer
                                          ${CMAKE_SOURCE_DIR}/arch/perfmodel/UnifiedShader
                                          ${CMAKE_SOURCE_DIR}/arch/perfmodel/StreamController
                                          ${CMAKE_SOURCE_DIR}/arch/utils

                                          ${CMAKE_SOURCE_DIR}/driver/utils               # zfstream.h
                                          # for TraceDriverMeta.cpp
                                          ${CMAKE_SOURCE_DIR}/driver/utils/GL            # glAll.h
                                          ${CMAKE_SOURCE_DIR}/driver/utils/TraceReader   # GLExec.h
                                          ${CMAKE_SOURCE_DIR}/driver/utils/OGLApiCodeGen # APICall.gen
                                          ${CMAKE_SOURCE_DIR}/driver/utils/TraceDriver# TraceDriverBase.h
                                          ${CMAKE_SOURCE_DIR}/driver/utils/MetaTraceGenerator/traceExtractor  # RegisterWriteBufferMeta.h
                                          ${CMAKE_SOURCE_DIR}/driver/utils/MetaTraceGenerator
                                          ${CMAKE_SOURCE_DIR}/driver/hal     # HAL.h
                          )
