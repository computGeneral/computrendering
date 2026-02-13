cmake_minimum_required(VERSION 3.13.1)
cmake_policy(SET CMP0079 NEW)
cmake_policy(SET CMP0135 NEW) # DOWNLOAD_EXPLICT_TIMESTAMP

project(MetaStreamGenerator VERSION 1.0)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-as-needed" CACHE STRING "" FORCE)
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "[FATAL] Cmake binary must be different to the source dir")
endif()

#### platform setup
if(MSVC)
    if((${CMAKE_GENERATOR} MATCHES "(Win64|x64)") OR (${CMAKE_GENERATOR_PLATFORM} MATCHES "(Win64|x64)"))
        set(MSVC_64BIT Yes)
    else()
        set(MSVC_64BIT No)
    endif()
    set(WIN_ENABLE_VECTOR_EXTENSIONS No CACHE BOOL "Eables MSVC SSE/VAX extensions")
else()
    set(MSVC_64BIT Yes)
endif()

##### Compiler Setup
if(MSVC)
    include(${CMAKE_SOURCE_DIR}/../../../tools/script/CompilerMsvc.cmake)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    include(${CMAKE_SOURCE_DIR}/../../../tools/script/CompilerClang.cmake)
else()
    include(${CMAKE_SOURCE_DIR}/../../../tools/script/CompilerGcc.cmake)
endif()

#### options
set(WARNINGS_AS_ERRORS           Yes     CACHE BOOL   "If set, Warnings are Treated as Errors.")
set(USE_PTHREADS                 No      CACHE BOOL   "If set, Use the pthreads library.")
set(PERFORMANCE_COUNTERS         No      CACHE BOOL   "If set, Performance Counters is defined, and calculated.")
set(POWER_COUNTERS               No      CACHE BOOL   "If set, Performance Counters is defined, and calculated.")
set(SOURCES_RELEASE              No      CACHE BOOL   "If set, Generates and build the release package.")
set(SYSTEMC_VERSION              "2.3.3" CACHE STRING "If set, Generates and build the release package.")
set(SYSTEMC_PROFILER_ENABLED     No      CACHE BOOL   "If set, Generates and build the release package.")

##### include dirs
# module path for 3rd party libs
#list(APPEND CMAKE_MODULE_PATH
#    ${CMAKE_SOURCE_DIR}/thirdparty/zlib-1.2.8
#    )


#include_directories(
#    ${CMAKE_SOURCE_DIR}/arch/common
#    ${CMAKE_SOURCE_DIR}/arch/funcmodel
#    ${CMAKE_SOURCE_DIR}/arch/behvmodel
#    )

#### Definitions
add_definitions(
    -DUNIFIEDSHADER
    -DCG_STREAM_PROCESSOR
    -D_CONSOLE
    -DYY_NEVER_INTERACTIVE
    -DLOAD_JUMPTABLE_STATICALLY
    -DGL_GLEXT_LEGACY
    -DGL_GLEXT_PROTOTYPES
)


##### sub-projects
# tools
#include(ExternalProject)
add_subdirectory(traceExtractor)
#add_subdirectory(traceTranslator)


##### depnedences
#### add explict dependencies for non-library targets, all others will be tracked automatically.

# generated files


##### misc
##
