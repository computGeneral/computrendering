PROJECT(test_opencl CXX)

# version requirement
cmake_minimum_required(VERSION 2.8)
# do not change
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
#set(CMAKE_CXX_COMPILER /usr/bin/g++)


#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILD_DIR}/archieve )
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILD_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILD_DIR} )
set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE STRING INTERNAL FORCE )

#MACRO(SUBDIRLIST result curdir)
#  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
#  SET(dirlist "")
#  FOREACH(child ${children})
#    IF(IS_DIRECTORY ${curdir}/${child})
#      LIST(APPEND dirlist ${curdir}/${child})
#    ENDIF()
#  ENDFOREACH()
#  SET(${result} ${dirlist})
#ENDMACRO()
#
#MACRO(SUBDIRLIST_RELATIVE result curdir)
#  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
#  SET(dirlist "")
#  FOREACH(child ${children})
#    IF(IS_DIRECTORY ${curdir}/${child})
#      LIST(APPEND dirlist ${child})
#    ENDIF()
#  ENDFOREACH()
#  SET(${result} ${dirlist})
#ENDMACRO()


#set(OPENCL_DIR /opt/OpenCL-ICD)
#set(GFLAGS_DIR ${CMAKE_CURRENT_BINARY_DIR}/libs/)

set(OCLLIB_DIR ${CMAKE_SOURCE_DIR}/libs/libOpenCL)

set(SOLUTION_DEPENDENCY_DIR ${OCLLIB_DIR}/include ${CMAKE_SOURCE_DIR}/utils CACHE INTERNAL "solution denpendency dir")
set(OPENCL_LIB_DIR ${OCLLIB_DIR}/lib  CACHE INTERNAL "OpenCL library dir")

add_subdirectory(utils)
#add_subdirectory(libs)
#add_subdirectory(apps/ocl_template)
add_subdirectory(apps/cdm_workgroup_issue)

