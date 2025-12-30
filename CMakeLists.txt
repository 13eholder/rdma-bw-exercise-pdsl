cmake_minimum_required(VERSION 3.22)
project(rdma-bw-exercise-pdsl VERSION 0.2.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

add_subdirectory("third_party/spdlog" EXCLUDE_FROM_ALL)
add_subdirectory("third_party/expected" EXCLUDE_FROM_ALL)
add_subdirectory("third_party/json" EXCLUDE_FROM_ALL)
add_subdirectory("third_party/json-rpc-cxx" EXCLUDE_FROM_ALL)

set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF)
set(HTTPLIB_USE_ZSTD_IF_AVAILABLE OFF)
add_subdirectory("third_party/cpp-httplib" EXCLUDE_FROM_ALL)

add_library(rdma "include/rdma.h" "include/rdma.cc")
target_link_libraries(rdma PUBLIC spdlog expected nlohmann_json::nlohmann_json httplib::httplib json-rpc-cxx)

add_executable(main "examples/main.cc")
target_link_libraries(main PRIVATE rdma)

# add_subdirectory("third_party/jsoncpp")
# add_subdirectory("third_party/libjson-rpc-cpp")

# add_executable(server server.cc rdma.cc)
# target_link_libraries(server fmt::fmt libjson-rpc-cpp::jsonrpcserver ibverbs)

# add_executable(client client.cc rdma.cc)
# target_link_libraries(client fmt::fmt libjson-rpc-cpp::jsonrpcclient ibverbs)