cmake_minimum_required(VERSION 3.1)

project(cpp-burst-pool-node)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE Debug)

enable_language(ASM)

option(USE_AVX2 "If yes, AVX2 will be enabled" ON)

find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)
find_package (Threads)
find_package(Boost REQUIRED COMPONENTS system thread)
include_directories(
  ${Boost_INCLUDE_DIRS}
  vendor/beast/include
  )

subdirs(src)

