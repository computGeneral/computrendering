add_library(ApitraceParser
    ApitraceParser.h
    ApitraceParser.cpp
    ApitraceCallDispatcher.h
    ApitraceCallDispatcher.cpp
)

target_include_directories(ApitraceParser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/arch/common
    ${CMAKE_SOURCE_DIR}/common/thirdparty/snappy-1.1.10
    ${CMAKE_SOURCE_DIR}/driver/utils
    ${CMAKE_SOURCE_DIR}/driver/utils/GL
    ${CMAKE_SOURCE_DIR}/driver/utils/OGLApiCodeGen
    ${CMAKE_SOURCE_DIR}/driver/ogl/OGL2
    ${CMAKE_SOURCE_DIR}/driver/ogl/OGL14
    ${CMAKE_SOURCE_DIR}/driver/ogl/OGL14/ARBP
    ${CMAKE_SOURCE_DIR}/driver/ogl/OGL14/GLObject
    ${CMAKE_SOURCE_DIR}/driver/ogl/OGL14/Texture
    ${CMAKE_SOURCE_DIR}/driver/ogl/OGL14/BufferObject
    ${CMAKE_SOURCE_DIR}/driver/ogl/OGL2/ARBProgramObject
    ${CMAKE_SOURCE_DIR}/driver/ogl/OGL2/OGLBaseObject
    ${CMAKE_SOURCE_DIR}/driver/ogl/OGL2/OGLBufferObject
    ${CMAKE_SOURCE_DIR}/driver/ogl/OGL2/OGLTextureObject
    ${CMAKE_SOURCE_DIR}/driver/gal/GAL/Interface
    ${CMAKE_SOURCE_DIR}/driver/gal/GALx/Interface
    ${CMAKE_SOURCE_DIR}/driver/hal
    ${CMAKE_SOURCE_DIR}/arch/common/params
    ${CMAKE_SOURCE_DIR}/arch/utils
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/UnifiedShader
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/common/base
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/common/math
    ${CMAKE_SOURCE_DIR}/arch/funcmodel/CommandProcessor
    ${CMAKE_SOURCE_DIR}/arch/funcmodel/MemoryController
)

target_link_libraries(ApitraceParser PUBLIC snappy)
