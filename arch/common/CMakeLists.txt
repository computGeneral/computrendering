add_library(archcommon)

set_target_properties(archcommon PROPERTIES FOLDER arch)

target_sources( archcommon
PUBLIC
    BitStream.cpp
    BitStream.h
    BitStreamReader.cpp
    BitStreamReader.h
    BitStreamWriter.cpp
    BitStreamWriter.h
    DynamicObject.cpp
    DynamicObject.h
    ImageSaver.cpp
    ImageSaver.h
    DynamicMemoryOpt.cpp
    DynamicMemoryOpt.h
    Parser.cpp
    Parser.h
    Profiler.cpp
    Profiler.h
    Vec4FP32.cpp
    Vec4FP32.h
    Vec4Int.cpp
    Vec4Int.h
    support.cpp
    support.h
    GPUType.h
    RegStreamMultiprocessor.h
    RegGpu.hpp
    RegGpu.cpp
    SupportMacro.h
    params/param_loader.cpp
    params/param_loader.hpp
)

target_include_directories(archcommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                            ${CMAKE_CURRENT_SOURCE_DIR}/params
                                            ${CMAKE_SOURCE_DIR}/arch/utils)

if(NOT WIN32)
    find_package(PNG)
    if(PNG_FOUND)
        target_include_directories(archcommon PRIVATE ${PNG_INCLUDE_DIRS})
        target_link_libraries(archcommon PUBLIC ${PNG_LIBRARIES})
    endif()
endif()
