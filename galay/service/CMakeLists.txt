project(service)

find_package(gRPC CONFIG REQUIRED)

include(../../cmake/GrpcCommon.cmake)

file(GLOB SRC_LIST "*.cc")

set(CMAKE_EXE_LINKER_FLAGS -Wl,--no-as-needed)

add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})

target_sources(${PROJECT_NAME}
    PRIVATE 
        $<TARGET_OBJECTS:kernel>  # 假设kernel的目标名为kernel
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)


link_libraries(${PROJECT_NAME} 
        PUBLIC
        gRPC::grpc++)

install(TARGETS service EXPORT GalayTargets)