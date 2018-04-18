set_source_files_properties(mshabal256_avx2.c PROPERTIES COMPILE_FLAGS -mavx2)

set(SOURCE_FILES
  Server.cpp
  Wallet.cpp
  DeadlineRequestHandler.cpp
  Session.cpp
  NodeComClient.hpp
  nodecom.grpc.pb.cc
  nodecom.pb.cc
  burstmath.c
  shabal64.s
  mshabal_sse4.c
  )

set(HEADER_FILES
  Wallet.hpp
  DeadlineRequestHandler.hpp
  Session.hpp
  NodeComClient.hpp
  concurrentqueue.h
  nodecom.grpc.pb.h
  nodecom.pb.h
  Config.hpp
  RateLimiter.hpp
  burstmath.h
  shabal.h
  mshabal.h
  )

if (USE_AVX2)
  add_definitions(-DUSE_AVX2)
  set(SOURCE_FILES ${SOURCE_FILES} mshabal256_avx2.c)
endif()

add_executable(server ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(server
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES} 
  glog
  grpc
  grpc++_unsecure
  protobuf
  mysqlcppconn
  curl
  )


