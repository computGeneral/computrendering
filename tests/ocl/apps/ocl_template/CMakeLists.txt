# version requirement
cmake_minimum_required(VERSION 2.8)

# project information
set(full_folder_name ${CMAKE_CURRENT_SOURCE_DIR})
get_filename_component(project_name ${full_folder_name} NAME)
project(${project_name})

file(GLOB_RECURSE APP_SOURCE *.cpp)
file(GLOB_RECURSE APP_HEADER *.h)

include_directories(${SOLUTION_DEPENDENCY_DIR})
FOREACH(subdir ${APP_INCLUDE_DIR})
	include_directories(${subdir})
ENDFOREACH()

link_directories(${OPENCL_LIB_DIR})
link_directories(${GFLAGS_LIB_DIR})
add_executable(${project_name} ${APP_SOURCE} ${APP_HEADER})

#target_link_libraries(${project_name} OpenCL gflags_nothreads utils)
target_link_libraries(${project_name} OpenCL utils)

set_target_properties(${project_name} PROPERTIES FOLDER apps)



