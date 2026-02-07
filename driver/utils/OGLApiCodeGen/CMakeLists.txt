cmake_minimum_required(VERSION 3.13.1)
#project(OGLApiCodeGen VERSION 1.0)
# we require a number of libraries and defines for this project and all subprojects
# set these things up before adding subdirectories


#set_property(GLOBAL PROPERTY CORE_MODULES)

# external packages required
#find_package(SystemC         REQUIRED)
#find_package(ZLIB            REQUIRED)

# external include dirs
include_directories(
#    ${SYSC_INCLUDE_DIRS}
#    ${ZLIB_INCLUDE_DIRS}
)

#set(CG1GPUSIM_REQUIRED_LIBRARIES
#    ${ZLIB_LIBRARY}
#    ${SYSTEMC_LIBRARY}
#    )

FILE(GLOB CODEGENSRC "*.h" "*.cpp")
FILE(GLOB PARSERSRC "../Parsers/*.h"  "../Parsers/*.cpp")

source_group(codeGen   FILES ${CODEGENSRC})
source_group(parser    FILES ${PARSERSRC})


add_executable(OGLApiCodeGen ${CODEGENSRC}
                             ${PARSERSRC}
                             )

set_target_properties(OGLApiCodeGen PROPERTIES FOLDER driver/utils)

target_include_directories(OGLApiCodeGen PUBLIC
                          ${CMAKE_CURRENT_SOURCE_DIR}/../../../arch/common
                          ${CMAKE_CURRENT_SOURCE_DIR}
                          ${CMAKE_CURRENT_SOURCE_DIR}/../Parsers
                          )

file(WRITE  ${CMAKE_CURRENT_SOURCE_DIR}/OGLApiCodeGen.cfg 
            "inGL=${CMAKE_CURRENT_SOURCE_DIR}/../../../driver/utils/GL\n"
            "inLibFuncsFile=${CMAKE_CURRENT_SOURCE_DIR}/../../../driver/utils/GL/OGL_functions_supported_by_CG1GPU.h\n"
            "inSpecificFile=${CMAKE_CURRENT_SOURCE_DIR}/../../../driver/ogl/trace/GLInterceptor/Specific.h\n"
            "inSpecificStatsFile=${CMAKE_CURRENT_SOURCE_DIR}/../../../driver/ogl/trace/GLInterceptor/SpecificStats.h\n"
            "outGLInterceptor=${CMAKE_CURRENT_SOURCE_DIR}/../../../driver/ogl/trace/GLInterceptor\n"
            "outGLInstrument=${CMAKE_CURRENT_SOURCE_DIR}/../../../driver/ogl/trace/GLInstrument\n"
            "outTraceReader=${CMAKE_CURRENT_SOURCE_DIR}/../../../driver/utils/TraceReader\n"
            "outGen=${CMAKE_CURRENT_SOURCE_DIR}/../../../driver/utils/OGLApiCodeGen\n")

if(MSVC)
    add_custom_command(TARGET OGLApiCodeGen POST_BUILD
                       COMMAND 
                       ${CMAKE_BINARY_DIR}/driver/utils/OGLApiCodeGen/Debug/OGLApiCodeGen.exe 
                       ${CMAKE_CURRENT_SOURCE_DIR}/OGLApiCodeGen.cfg
                       COMMENT "Generate OGL API related code for CG1GPUSIM project")
else()
    add_custom_command(TARGET OGLApiCodeGen POST_BUILD
                       COMMAND 
                       OGLApiCodeGen
                       ${CMAKE_CURRENT_SOURCE_DIR}/OGLApiCodeGen.cfg
                       COMMENT "Generate OGL API related code for CG1GPUSIM project")
endif()
