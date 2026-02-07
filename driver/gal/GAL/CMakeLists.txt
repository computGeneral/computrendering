
add_library(GAL)

set_target_properties(GAL PROPERTIES FOLDER driver/gal)

FILE(GLOB GALIF  ${CMAKE_CURRENT_SOURCE_DIR}/Interface/*.h)
source_group(interface FILES ${GALIF})

FILE(GLOB GALIMP ${CMAKE_CURRENT_SOURCE_DIR}/Implementation/*.h
                 ${CMAKE_CURRENT_SOURCE_DIR}/Implementation/*.cpp)
source_group(implementation FILES ${GALIMP})

FILE(GLOB GALSHADEROPT ${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ShaderOptimization/*.h
                       ${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ShaderOptimization/*.cpp)
source_group(shaderOpt FILES ${GALSHADEROPT})


target_sources(GAL PUBLIC ${GALIF}
                          ${GALIMP}
                          ${GALSHADEROPT})

target_include_directories(GAL PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Interface
                                      ${CMAKE_CURRENT_SOURCE_DIR}/Implementation
                                      ${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ShaderOptimization
                                      ${CMAKE_SOURCE_DIR}/driver/utils
                                      ${CMAKE_SOURCE_DIR}/driver/hal
                                      ${CMAKE_SOURCE_DIR}/arch/common
                                      ${CMAKE_SOURCE_DIR}/arch/bhavmodel
                                      ${CMAKE_SOURCE_DIR}/arch/bhavmodel/UnifiedShader
                                      ${CMAKE_SOURCE_DIR}/arch/bhavmodel/common/base
                                      ${CMAKE_SOURCE_DIR}/arch/bhavmodel/common/math
                                      ${CMAKE_SOURCE_DIR}/arch/funcmodel/CommandProcessor
                                      ${CMAKE_SOURCE_DIR}/arch/funcmodel/MemoryController
                                      ${CMAKE_SOURCE_DIR}/arch/funcmodel/UnifiedShader
                                      ${CMAKE_SOURCE_DIR}/arch/funcmodel/Shader
                                      ${CMAKE_SOURCE_DIR}/arch/funcmodel/common/base
                          )
