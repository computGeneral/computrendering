cmake_minimum_required(VERSION 3.5.1)
add_library(D3DTraceCore)

set_target_properties(D3DTraceCore PROPERTIES FOLDER driver/d3d/trace)

FILE(GLOB D3DTRACE "*.h" "*.cpp")
source_group(d3dTraceCore FILES ${D3DTRACE})

target_sources(D3DTraceCore PUBLIC ${D3DTRACE}
)

target_include_directories(D3DTraceCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                               ${CMAKE_SOURCE_DIR}/arch/common
                                               ${CMAKE_SOURCE_DIR}/driver/trace
                                               ${CMAKE_SOURCE_DIR}/driver/d3d/inc
                                               ${CMAKE_SOURCE_DIR}/driver/utils
                                               ${CMAKE_SOURCE_DIR}/driver/utils/log
                          )