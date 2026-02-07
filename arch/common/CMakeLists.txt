add_library(CG1Common)

set_target_properties(CG1Common PROPERTIES FOLDER arch)

target_sources( CG1Common
PUBLIC
    BitStream.cpp
    BitStream.h
    BitStreamReader.cpp
    BitStreamReader.h
    BitStreamWriter.cpp
    BitStreamWriter.h
    DynamicObject.cpp
    DynamicObject.h
    GlobalProfiler.cpp
    GlobalProfiler.h
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
)

target_include_directories(CG1Common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
