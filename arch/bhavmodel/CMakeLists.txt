add_library(CG1BMDL)

set_target_properties(CG1BMDL PROPERTIES FOLDER arch)

set(BASE
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/common/base/bmMduBase.h
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/common/base/ValidationInfo.h
)

set(MATH
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/common/math/FixedPoint.h
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/common/math/FixedPoint.cpp
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/common/math/GPUMath.h
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/common/math/GPUMath.cpp
)

set(CLIPPER 
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/Clipper/bmClipper.cpp
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/Clipper/bmClipper.h
)

set(RASTER
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/Rasterizer/bmRasterizer.cpp
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/Rasterizer/bmRasterizer.h
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/Rasterizer/Fragment.cpp
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/Rasterizer/Fragment.h
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/Rasterizer/SetupTriangle.cpp
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/Rasterizer/SetupTriangle.h
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/Rasterizer/Tile.cpp
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/Rasterizer/Tile.h
)

set(TEXTURE
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/TextureProcessor/bmTextureProcessor.cpp
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/TextureProcessor/bmTextureProcessor.h
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/TextureProcessor/PixelMapper.cpp
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/TextureProcessor/PixelMapper.h
)

set(SHADER
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/UnifiedShader/bmUnifiedShader.cpp
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/UnifiedShader/bmUnifiedShader.h
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/UnifiedShader/ShaderInstr.cpp
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/UnifiedShader/ShaderInstr.h
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/UnifiedShader/ShaderOptimization.cpp
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/UnifiedShader/ShaderOptimization.h
)

set(FRAGOP
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/FragmentOperator/bmFragmentOperator.cpp
    ${CMAKE_SOURCE_DIR}/arch/bhavmodel/FragmentOperator/bmFragmentOperator.h
)

FILE(GLOB TOP "*.h" "*.cpp")

source_group(top FILES ${TOP})
source_group(common/base FILES ${BASE})
source_group(common/math FILES ${MATH})
source_group(clipper FILES ${CLIPPER})
source_group(fragmentOperator FILES ${FRAGOP})
source_group(rasterizer FILES ${RASTER})
source_group(unifiedShader FILES ${SHADER})
source_group(textureProcessor FILES ${TEXTURE})

target_sources(CG1BMDL PUBLIC ${BASE}
                              ${MATH}
                              ${CLIPPER}
                              ${FRAGOP}
                              ${RASTER}
                              ${SHADER}
                              ${TEXTURE}
                              ${TOP}
)

target_include_directories(CG1BMDL PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                          ${CMAKE_CURRENT_SOURCE_DIR}/common/base
                                          ${CMAKE_CURRENT_SOURCE_DIR}/common/math
                                          ${CMAKE_CURRENT_SOURCE_DIR}/Clipper
                                          ${CMAKE_CURRENT_SOURCE_DIR}/FragmentOperator
                                          ${CMAKE_CURRENT_SOURCE_DIR}/Rasterizer
                                          ${CMAKE_CURRENT_SOURCE_DIR}/TextureProcessor
                                          ${CMAKE_CURRENT_SOURCE_DIR}/UnifiedShader
                                          ${CMAKE_SOURCE_DIR}/arch/common
                                          ${CMAKE_SOURCE_DIR}/arch/common/params
                                          ${CMAKE_SOURCE_DIR}/arch/funcmodel/CommandProcessor
                                          ${CMAKE_SOURCE_DIR}/arch/funcmodel/MemoryController
                                          ${CMAKE_SOURCE_DIR}/arch/utils
                                          ${CMAKE_SOURCE_DIR}/driver/utils/TraceDriver
                                          ${CMAKE_SOURCE_DIR}/driver/utils/MetaTraceGenerator
                                          ${CMAKE_SOURCE_DIR}/driver/utils
                                          ${CMAKE_SOURCE_DIR}/driver/hal
                          )
