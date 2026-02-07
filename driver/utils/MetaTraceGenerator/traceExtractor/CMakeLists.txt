project(MetaExtractor)

set_property(GLOBAL PROPERTY CORE_MODULES)

set(EXTRACTOR ${CMAKE_SOURCE_DIR}/traceExtractor/traceExtractor.cpp
              #${CMAKE_SOURCE_DIR}/traceExtractor/extraceTraceRegion.h
              ${CMAKE_SOURCE_DIR}/traceExtractor/RegisterWriteBufferMeta.cpp
              ${CMAKE_SOURCE_DIR}/traceExtractor/RegisterWriteBufferMeta.h
              ${CMAKE_SOURCE_DIR}/../../../arch/funcmodel/CommandProcessor/MetaStream.cpp
              ${CMAKE_SOURCE_DIR}/../../../arch/funcmodel/CommandProcessor/MetaStream.h
              ${CMAKE_SOURCE_DIR}/../../../arch/common/DynamicObject.cpp
              ${CMAKE_SOURCE_DIR}/../../../arch/common/DynamicMemoryOpt.cpp
              ${CMAKE_SOURCE_DIR}/../../../arch/common/Vec4FP32.cpp
              ${CMAKE_SOURCE_DIR}/../../../arch/common/support.cpp
              ${CMAKE_SOURCE_DIR}/../../../arch/common/DynamicObject.h
              ${CMAKE_SOURCE_DIR}/../../../arch/common/GPUType.h
              ${CMAKE_SOURCE_DIR}/../../../arch/common/DynamicMemoryOpt.h
              ${CMAKE_SOURCE_DIR}/../../../arch/common/Vec4FP32.h
              ${CMAKE_SOURCE_DIR}/../../../arch/common/support.h
#              ${CMAKE_SOURCE_DIR}/../zfstream.cpp
#              ${CMAKE_SOURCE_DIR}/../zfstream.h
    )

add_executable(MetaExtractor ${EXTRACTOR})

include_directories(${CMAKE_SOURCE_DIR}/../../../common/thirdparty/zlib-1.2.13)
link_directories(${CMAKE_SOURCE_DIR}/../../../_BUILD_/common/thirdparty/zlib-1.2.13/Debug)

target_link_libraries(MetaExtractor PUBLIC zlibstaticd)

target_include_directories(MetaExtractor PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/../../../arch/common
        ${CMAKE_SOURCE_DIR}/../../../arch/bhavmodel
        ${CMAKE_SOURCE_DIR}/../../../arch/funcmodel
        ${CMAKE_SOURCE_DIR}/../../../arch/funcmodel/common/base
        ${CMAKE_SOURCE_DIR}/../../../arch/funcmodel/CommandProcessor
        ${CMAKE_SOURCE_DIR}/../../../arch/funcmodel/UnifiedShader
        ${CMAKE_SOURCE_DIR}/../../../arch/funcmodel/StreamController
        ${CMAKE_SOURCE_DIR}/../../../arch/funcmodel/MemoryController
        ${CMAKE_SOURCE_DIR}/../../../arch/funcmodel/MemoryControllerV2
        ${CMAKE_SOURCE_DIR}/../../../arch/funcmodel/Rasterizer
        ${CMAKE_SOURCE_DIR}/../../../arch/funcmodel/CacheSystem
        ${CMAKE_SOURCE_DIR}/../../../arch/funcmodel/FragmentOperator
        ${CMAKE_SOURCE_DIR}/../../../arch/funcmodel/PrimitiveAssembler
        ${CMAKE_SOURCE_DIR}/../../../arch/funcmodel/DisplayController
        ${CMAKE_SOURCE_DIR}/../../../arch/funcmodel/Clipper
        ${CMAKE_SOURCE_DIR}/../../hal
        ${CMAKE_SOURCE_DIR}/../../ogl/OGL14
        ${CMAKE_SOURCE_DIR}/../../ogl/OGL14/ARBP
        ${CMAKE_SOURCE_DIR}/../../ogl/OGL14/ARBP/FragmentProgram
        ${CMAKE_SOURCE_DIR}/../../ogl/OGL14/ARBP/VertexProgram
        ${CMAKE_SOURCE_DIR}/../../ogl/OGL14/GLObject
        ${CMAKE_SOURCE_DIR}/../../ogl/OGL14/BufferObject
        ${CMAKE_SOURCE_DIR}/../../ogl/OGL14/Texture
        ${CMAKE_SOURCE_DIR}/../
        ${CMAKE_SOURCE_DIR}/../TraceReader
        ${CMAKE_SOURCE_DIR}/../OGLApiCodeGen
        ${CMAKE_SOURCE_DIR}/../GL
        )

#target_link_libraries(MetaExtractor PUBLIC
#        
#        )

#add_subdirectory(${CMAKE_SOURCE_DIR}/../../../common/thirdParty/zlib-1.2.13)

add_definitions(
        -DWIN32
        -DZLIB_WINAPI
        -D_DEBUG
        -D_CONSOLE
        -DYY_NEVER_INTERACTIVE
        -DLOAD_JUMPTABLE_STATICALLY
        -DGL_GLEXT_LEGACY
        -DGL_GLEXT_PROTOTYPES
        -D_CRT_SECURE_NO_WARNINGS
        -D_CRT_SECURE_NO_DEPRECATE
  )


