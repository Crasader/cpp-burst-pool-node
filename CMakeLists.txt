cmake_minimum_required(VERSION 3.1)

project(cpp-burst-pool-node)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
  nodecom.grpc.pb.h
  nodecom.pb.h
  )

set(SOURCE_FILES
  mshabal256_avx2.c
  mshabal256_sse4.c
  shabal64.s
  DeadlineRequestHandler.cpp
  NodeComClient.cpp
  nodecom.grpc.pb.cc
  nodecom.pb.cc
  )

set_source_files_properties(mshabal256_avx2.c PROPERTIES COMPILE_FLAGS -mavx2)

find_package(Protobuf REQUIRED)

find_package (Threads)
find_package(Boost REQUIRED COMPONENTS system thread)
include_directories(${Boost_INCLUDE_DIRS})

add_library(burstmath burstmath.c mshabal_sse4.c mshabal256_avx2.c shabal64.s)

add_library(wallet Wallet.cpp)
target_link_libraries(wallet ${Boost_LIBRARIES} PocoNet PocoUtil
  PocoFoundation PocoJSON pthread burstmath mysqlcppconn)

add_library(deadline_request_handler DeadlineRequestHandler.cpp)
target_link_libraries(deadline_request_handler ${Boost_LIBRARIES} PocoUtil ${CMAKE_THREAD_LIBS_INIT} burstmath)

add_library(node_com_client nodecom.grpc.pb.cc nodecom.pb.cc)
target_link_libraries(node_com_client grpc grpc++_unsecure protobuf)

add_executable(server Server.cpp Wallet.hpp DeadlineRequestHandler.hpp Config.hpp)
target_link_libraries(server PocoNet PocoUtil PocoFoundation ${Boost_LIBRARIES} wallet burstmath
  deadline_request_handler node_com_client)

# add_executable(node_com_client NodeComClient.cpp nodecom.grpc.pb.h nodecom.pb.h)
