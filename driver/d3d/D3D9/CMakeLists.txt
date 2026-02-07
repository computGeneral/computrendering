cmake_minimum_required(VERSION 3.5.1)
add_library(D3D9)

set_target_properties(D3D9 PROPERTIES FOLDER driver/d3d)

FILE(GLOB D3D9SRC "*.h" "*.cpp")
source_group(d3d9 FILES ${D3D9SRC})

FILE(GLOB D3DSTATE "D3DState/*.h" "D3DState/*.cpp")
source_group(d3dState FILES ${D3DSTATE})

#FILE(GLOB D3DINTERFACE "D3DInterface/*.h" "D3DInterface/*.cpp")
#source_group(d3dInterface FILES ${D3DINTERFACE})

FILE(GLOB D3DIMPLEMENT "D3DImplement/*.h" "D3DImplement/*.cpp")
source_group(d3dImplement FILES ${D3DIMPLEMENT})
FILE(GLOB D3DIMPLFFGEN  "D3DImplement/FFGenerator/*.h" "D3DImplement/FFGenerator/*.cpp")
source_group(d3dImplement/ffgen FILES ${D3DIMPLFFGEN})
FILE(GLOB D3DIMPLSHADERTRANS "D3DImplement/ShaderTranslator/*.h" "D3DImplement/ShaderTranslator/*.cpp")
source_group(d3dImplement/shTrans FILES ${D3DIMPLSHADERTRANS})

#FILE(GLOB D3DCONTROLLER "D3DControllers/*.h" "D3DControllers/*.cpp")
#source_group(d3dController FILES ${D3DCONTROLLER})
#FILE(GLOB D3DCTRLFFGEN  "D3DControllers/FFGenerator/*.h" "D3DControllers/FFGenerator/*.cpp")
#source_group(d3dController/ffgen FILES ${D3DCTRLFFGEN})
#FILE(GLOB D3DCTRLSHADERTRANS "D3DControllers/ShaderTranslator/*.h" "D3DControllers/ShaderTranslator/*.cpp")
#source_group(d3dController/shTrans FILES ${D3DCTRLSHADERTRANS})

add_definitions( -D_LIB
                 -DD3D9_WIN32_COMP)

target_sources(D3D9 PUBLIC ${D3D9SRC}
                           ${D3DSTATE}
#                           ${D3DINTERFACE}
                           ${D3DIMPLEMENT}
                           ${D3DIMPLFFGEN}
                           ${D3DIMPLSHADERTRANS}
#                           ${D3DCONTROLLER}
#                           ${D3DCTRLFFGEN}
#                           ${D3DCTRLSHADERTRANS}
)

target_include_directories(D3D9 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                       ${CMAKE_SOURCE_DIR}/arch/common
                                       ${CMAKE_SOURCE_DIR}/arch/behvmodel/common/math
                                       ${CMAKE_SOURCE_DIR}/arch/behvmodel/Rasterizer
                                       ${CMAKE_SOURCE_DIR}/arch/behvmodel/UnifiedShader
                                       ${CMAKE_SOURCE_DIR}/arch/funcmodel/CommandProcessor
                                       ${CMAKE_SOURCE_DIR}/arch/funcmodel/MemoryController
                                       ${CMAKE_SOURCE_DIR}/driver/utils
                                       ${CMAKE_SOURCE_DIR}/driver/d3d/trace/D3DTraceCore
                                       ${CMAKE_SOURCE_DIR}/driver/d3d/inc
                                       ${CMAKE_SOURCE_DIR}/driver/hal
                                       ${CMAKE_SOURCE_DIR}/driver/gal/GAL/Interface
                                       ${CMAKE_CURRENT_SOURCE_DIR}/D3DState
                                       #${CMAKE_CURRENT_SOURCE_DIR}/D3DInterface
                                       ${CMAKE_CURRENT_SOURCE_DIR}/D3DImplement
                                       ${CMAKE_CURRENT_SOURCE_DIR}/D3DImplement/FFGenerator
                                       ${CMAKE_CURRENT_SOURCE_DIR}/D3DImplement/ShaderTranslator
                                       ${CMAKE_CURRENT_SOURCE_DIR}/D3DControllers
                          )