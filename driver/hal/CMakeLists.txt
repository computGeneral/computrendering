cmake_minimum_required(VERSION 3.13.1)

add_library(HAL)

set_target_properties(HAL PROPERTIES FOLDER driver/hal)

FILE(GLOB HALSRC "*.H" "*.cpp")

source_group(hal FILES ${HALSRC})

target_sources(HAL PUBLIC ${HALSRC})

target_include_directories(HAL PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                      ${CMAKE_SOURCE_DIR}/arch/common
                                      ${CMAKE_SOURCE_DIR}/arch/behvmodel
                                      ${CMAKE_SOURCE_DIR}/arch/behvmodel/TextureProcessor
                                      ${CMAKE_SOURCE_DIR}/arch/behvmodel/UnifiedShader
                                      ${CMAKE_SOURCE_DIR}/arch/funcmodel/base
                                      ${CMAKE_SOURCE_DIR}/arch/funcmodel/CommandProcessor
                                      ${CMAKE_SOURCE_DIR}/arch/funcmodel/MemoryController
                                      ${CMAKE_SOURCE_DIR}/driver/utils
                          )
