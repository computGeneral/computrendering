
add_library(GAL INTERFACE)

set_target_properties(GAL PROPERTIES FOLDER driver/gal)

FILE(GLOB GALIF  ${CMAKE_CURRENT_SOURCE_DIR}/interface/*.h)
source_group(interface FILES ${GALIF})

FILE(GLOB GALIMP ${CMAKE_CURRENT_SOURCE_DIR}/implementation/*.h
                 ${CMAKE_CURRENT_SOURCE_DIR}/implementation/*.cpp)
source_group(implementation FILES ${GALIMP})

FILE(GLOB GALSHADEROPT ${CMAKE_CURRENT_SOURCE_DIR}/implementation/ShaderOptimization/*.h
                       ${CMAKE_CURRENT_SOURCE_DIR}/implementation/ShaderOptimization/*.cpp)
source_group(shaderOpt FILES ${GALSHADEROPT})


target_sources(GAL PUBLIC ${GALIF}
                          ${GALIMP}
                          ${GALSHADEROPT})

target_include_directories(GAL INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/interface
                                      ${CMAKE_CURRENT_SOURCE_DIR}/implementation
                                      ${CMAKE_CURRENT_SOURCE_DIR}/implementation/ShaderOptimization
                                      ${CMAKE_SOURCE_DIR}/driver/utils
                                      ${CMAKE_SOURCE_DIR}/driver/hal
                                      ${CMAKE_SOURCE_DIR}/arch/common
                                      ${CMAKE_SOURCE_DIR}/arch/behvmodel
                                      ${CMAKE_SOURCE_DIR}/arch/funcmodel/CommandProcessor
                                      ${CMAKE_SOURCE_DIR}/arch/funcmodel/MemoryController
                                      ${CMAKE_SOURCE_DIR}/arch/funcmodel/Shader
                          )
