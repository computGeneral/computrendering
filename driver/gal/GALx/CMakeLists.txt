add_library(GALx)

set_target_properties(GALx PROPERTIES FOLDER driver/gal)

FILE(GLOB GALIF "interface/*.h")
source_group(interface FILES ${GALIF})

FILE(GLOB GALIMP "Implementation/*.h" "Implementation/*.cpp")
source_group(implementation FILES ${GALIMP})

FILE(GLOB ARBCOMP "Implementation/ARBCompilers/*.h" 
                  "Implementation/ARBCompilers/*.cpp"
                  "Implementation/ARBCompilers/FragmentProgram/*.h" 
                  "Implementation/ARBCompilers/FragmentProgram/*.cpp"
                  "Implementation/ARBCompilers/VertexProgram/*.h" 
                  "Implementation/ARBCompilers/VertexProgram/*.cpp"
                  )
source_group(arbCompiler FILES ${ARBCOMP})

FILE(GLOB FPEMUL "Implementation/FPEmulation/*.h" "Implementation/FPEmulation/*.cpp")
source_group(FPEmulation FILES ${FPEMUL})

target_sources(GALx PUBLIC ${GALIF} ${GALIMP} ${ARBCOMP} ${FPEMUL})

target_include_directories(GALx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Interface
                                       ${CMAKE_CURRENT_SOURCE_DIR}/Implementation
                                       ${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ARBCompilers
                                       ${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ARBCompilers/FragmentProgram
                                       ${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ARBCompilers/VertexProgram
                                       ${CMAKE_CURRENT_SOURCE_DIR}/Implementation/FPEmulation
                                       ${CMAKE_SOURCE_DIR}/driver/gal/GAL/Interface
                                       ${CMAKE_SOURCE_DIR}/driver/gal/GAL/Implementation
                                       ${CMAKE_SOURCE_DIR}/driver/ogl/inc
                                       ${CMAKE_SOURCE_DIR}/arch/common
                                       ${CMAKE_SOURCE_DIR}/arch/bhavmodel/UnifiedShader
                          )

execute_process(
                   COMMAND 
                   flex -Pgalxvp1
                        -t -8 ${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ARBCompilers/VertexProgram/GALxVP1Lexic.l
                   OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ARBCompilers/VertexProgram/GALxVP1Flex.gen
)
# Cross-platform replacement for sed: guard #include <unistd.h> with #ifndef _WIN32
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ARBCompilers/VertexProgram/GALxVP1Flex.gen _FLEX_CONTENT)
string(REPLACE "#include <unistd.h>" "#ifndef _WIN32\n#include <unistd.h>\n#endif" _FLEX_CONTENT "${_FLEX_CONTENT}")
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ARBCompilers/VertexProgram/GALxVP1Flex.gen "${_FLEX_CONTENT}")

execute_process(
                   COMMAND 
                   flex -Pgalxfp1 
                        -t -8 ${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ARBCompilers/FragmentProgram/GALxFP1Lexic.l
                   OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ARBCompilers/FragmentProgram/GALxFP1Flex.gen
)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ARBCompilers/FragmentProgram/GALxFP1Flex.gen _FLEX_CONTENT)
string(REPLACE "#include <unistd.h>" "#ifndef _WIN32\n#include <unistd.h>\n#endif" _FLEX_CONTENT "${_FLEX_CONTENT}")
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ARBCompilers/FragmentProgram/GALxFP1Flex.gen "${_FLEX_CONTENT}")

execute_process(
                   COMMAND
                   bison -p galxvp1 
                         -t ${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ARBCompilers/VertexProgram/GALxVP1Grammar.y
                         -o ${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ARBCompilers/VertexProgram/GALxVP1Bison.gen
)
execute_process(
                   COMMAND 
                   bison -p galxfp1 
                         -t ${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ARBCompilers/FragmentProgram/GALxFP1Grammar.y
                         -o ${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ARBCompilers/FragmentProgram/GALxFP1Bison.gen
               )

#add_custom_command(TARGET GALx PRE_BUILD
#                   COMMAND 
#                   flex -Pgalxvp1
#                        -t -8 "${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ARBCompilers/VertexProgram/GALxVP1Lexic.l"
#                            > "${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ARBCompilers/VertexProgram/GALxVP1Flex.gen"
#                   COMMENT "Creating flex parser for ARB Vertex Program"
#                   COMMAND 
#                   flex -Pgalxfp1 
#                        -t -8 "${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ARBCompilers/FragmentProgram/GALxFP1Lexic.l" 
#                            > "${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ARBCompilers/FragmentProgram/GALxFP1Flex.gen"
#                   COMMENT "Creating flex parser for ARB Fragment Program"
#                   COMMAND 
#                   bison -p galxvp1 
#                         -t "${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ARBCompilers/VertexProgram/GALxVP1Grammar.y"
#                         -o "${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ARBCompilers/VertexProgram/GALxVP1Bison.gen"
#                   COMMENT "Creating grammatic parser for ARB Vertex Program"
#                   COMMAND 
#                   bison -p galxfp1 
#                         -t "${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ARBCompilers/FragmentProgram/GALxFP1Grammar.y"
#                         -o "${CMAKE_CURRENT_SOURCE_DIR}/Implementation/ARBCompilers/FragmentProgram/GALxFP1Bison.gen"
#                   COMMENT "Creating grammatic parser for ARB Fragment Program"
#              )
