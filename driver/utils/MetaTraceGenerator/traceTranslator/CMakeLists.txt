project(MetaTranslator)

set_property(GLOBAL PROPERTY CORE_MODULES)

set(EXTRACTOR ${CMAKE_SOURCE_DIR}/traceTranslator/Api2MetaTranslator.cpp
              ${CMAKE_SOURCE_DIR}/traceTranslator/Api2MetaTranslator.h
              ${CMAKE_SOURCE_DIR}/../../../arch/utils/ConfigLoader.cpp
              ${CMAKE_SOURCE_DIR}/../../../arch/utils/ConfigLoader.h
              ${CMAKE_SOURCE_DIR}/../TraceDriver/TraceDriverD3D.cpp
              ${CMAKE_SOURCE_DIR}/../TraceDriver/TraceDriverD3D.h
              ${CMAKE_SOURCE_DIR}/MetaStreamTrace.h
    )

add_executable(MetaTranslator ${EXTRACTOR})

target_link_libraries(MetaTranslator PUBLIC d3d9
                                            D3DTraceCore
                                            CG1BMDL
                                            ogl
                                            CG1CMDL
                                            CG1COMMON)

target_include_directories(MetaTranslator PUBLIC
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

#target_link_libraries(MetaTranslator PUBLIC
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


