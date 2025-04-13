project(rpc_test)

# 生成 proto 文件（需要 protobuf）
set(PROTO_SRC hello.grpc.pb.cc hello.pb.cc)
set(PROTO_HDR hello.grpc.pb.h hello.pb.h)

set(CMAKE_EXE_LINKER_FLAGS -Wl,--no-as-needed)

find_package(OpenSSL REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

include(../cmake/GrpcCommon.cmake)

set(hw_proto_srcs "hello.pb.cc")
set(hw_proto_hdrs "hello.pb.h")
set(hw_grpc_srcs "hello.grpc.pb.cc")
set(hw_grpc_hdrs "hello.grpc.pb.h")

add_library(hw_grpc_proto
  ${hw_grpc_srcs}
  ${hw_grpc_hdrs}
  ${hw_proto_srcs}
  ${hw_proto_hdrs})

set_target_properties(hw_grpc_proto PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)

target_link_libraries(hw_grpc_proto
  gRPC::grpc++
  OpenSSL::SSL  # 使用 CMake 提供的 OpenSSL 目标
  OpenSSL::Crypto
  Threads::Threads
)


# Targets greeter_[async_](client|server)
foreach(_target
  test_grpc_client test_grpc_server)
  add_executable(${_target} "${_target}.cc")
  set_target_properties(${_target} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
  )

  # 添加协程编译标志
 
  target_link_libraries(${_target}
    hw_grpc_proto  # 明确链接 hw_grpc_proto 库
    gRPC::grpc++ 
    OpenSSL::SSL  # 使用 CMake 提供的 OpenSSL 目标
    OpenSSL::Crypto
    Threads::Threads 
    galay
    aio)
endforeach()