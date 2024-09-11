cmake_minimum_required(VERSION 3.10)
project(rdma-bw-exercise-pdsl VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2")

include_directories(${CMAKE_SOURCE_DIR}/deps/install/include)

find_package(fmt REQUIRED)
find_package(jsoncpp REQUIRED)
find_package(libjson-rpc-cpp REQUIRED)

add_executable(server server.cc rdma.cc)
target_link_libraries(server fmt::fmt libjson-rpc-cpp::jsonrpcserver ibverbs)

add_executable(client client.cc rdma.cc)
target_link_libraries(client fmt::fmt libjson-rpc-cpp::jsonrpcclient ibverbs)