add_library(OGL2)

set_target_properties(OGL2 PROPERTIES FOLDER driver/ogl)

FILE(GLOB OGL2SRC "*.h" "*.cpp")
source_group(ogl2 FILES ${OGL2SRC})
FILE(GLOB OGL2ARBP "ARBProgramObject/*.h" "ARBProgramObject/*.cpp")
source_group(ogl2Arbp FILES ${OGL2ARBP})
FILE(GLOB OGL2BASE "OGLBaseObject/*.h" "OGLBaseObject/*.cpp")
source_group(ogl2BaseObject FILES ${OGL2BASE})
FILE(GLOB OGL2BUF "OGLBufferObject/*.h" "OGLBufferObject/*.cpp")
source_group(ogl2BufferObject FILES ${OGL2BUF})
FILE(GLOB OGL2TEX "OGLTextureObject/*.h" "OGLTextureObject/*.cpp")
source_group(ogl2TextureObject FILES ${OGL2TEX})

FILE(GLOB OGL14SRC "../OGL14/*.h" "../OGL14/*.cpp")
source_group(ogl14 FILE ${OGL14SRC})
FILE(GLOB OGL14ARBP "../OGL14/ARBP/*.h" "../OGL14/ARBP/*.cpp"
                    "../OGL14/ARBP/VertexProgram/*.h" "../OGL14/ARBP/VertexProgram/*.cpp"
                    "../OGL14/ARBP/FragmentProgram/*.h" "../OGL14/ARBP/FragmentProgram/*.cpp")
source_group(ogl14Arbp FILE ${OGL14ARBP})
FILE(GLOB OGL14BASE "../OGL14/GLObject/*.h" "../OGL14/GLObject/*.cpp")
source_group(ogl14BaseObject FILES ${OGL14BASE})
FILE(GLOB OGL14BUF "../OGL14/BufferObject/*.h" "../OGL14/BufferObject/*.cpp")
source_group(ogl14BufferObject FILES ${OGL14BUF})
FILE(GLOB OGL14TEX "../OGL14/Texture/*.h" "../OGL14/Texture/*.cpp")
source_group(ogl14TextureObject FILES ${OGL14TEX})


target_sources(OGL2 PUBLIC ${OGL2SRC}
                           ${OGL2ARBP}
                           ${OGL2BASE}
                           ${OGL2BUF}
                           ${OGL2TEX}
                           ${OGL14SRC}
                           ${OGL14ARBP}
                           ${OGL14BASE}
                           ${OGL14BUF}
                           ${OGL14TEX}
)

target_include_directories(OGL2 PUBLIC ${CMAKE_SOURCE_DIR}/arch/common
                                       ${CMAKE_SOURCE_DIR}/arch/behvmodel/common/math
                                       ${CMAKE_SOURCE_DIR}/arch/behvmodel/Rasterizer
                                       ${CMAKE_SOURCE_DIR}/arch/behvmodel/UnifiedShader
                                       ${CMAKE_SOURCE_DIR}/arch/funcmodel/MemoryController
                                       ${CMAKE_SOURCE_DIR}/arch/funcmodel/CommandProcessor
                                       ${CMAKE_SOURCE_DIR}/arch/funcmodel/UnifiedShader
                                       ${CMAKE_SOURCE_DIR}/driver/utils
                                       ${CMAKE_SOURCE_DIR}/driver/utils/OGLApiCodeGen
                                       ${CMAKE_SOURCE_DIR}/driver/hal
                                       ${CMAKE_SOURCE_DIR}/driver/gal/GAL/Interface
                                       ${CMAKE_SOURCE_DIR}/driver/gal/GALx/Interface
                                       ${CMAKE_SOURCE_DIR}/driver/ogl/inc
                                       ${CMAKE_SOURCE_DIR}/driver/ogl/OGL2
                                       ${CMAKE_SOURCE_DIR}/driver/ogl/OGL2/ARBProgramObject
                                       ${CMAKE_SOURCE_DIR}/driver/ogl/OGL2/OGLBaseObject
                                       ${CMAKE_SOURCE_DIR}/driver/ogl/OGL2/OGLBufferObject
                                       ${CMAKE_SOURCE_DIR}/driver/ogl/OGL2/OGLTextureObject
                                       ${CMAKE_SOURCE_DIR}/driver/ogl/OGL14
                                       ${CMAKE_SOURCE_DIR}/driver/ogl/OGL14/ARBP
                                       ${CMAKE_SOURCE_DIR}/driver/ogl/OGL14/GLObject
                                       ${CMAKE_SOURCE_DIR}/driver/ogl/OGL14/BufferObject
                                       ${CMAKE_SOURCE_DIR}/driver/ogl/OGL14/Texture
                          )

execute_process(
                   COMMAND
                   flex -Pvp1
                        -t -8  ${CMAKE_CURRENT_SOURCE_DIR}/../OGL14/ARBP/VertexProgram/VP1Lexic.l
                   OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../OGL14/ARBP/VertexProgram/VP1Flex.gen
)
execute_process( COMMAND sed -i "42i #ifndef WIN32" ${CMAKE_CURRENT_SOURCE_DIR}/../OGL14/ARBP/VertexProgram/VP1Flex.gen)
execute_process( COMMAND sed -i "44i #endif"      ${CMAKE_CURRENT_SOURCE_DIR}/../OGL14/ARBP/VertexProgram/VP1Flex.gen)

execute_process(
                   COMMAND
                   flex -Pfp1
                        -t -8  ${CMAKE_CURRENT_SOURCE_DIR}/../OGL14/ARBP/FragmentProgram/FP1Lexic.l
                   OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../OGL14/ARBP/FragmentProgram/FP1Flex.gen
)
execute_process( COMMAND sed -i "42i #ifndef WIN32" ${CMAKE_CURRENT_SOURCE_DIR}/../OGL14/ARBP/FragmentProgram/FP1Flex.gen)
execute_process( COMMAND sed -i "44i #endif"      ${CMAKE_CURRENT_SOURCE_DIR}/../OGL14/ARBP/FragmentProgram/FP1Flex.gen)

execute_process(
                   COMMAND
                   bison -p vp1
                         -t ${CMAKE_CURRENT_SOURCE_DIR}/../OGL14/ARBP/VertexProgram/VP1Grammar.y
                         -o ${CMAKE_CURRENT_SOURCE_DIR}/../OGL14/ARBP/VertexProgram/VP1Bison.gen
)
execute_process(
                   COMMAND
                   bison -p fp1
                         -t ${CMAKE_CURRENT_SOURCE_DIR}/../OGL14/ARBP/FragmentProgram/FP1Grammar.y
                         -o ${CMAKE_CURRENT_SOURCE_DIR}/../OGL14/ARBP/FragmentProgram/FP1Bison.gen

)
