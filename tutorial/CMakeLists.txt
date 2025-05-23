project(tutorial)
set(SRC_LIST 
    tutorial_client.cc
    tutorial_echo_server.cc
    tutorial_static_server.cc
    tutorial_ssl_echo_server.cc
    tutorial_cmd_parser.cc
    test_time.cc
    test_fileio.cc
    test_waitgroup.cc
    test_udp.cc
    tutorial_cricuitbreaker.cc
    )


set(COMMON_LIB ssl crypto)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND COMMON_LIB aio)
endif()

if(ENABLE_GRPC)
    include(../cmake/GrpcCommon.cmake)
    list(APPEND COMMON_LIB gRPC::grpc++)
endif()


set(INSTALL_BIN)
include_directories(${CMAKE_SOURCE_DIR})    

foreach(SRC ${SRC_LIST})
    string(REPLACE ".cc" "" EXECUTABLE_NAME ${SRC})
    add_executable(${EXECUTABLE_NAME} ${SRC})
    target_link_libraries(${EXECUTABLE_NAME} galay ${COMMON_LIB})
    set_target_properties(${EXECUTABLE_NAME} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        INSTALL_RPATH $ORIGIN/../lib
    )
    list(APPEND INSTALL_BIN ${EXECUTABLE_NAME})
endforeach()

if(ETCD_FOUND)
    add_executable(tutorial_etcd_discover_client tutorial_etcd_discover_client.cc)
    target_link_libraries(tutorial_etcd_discover_client PRIVATE galay cpprest etcd-cpp-api ${COMMON_LIB})
    set_target_properties(tutorial_etcd_discover_client PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        INSTALL_RPATH $ORIGIN/../lib
    )
    add_executable(tutorial_etcd_register_client tutorial_etcd_register_client.cc)
    target_link_libraries(tutorial_etcd_register_client PRIVATE galay cpprest etcd-cpp-api ${COMMON_LIB})
    set_target_properties(tutorial_etcd_register_client PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        INSTALL_RPATH $ORIGIN/../lib
    )
    list(APPEND INSTALL_BIN tutorial_etcd_discover_client tutorial_etcd_register_client)
endif()

if(MYSQL_FOUND)
    add_executable(tutorial_mysql_client tutorial_mysql_client.cc)
    target_link_libraries(tutorial_mysql_client PRIVATE galay mysqlclient ${COMMON_LIB})
    set_target_properties(tutorial_mysql_client PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        INSTALL_RPATH $ORIGIN/../lib
    )
    list(APPEND INSTALL_BIN tutorial_mysql_client)
endif()

if(REDIS_FOUND)
    add_executable(tutorial_redis_client tutorial_redis_client.cc)
    find_package(hiredis REQUIRED)
    target_link_libraries(tutorial_redis_client PRIVATE galay ${hiredis} ${COMMON_LIB})
    set_target_properties(tutorial_redis_client PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        INSTALL_RPATH $ORIGIN/../lib
    )
    list(APPEND INSTALL_BIN tutorial_redis_client)
endif()

if(NOT ENABLE_INSTALL_SYSTEM)
    install(TARGETS ${INSTALL_BIN} DESTINATION bin)
endif()