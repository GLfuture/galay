project(service)

file(GLOB SRC_LIST "*.cc")

add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})
link_libraries(${PROJECT_NAME} 
        gRPC::grpc++ 
        gRPC::grpc 
        Protobuf::libprotobuf)
install(TARGETS service EXPORT GalayTargets)