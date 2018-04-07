cmake_minimum_required(VERSION 3.1)

project(cpp-burst-pool-node)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE Debug)

enable_language(ASM)

set_source_files_properties(mshabal256_avx2.c PROPERTIES COMPILE_FLAGS -mavx2)

find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)
find_package (Threads)
find_package(Boost REQUIRED COMPONENTS system thread)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(server
  Server.cpp
  Config.hpp
  Wallet.cpp
  DeadlineRequestHandler.cpp
  Session.cpp
  RateLimiter.hpp
  nodecom.grpc.pb.cc
  nodecom.pb.cc
  burstmath.c
  mshabal_sse4.c
  mshabal256_avx2.c
  shabal64.s
  )

target_link_libraries(server
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES} 
  glog
  grpc
  grpc++_unsecure
  protobuf
  mysqlcppconn
  curlpp
  curl
  )


