#cmake_minimum_required(VERSION 3.24)

#ExternalProject_Add(
#    systemc
#    PREFIX ${CMAKE_SOURCE_DIR}
#    TMP_DIR ${CMAKE_BINARY_DIR}
#    STAMP_DIR ${CMAKE_CURRENT_SOURCE_DIR}
#    DOWNLOAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}
#    URL      https://codeload.github.com/accellera-official/systemc/tar.gz/refs/tags/2.3.3.tar.gz
#    URL_MD5  589d313f3477f42d259607e5dbd316b4
#    TIMEOUT  10
#    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
#    CMAKE_COMMAND cmake
#    CMAKE_ARGS -DSC_ENABLE_ASSERTIONS
#               -DSC_DISABLE_COPYRIGHT_MESSAGE
#    BINARY_DIR ${CMAKE_BINARY_DIR}
#    INSTALL_COMMAND "" # Disable install step
#    LOG_DOWNLOAD ON
#    LOG_CONFIGURE ON
#    LOG_BUILD ON
#        )

# Fetch SystemC
set(SYSTEMC_URL https://github.com/accellera-official/systemc/archive/refs/tags/2.3.3.tar.gz)
set(SYSTEMC_DEP systemc-2.3.3/src/systemc.h)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SYSTEMC_DEP}")
    message("Downloading SystemC Lib...")
    file(DOWNLOAD ${SYSTEMC_URL} "${CMAKE_CURRENT_SOURCE_DIR}/SystemC-2.3.3.tar.gz" 
         SHOW_PROGRESS
         EXPECTED_HASH MD5=589d313f3477f42d259607e5dbd316b4)
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/SystemC-2.3.3.tar.gz WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    #set_target_properties(systemc PROPERTIES FOLDER "common/thirdparty")
    #execute_process(COMMAND tar -xzf ${CMAKE_CURRENT_SOURCE_DIR}/SystemC-2.3.3.tar.gz )
endif()

# Fetch SDL
set(SDL2_URL https://github.com/libsdl-org/SDL/releases/download/release-2.0.8/SDL2-2.0.8.tar.gz)
set(SDL2_DEP SDL2-2.0.8/include/SDL.h)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SDL2_DEP}")
    message("Downloading Simple DirectMedia Layer2 (SDL2)...")
    file(DOWNLOAD ${SDL2_URL} "${CMAKE_CURRENT_SOURCE_DIR}/SDL2-2.0.8.tar.gz" SHOW_PROGRESS)
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/SDL2-2.0.8.tar.gz WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
#    file(RENAME ${CMAKE_CURRENT_SOURCE_DIR}/SDL2-2.0.8 ${CMAKE_CURRENT_SOURCE_DIR}/SDL2)
endif()


# Fetch RapidJSON
set(RAPIDJSON_URL https://github.com/Tencent/rapidjson/archive/refs/tags/v1.1.0.tar.gz)
set(RAPIDJSON_DEP rapidjson-1.1.0/include/rapidjson/rapidjson.h)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${RAPIDJSON_DEP}")
    message("Downloading RapidJSON...")
    file(DOWNLOAD ${RAPIDJSON_URL} "${CMAKE_CURRENT_SOURCE_DIR}/rapidjson-1.1.0.tar.gz" SHOW_PROGRESS)
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson-1.1.0.tar.gz WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
#    file(RENAME ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson-1.1.0 ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson)
endif()

# Fetch ZLIB
set(ZLIB_URL https://github.com/madler/zlib/releases/download/v1.2.13/zlib-1.2.13.tar.gz)
set(ZLIB_DEP zlib-1.2.13/zlib.h)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${ZLIB_DEP}")
    message("Downloading Zlib...")
    file(DOWNLOAD ${ZLIB_URL} "${CMAKE_CURRENT_SOURCE_DIR}/zlib-1.2.13.tar.gz" SHOW_PROGRESS)
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/zlib-1.2.13.tar.gz WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
#    file(RENAME ${CMAKE_CURRENT_SOURCE_DIR}/zlib-1.2.13 ${CMAKE_CURRENT_SOURCE_DIR}/zlib)
endif()

# Fetch TinyXML2
set(TINYXML2_URL https://github.com/leethomason/tinyxml2/archive/refs/tags/8.0.0.tar.gz)
set(TINYXML2_DEP tinyxml2-8.0.0/tinyxml2.h)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TINYXML2_DEP}")
    message("Downloading TinyXML2...")
    file(DOWNLOAD ${TINYXML2_URL} "${CMAKE_CURRENT_SOURCE_DIR}/tinyxml2-8.0.0.tar.gz" SHOW_PROGRESS)
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/tinyxml2-8.0.0.tar.gz WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
#    file(RENAME ${CMAKE_CURRENT_SOURCE_DIR}/tinyxml2-8.0.0 ${CMAKE_CURRENT_SOURCE_DIR}/tinyxml2)
endif()

# Fetch Snappy (for apitrace support)
set(SNAPPY_URL https://github.com/google/snappy/archive/refs/tags/1.1.10.tar.gz)
set(SNAPPY_DEP snappy-1.1.10/snappy.h)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SNAPPY_DEP}")
    message("Downloading Snappy compression library...")
    file(DOWNLOAD ${SNAPPY_URL} "${CMAKE_CURRENT_SOURCE_DIR}/snappy-1.1.10.tar.gz" 
         SHOW_PROGRESS
         EXPECTED_HASH MD5=70153395ebe6d72febe2cf2e40026a44)
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/snappy-1.1.10.tar.gz WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# Build Snappy as a static library
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SNAPPY_DEP}")
    set(SNAPPY_BUILD_TESTS OFF CACHE BOOL "Build Snappy tests" FORCE)
    set(SNAPPY_BUILD_BENCHMARKS OFF CACHE BOOL "Build Snappy benchmarks" FORCE)
    add_subdirectory(snappy-1.1.10 EXCLUDE_FROM_ALL)
endif()
