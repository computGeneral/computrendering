cmake_minimum_required(VERSION 3.10)
project(PPMTools)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files for PPMConverter
set(CONVERTER_SOURCES
    main.cpp
    FragmentMapConverter.cpp
    FragmentMapConverter.h
    GroupRemap.cpp
    GroupRemap.h
    LinearRemap.cpp
    LinearRemap.h
    PPMFile.cpp
    PPMFile.h
    PPMInspector.cpp
    PPMInspector.h
)

# Source files for PPMInspector
set(INSPECTOR_SOURCES
    PPMInspect.cpp
    PPMInspector.cpp
    PPMInspector.h
    PPMFile.cpp
    PPMFile.h
)

# Create executables
add_executable(PPMConverter ${CONVERTER_SOURCES})
add_executable(PPMInspect ${INSPECTOR_SOURCES})

# Installation (optional, but good practice)
install(TARGETS PPMConverter PPMInspect RUNTIME DESTINATION bin)
