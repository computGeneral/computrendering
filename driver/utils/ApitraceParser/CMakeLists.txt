add_library(ApitraceParser
    ApitraceParser.h
    ApitraceParser.cpp
)

target_include_directories(ApitraceParser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/arch/common
    ${CMAKE_SOURCE_DIR}/common/thirdparty/snappy-1.1.10
)

target_link_libraries(ApitraceParser PUBLIC snappy)
