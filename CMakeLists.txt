cmake_minimum_required(VERSION 3.10)
project(rdma-bw-exercise-pdsl VERSION 0.1.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

# set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_COMPILER g++)

# set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2")
set(DEPS_PREFIX ${CMAKE_SOURCE_DIR}/deps)
set(CMAKE_PREFIX_PATH ${DEPS_PREFIX}/install)
include_directories(${DEPS_PREFIX}/install/include)

find_package(fmt REQUIRED)
find_package(jsoncpp REQUIRED)
find_package(libjson-rpc-cpp REQUIRED)

add_executable(server server.cc rdma.cc)
target_link_libraries(server fmt::fmt libjson-rpc-cpp::jsonrpcserver ibverbs)

add_executable(client client.cc rdma.cc)
target_link_libraries(client fmt::fmt libjson-rpc-cpp::jsonrpcclient ibverbs)