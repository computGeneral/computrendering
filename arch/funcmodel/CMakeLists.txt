add_library(CG1CMDL)

set_target_properties(CG1CMDL PROPERTIES FOLDER arch)

set( BASE
    common/base/cmMduBase.h
    common/base/cmGPUSignal.h
    common/base/cmMultiClockMdu.h
    common/base/cmSignalBinder.h
    common/base/cmStatistic.h
    common/base/cmStatisticsManager.h
    common/base/cmtoolsQueue.h
    common/base/cmMduBase.cpp
    common/base/cmGPUSignal.cpp
    common/base/cmMultiClockMdu.cpp
    common/base/cmSignalBinder.cpp
    common/base/cmStatistic.cpp
    common/base/cmStatisticsManager.cpp
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
    CacheSystem/cmColorCacheV2.h
    CacheSystem/cmFetchCache.h
    CacheSystem/cmFetchCache64.h
    CacheSystem/cmInputCache.h
    CacheSystem/cmROPCache.h
    CacheSystem/cmTextureCache.h
    CacheSystem/cmTextureCacheGen.h
    CacheSystem/cmTextureCacheL2.h
    CacheSystem/cmZCacheV2.h
    CacheSystem/cmColorCacheV2.cpp
    CacheSystem/cmFetchCache.cpp
    CacheSystem/cmFetchCache64.cpp
    CacheSystem/cmInputCache.cpp
    CacheSystem/cmROPCache.cpp
    CacheSystem/cmTextureCache.cpp
    CacheSystem/cmTextureCacheGen.cpp
    CacheSystem/cmTextureCacheL2.cpp
    CacheSystem/cmZCacheV2.cpp
)

set( CLIPPER
    Clipper/cmClipper.h
    Clipper/cmClipperCommand.h
    Clipper/cmClipperStateInfo.h
    Clipper/cmClipperStatusInfo.h
    Clipper/cmClipper.cpp
    Clipper/cmClipperCommand.cpp
    Clipper/cmClipperStateInfo.cpp
    Clipper/cmClipperStatusInfo.cpp
)

set( COMMANDPROC
    ../common/MetaStream.h
    ../common/MetaStream.cpp
    CommandProcessor/cmCommandProcessor.h
    CommandProcessor/cmCommandProcessor.cpp
)

set( DISPLAYCTRL
    DisplayController/cmBlitter.h
    DisplayController/cmDisplayController.h
    DisplayController/cmWakeUpQueue.h
    DisplayController/cmBlitter.cpp
    DisplayController/cmDisplayController.cpp
)

set( FRAGMENTOPERATOR
    FragmentOperator/cmBlendOperation.h
    FragmentOperator/cmColorBlockStateInfo.h
    FragmentOperator/cmColorWriteV2.h
    FragmentOperator/cmGenericROP.h
    FragmentOperator/cmROPOperation.h
    FragmentOperator/cmROPStatusInfo.h
    FragmentOperator/cmZOperation.h
    FragmentOperator/cmZStencilStatusInfo.h
    FragmentOperator/cmZStencilTestV2.h
    FragmentOperator/cmBlendOperation.cpp
    FragmentOperator/cmColorBlockStateInfo.cpp
    FragmentOperator/cmColorWriteV2.cpp
    FragmentOperator/cmGenericROP.cpp
    FragmentOperator/cmROPOperation.cpp
    FragmentOperator/cmROPStatusInfo.cpp
    FragmentOperator/cmZOperation.cpp
    FragmentOperator/cmZStencilStatusInfo.cpp
    FragmentOperator/cmZStencilTestV2.cpp
)

set( MEMORYCTRL
    MemoryControllerV2/cmBankQueueScheduler.h
    MemoryControllerV2/cmBankSelectionPolicy.h
    MemoryControllerV2/cmChannelScheduler.h
    MemoryControllerV2/cmChannelTransaction.h
    MemoryControllerV2/cmDDRBank.h
    MemoryControllerV2/cmDDRBurst.h
    MemoryControllerV2/cmDDRCommand.h
    MemoryControllerV2/cmDDRModule.h
    MemoryControllerV2/cmDDRModuleState.h
    MemoryControllerV2/cmDependencyBuffer.h
    MemoryControllerV2/cmDependencyQueue.h
    MemoryControllerV2/cmFifoScheduler.h
    MemoryControllerV2/cmFifoSchedulerBase.h
    MemoryControllerV2/cmGPUMemorySpecs.h
    MemoryControllerV2/cmMCSplitter.h
    MemoryControllerV2/cmMCSplitter2.h
    MemoryControllerV2/cmMemoryControllerV2.h
    MemoryControllerV2/cmMemoryRequest.h
    MemoryControllerV2/cmMemoryRequestSplitter.h
    MemoryControllerV2/cmMemoryTraceRecorder.h
    MemoryControllerV2/cmRWFifoScheduler.h
    MemoryControllerV2/cmSchedulerSelector.h
    MemoryControllerV2/cmSwitchOperationMode.h
    MemoryController/cmMemoryController.h
    MemoryController/cmMemoryControllerCommand.h
    MemoryController/cmMemoryControllerDefs.h
    MemoryController/cmMemorySpace.h
    MemoryController/cmMemoryTransaction.h
    MemoryControllerV2/cmBankQueueScheduler.cpp
    MemoryControllerV2/cmBankSelectionPolicy.cpp
    MemoryControllerV2/cmChannelScheduler.cpp
    MemoryControllerV2/cmChannelTransaction.cpp
    MemoryControllerV2/cmDDRBank.cpp
    MemoryControllerV2/cmDDRBurst.cpp
    MemoryControllerV2/cmDDRCommand.cpp
    MemoryControllerV2/cmDDRModule.cpp
    MemoryControllerV2/cmDDRModuleState.cpp
    MemoryControllerV2/cmDependencyBuffer.cpp
    MemoryControllerV2/cmDependencyQueue.cpp
    MemoryControllerV2/cmFifoScheduler.cpp
    MemoryControllerV2/cmFifoSchedulerBase.cpp
    MemoryControllerV2/cmMCSplitter.cpp
    MemoryControllerV2/cmMCSplitter2.cpp
    MemoryControllerV2/cmMemoryControllerV2.cpp
    MemoryControllerV2/cmMemoryRequest.cpp
    MemoryControllerV2/cmMemoryRequestSplitter.cpp
    MemoryControllerV2/cmMemoryTraceRecorder.cpp
    MemoryControllerV2/cmRWFifoScheduler.cpp
    MemoryControllerV2/cmSchedulerSelector.cpp
    MemoryControllerV2/cmSwitchOperationMode.cpp
    MemoryController/cmMemoryController.cpp
    MemoryController/cmMemoryControllerCommand.cpp
    MemoryController/cmMemoryTransaction.cpp
)

set( PRIMITIVEASM
    PrimitiveAssembler/cmPrimitiveAssembly.h
    PrimitiveAssembler/cmPrimitiveAssemblyCommand.h
    PrimitiveAssembler/cmPrimitiveAssemblyInput.h
    PrimitiveAssembler/cmPrimitiveAssemblyRequest.h
    PrimitiveAssembler/cmPrimitiveAssemblyStateInfo.h
    PrimitiveAssembler/cmPrimitiveAssembly.cpp
    PrimitiveAssembler/cmPrimitiveAssemblyCommand.cpp
    PrimitiveAssembler/cmPrimitiveAssemblyInput.cpp
    PrimitiveAssembler/cmPrimitiveAssemblyRequest.cpp
    PrimitiveAssembler/cmPrimitiveAssemblyStateInfo.cpp
)

set( RASTERIZER
    Rasterizer/cmFFIFOStateInfo.h
    Rasterizer/cmFragmentFIFO.h
    Rasterizer/cmFragmentInput.h
    Rasterizer/cmHierarchicalZ.h
    Rasterizer/cmHZAccess.h
    Rasterizer/cmHZStateInfo.h
    Rasterizer/cmHZUpdate.h
    Rasterizer/cmInterpolator.h
    Rasterizer/cmRasterizer.h
    Rasterizer/cmRasterizerCommand.h
    Rasterizer/cmRasterizerState.h
    Rasterizer/cmRasterizerStateInfo.h
    Rasterizer/cmTileEvaluatorStateInfo.h
    Rasterizer/cmTileInput.h
    Rasterizer/cmTriangleSetup.h
    Rasterizer/cmTriangleSetupInput.h
    Rasterizer/cmTriangleSetupOutput.h
    Rasterizer/cmTriangleSetupRequest.h
    Rasterizer/cmTriangleSetupStateInfo.h
    Rasterizer/cmTriangleTraversal.h
    Rasterizer/cmFFIFOStateInfo.cpp
    Rasterizer/cmFragmentFIFO.cpp
    Rasterizer/cmFragmentInput.cpp
    Rasterizer/cmHierarchicalZ.cpp
    Rasterizer/cmHZAccess.cpp
    Rasterizer/cmHZStateInfo.cpp
    Rasterizer/cmHZUpdate.cpp
    Rasterizer/cmInterpolator.cpp
    Rasterizer/cmRasterizer.cpp
    Rasterizer/cmRasterizerCommand.cpp
    Rasterizer/cmRasterizerStateInfo.cpp
    Rasterizer/cmTileEvaluatorStateInfo.cpp
    Rasterizer/cmTileInput.cpp
    Rasterizer/cmTriangleSetup.cpp
    Rasterizer/cmTriangleSetupInput.cpp
    Rasterizer/cmTriangleSetupOutput.cpp
    Rasterizer/cmTriangleSetupRequest.cpp
    Rasterizer/cmTriangleSetupStateInfo.cpp
    Rasterizer/cmTriangleTraversal.cpp
)

set( UNIFIEDSHADER
    UnifiedShader/cmConsumerStateInfo.h
    UnifiedShader/cmFilterOperation.h
    UnifiedShader/cmShaderArchParam.h
    UnifiedShader/cmShaderCommand.h
    UnifiedShader/cmShaderCommon.h
    UnifiedShader/cmShaderDecodeCommand.h
    UnifiedShader/cmShaderDecodeExecute.h
    UnifiedShader/cmShaderDecodeStateInfo.h
    UnifiedShader/cmShaderExecInstruction.h
    UnifiedShader/cmShaderFetch.h
    UnifiedShader/cmShaderInput.h
    UnifiedShader/cmShaderState.h
    UnifiedShader/cmShaderStateInfo.h
    UnifiedShader/cmTextureRequest.h
    UnifiedShader/cmTextureResult.h
    UnifiedShader/cmTextureProcessor.h
    UnifiedShader/cmTextureUnitStateInfo.h
    UnifiedShader/cmInstructionFetchVector.h
    UnifiedShader/cmShaderDecodeExecuteVector.h
    UnifiedShader/cmShaderFetchVector.h
    UnifiedShader/cmUnifiedShader.h
    UnifiedShader/cmConsumerStateInfo.cpp
    UnifiedShader/cmFilterOperation.cpp
    UnifiedShader/cmShaderArchParam.cpp
    UnifiedShader/cmShaderCommand.cpp
    UnifiedShader/cmShaderDecodeCommand.cpp
    UnifiedShader/cmShaderDecodeExecute.cpp
    UnifiedShader/cmShaderDecodeStateInfo.cpp
    UnifiedShader/cmShaderExecInstruction.cpp
    UnifiedShader/cmShaderFetch.cpp
    UnifiedShader/cmShaderInput.cpp
    UnifiedShader/cmShaderStateInfo.cpp
    UnifiedShader/cmTextureRequest.cpp
    UnifiedShader/cmTextureResult.cpp
    UnifiedShader/cmTextureProcessor.cpp
    UnifiedShader/cmTextureUnitStateInfo.cpp
    UnifiedShader/cmInstructionFetchVector.cpp
    UnifiedShader/cmShaderDecodeExecuteVector.cpp
    UnifiedShader/cmShaderFetchVector.cpp
    UnifiedShader/cmUnifiedShader.cpp
)

set( STREAMER
    StreamController/cmStreamer.h
    StreamController/cmStreamerCommand.h
    StreamController/cmStreamerCommit.h
    StreamController/cmStreamerControlCommand.h
    StreamController/cmStreamerFetch.h
    StreamController/cmStreamerLoader.h
    StreamController/cmStreamerOutputCache.h
    StreamController/cmStreamerStateInfo.h
    StreamController/cmStreamer.cpp
    StreamController/cmStreamerCommand.cpp
    StreamController/cmStreamerCommit.cpp
    StreamController/cmStreamerControlCommand.cpp
    StreamController/cmStreamerFetch.cpp
    StreamController/cmStreamerLoader.cpp
    StreamController/cmStreamerOutputCache.cpp
    StreamController/cmStreamerStateInfo.cpp
)

set( TOP
    cmGpuTop.h
    cmGpuTop.cpp
    CG1CMDL.h
    CG1CMDL.cpp)

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

target_sources(CG1CMDL PUBLIC ${BASE}
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

target_include_directories(CG1CMDL PUBLIC ${CMAKE_SOURCE_DIR}/arch/common
                                          ${CMAKE_SOURCE_DIR}/arch/common/params
                                          ${CMAKE_SOURCE_DIR}/arch/bhavmodel/common/base
                                          ${CMAKE_SOURCE_DIR}/arch/bhavmodel/common/math
                                          ${CMAKE_SOURCE_DIR}/arch/bhavmodel/Rasterizer
                                          ${CMAKE_SOURCE_DIR}/arch/bhavmodel/Clipper
                                          ${CMAKE_SOURCE_DIR}/arch/bhavmodel/FragmentOperator
                                          ${CMAKE_SOURCE_DIR}/arch/funcmodel/common/base
                                          ${CMAKE_SOURCE_DIR}/arch/funcmodel/common/compress
                                          ${CMAKE_SOURCE_DIR}/arch/funcmodel/CacheSystem
                                          ${CMAKE_SOURCE_DIR}/arch/funcmodel/Clipper
                                          ${CMAKE_SOURCE_DIR}/arch/funcmodel/CommandProcessor
                                          ${CMAKE_SOURCE_DIR}/arch/funcmodel/DisplayController
                                          ${CMAKE_SOURCE_DIR}/arch/funcmodel/FragmentOperator
                                          ${CMAKE_SOURCE_DIR}/arch/funcmodel/MemoryController
                                          ${CMAKE_SOURCE_DIR}/arch/funcmodel/MemoryControllerV2
                                          ${CMAKE_SOURCE_DIR}/arch/funcmodel/PrimitiveAssembler
                                          ${CMAKE_SOURCE_DIR}/arch/funcmodel/Rasterizer
                                          ${CMAKE_SOURCE_DIR}/arch/funcmodel/UnifiedShader
                                          ${CMAKE_SOURCE_DIR}/arch/funcmodel/StreamController
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
