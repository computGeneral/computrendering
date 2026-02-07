cmake_minimum_required(VERSION 3.5.1)
#project(D3DApiCodeGen VERSION 1.0)
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

FILE(GLOB MAINSRC "*.h" "*.cpp")
FILE(GLOB CONFIG  "./Config/*.h" "./Config/*.cpp")
FILE(GLOB PARSER "./Parser/*.h"  "./Parser/*.cpp")
FILE(GLOB GENERATOR "./Generator/*.h"  "./Generator/*.cpp")
FILE(GLOB ITEM "./Items/*.h"  "./Items/*.cpp")
FILE(GLOB UTILITIES "./Utilities/*.h"  "./Utilities/*.cpp")
FILE(GLOB LIBGRETA "./Libgreta/*.h"  "./Libgreta/*.cpp")
FILE(GLOB LIBTINYXML "./Libtinyxml/*.h"  "./Libtinyxml/*.cpp")
source_group(main        FILES ${MAINSRC})
source_group(config      FILES ${CONFIG})
source_group(parser      FILES ${PARSER})
source_group(generator   FILES ${GENERATOR})
source_group(item        FILES ${ITEM})
source_group(utilities   FILES ${UTILITIES})
source_group(LibGreta    FILES ${LIBGRETA})
source_group(LibTinyXML  FILES ${LIBTINYXML})

add_executable(D3DApiCodeGen ${MAINSRC}
                             ${CONFIG}
                             ${PARSER}
                             ${GENERATOR}
                             ${ITEM}
                             ${UTILITIES}
                             ${LIBGRETA}
                             ${LIBTINYXML})

set_target_properties(D3DApiCodeGen PROPERTIES FOLDER driver/utils)

target_include_directories(D3DApiCodeGen PUBLIC
                          ${CMAKE_SOURCE_DIR}/arch/common
                          ${CMAKE_CURRENT_SOURCE_DIR}
                          #${CMAKE_SOURCE_DIR}/driver/utils
                          )

add_custom_command(TARGET D3DApiCodeGen POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E 
                   copy    ${CMAKE_CURRENT_SOURCE_DIR}/Config/D3D9ApiCodeGen.config ${CMAKE_BINARY_DIR}/driver/utils/D3DApiCodeGen/D3D9ApiCodeGen.cfg
                   COMMENT "copy D3DApiCodeGen config file to build dir")

add_custom_command(TARGET D3DApiCodeGen POST_BUILD
                   COMMAND 
                   ${CMAKE_BINARY_DIR}/driver/utils/D3DApiCodeGen/Debug/D3DApiCodeGen.exe
                   COMMENT "Generate D3D9 API related code for CG1GPUSIM project")