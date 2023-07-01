# https://cmake.org/cmake/help/latest/module/FetchContent.html#examples
# https://www.w3cschool.cn/doc_cmake_3_6/cmake_3_6-module-externalproject.html
cmake_minimum_required(VERSION 3.13)
project(CallMonitor)

# Fix the warnings about `DOWNLOAD_EXTRACT_TIMESTAMP` in newer CMake versions.
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

# 支持多进程
#set(CMAKE_CXX_FLAGS -pthread)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Debug)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
get_filename_component(thirdparty "${CMAKE_SOURCE_DIR}/thirdparty" REALPATH BASE_DIR "${CMAKE_SOURCE_DIR}")
set(FETCHCONTENT_BASE_DIR ${thirdparty})
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -g -pthread -fPIC")
else()
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    # link_libraries(ws2_32)
endif()

include(gflags)
include(glog)
include(libtorch)

# 以下库, 只需要 include, 不需要 link.
include(nlohmann_json)
include(httplib)

include_directories(
    "${CMAKE_SOURCE_DIR}"
)
add_subdirectory(toolbox/hashlib)


file(GLOB_RECURSE SERVER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/server/**/*.cpp")
add_executable(CallMonitor main.cpp ${SERVER_SRC})

target_link_libraries(CallMonitor gflags)
target_link_libraries(CallMonitor glog::glog)
target_link_libraries(CallMonitor "${TORCH_LIBRARIES}")
target_link_libraries(CallMonitor hashlib)
