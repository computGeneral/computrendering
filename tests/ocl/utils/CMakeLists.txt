# version requirement
cmake_minimum_required(VERSION 2.8)
set(full_folder_name ${CMAKE_CURRENT_SOURCE_DIR})
get_filename_component(project_name ${full_folder_name} NAME)
project(${project_name})

# compile as a static library
file(GLOB_RECURSE UTIL_SOURCES *.cpp)
file(GLOB_RECURSE UTIL_HEADERS *.hpp)
add_library(${project_name} STATIC ${UTIL_SOURCES} ${UTIL_HEADERS}) 

include_directories(${SOLUTION_DEPENDENCY_DIR})

# set folder in solution
#set_target_properties(${project_name} PROPERTIES FOLDER libs)
