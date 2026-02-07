
PROJECT(test_opencl CXX)

# version requirement
cmake_minimum_required(VERSION 2.8)
# do not change
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(OCLHEADER_URL https://github.com/KhronosGroup/OpenCL-Headers/archive/refs/tags/v2022.09.30.tar.gz) 
set(OCLHEADER_DEP ${CMAKE_CURRENT_SOURCE_DIR}/libs/libOpenCL/include/CL/cl.h)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${OCLHEADER_DEP}")
    message("Downloading OpenCL-Headers-2022.09.30...")
    file(DOWNLOAD ${OCLHEADER_URL} "${CMAKE_CURRENT_SOURCE_DIR}/OpenCL-Headers-2022.09.30.tar.gz" SHOW_PROGRESS)
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/OpenCL-Headers-2022.09.30.tar.gz WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
endif()
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/OpenCL-Headers-2022.09.30)

set(OCLICD_URL https://github.com/KhronosGroup/OpenCL-ICD-Loader/archive/refs/tags/v2022.09.30.tar.gz)
set(OCLICD_DEP OpenCL-ICD-Loader/include/CL/opencl.h)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${OCLICD_DEP}")
    message("Downloading OpenCL-ICD-Loader-2022.09.30...")
    file(DOWNLOAD ${OCLICD_URL} "${CMAKE_CURRENT_SOURCE_DIR}/OpenCL-ICD-Loader-2022.09.30.tar.gz" SHOW_PROGRESS)
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/OpenCL-ICD-Loader-2022.09.30.tar.gz WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
endif()
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/OpenCL-ICD-Loader-2022.09.30)

#set(GFLAGS_URL https://github.com/gflags/gflags/archive/refs/tags/v2.2.2.tar.gz)
#set(GFLAGS_DEP gflags-2.2.2/src/config.h)
#if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${GFLAGS_DEP}")
#    message("Downloading GFLAGS...")
#    file(DOWNLOAD ${GFLAGS_URL} "${CMAKE_CURRENT_SOURCE_DIR}/gflags-2.2.2.tar.gz" SHOW_PROGRESS)
#    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/gflags-2.2.2.tar.gz WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
#endif()
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gflags-2.2.2)
