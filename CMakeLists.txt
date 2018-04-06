cmake_minimum_required(VERSION 3.1)

project(cpp-burst-pool-node)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE Debug)

enable_language(ASM)

set(HEADER_FILES
  shabal.h
  mshabal256.h
  mshabal.h
  DeadlineRequestHandler.hpp
  concurrentqueue.h
  Wallet.hpp
  Config.hpp
  NodeComClient.hpp
  Server.hpp
  Session.hpp
  nodecom.grpc.pb.h
  nodecom.pb.h
  )

set(SOURCE_FILES
  mshabal256_avx2.c
  mshabal256_sse4.c
  shabal64.s
  DeadlineRequestHandler.cpp
  Session.cpp
  NodeComClient.cpp
  nodecom.grpc.pb.cc
  nodecom.pb.cc
  Server.cpp
  )

set_source_files_properties(mshabal256_avx2.c PROPERTIES COMPILE_FLAGS -mavx2)

find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)
find_package (Threads)
# find_package(glog REQUIRED CONFIG NAMES google-glog glog)
find_package(Boost REQUIRED COMPONENTS system thread)
include_directories(${Boost_INCLUDE_DIRS})

add_library(burstmath burstmath.c mshabal_sse4.c mshabal256_avx2.c shabal64.s)

add_library(wallet Wallet.cpp)
target_link_libraries(wallet ${Boost_LIBRARIES} pthread burstmath mysqlcppconn curlpp curl)

# add_library(deadline_request_handler DeadlineRequestHandler.cpp Session.hpp)
# target_link_libraries(deadline_request_handler ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} burstmath grpc)

add_library(node_com_client nodecom.grpc.pb.cc nodecom.pb.cc)
target_link_libraries(node_com_client grpc grpc++_unsecure protobuf)

add_executable(server Server.cpp Config.hpp Wallet.cpp DeadlineRequestHandler.cpp Session.cpp Session.hpp RateLimiter.hpp)
target_link_libraries(server
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES} 
  glog
  wallet
  burstmath
  grpc
  grpc++_unsecure
  protobuf
  node_com_client
  )


