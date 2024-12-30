project(tutorial)
set(CMAKE_BUILD_TYPE Debug)
set(SRC_LIST 
    tutorial_client.cc
    tutorial_echo_server.cc
    tutorial_ssl_echo_server.cc
    test_time.cc
    test_fileio.cc
    test_waitgroup.cc
    test_udp.cc
    demo.cc
    )


set(COMMON_LIB ssl crypto)
if(LINUX)
    list(APPEND COMMON_LIB aio)
endif()

set(INSTALL_BIN)
include_directories(${CMAKE_SOURCE_DIR})    

foreach(SRC ${SRC_LIST})
    string(REPLACE ".cc" "" EXECUTABLE_NAME ${SRC})
    add_executable(${EXECUTABLE_NAME} ${SRC})
    target_link_libraries(${EXECUTABLE_NAME} galay ${COMMON_LIB})
    set_target_properties(${EXECUTABLE_NAME} PROPERTIES
        INSTALL_RPATH $ORIGIN/../lib
    )
    list(APPEND INSTALL_BIN ${EXECUTABLE_NAME})
endforeach()

if(ETCD_FOUND)
    add_executable(tutorial_etcd_discover_client tutorial_etcd_discover_client.cc)
    target_link_libraries(tutorial_etcd_discover_client PRIVATE galay cpprest etcd-cpp-api ${COMMON_LIB})
    set_target_properties(tutorial_etcd_discover_client PROPERTIES
        INSTALL_RPATH $ORIGIN/../lib
    )
    add_executable(tutorial_etcd_register_client tutorial_etcd_register_client.cc)
    target_link_libraries(tutorial_etcd_register_client PRIVATE galay cpprest etcd-cpp-api ${COMMON_LIB})
    set_target_properties(tutorial_etcd_register_client PROPERTIES
        INSTALL_RPATH $ORIGIN/../lib
    )
    list(APPEND INSTALL_BIN tutorial_etcd_discover_client tutorial_etcd_register_client)
endif()

if(MYSQL_FOUND)
    add_executable(tutorial_mysql_client tutorial_mysql_client.cc)
    target_link_libraries(tutorial_mysql_client PRIVATE galay mysqlclient ${COMMON_LIB})
    set_target_properties(tutorial_mysql_client PROPERTIES
        INSTALL_RPATH $ORIGIN/../lib
    )
    list(APPEND INSTALL_BIN tutorial_mysql_client)
endif()

if(REDIS_FOUND)
    add_executable(tutorial_redis_client tutorial_redis_client.cc)
    find_package(Hiredis REQUIRED)
    target_link_libraries(tutorial_redis_client PRIVATE galay ${Hiredis} ${COMMON_LIB})
    set_target_properties(tutorial_redis_client PROPERTIES
        INSTALL_RPATH $ORIGIN/../lib
    )
    list(APPEND INSTALL_BIN tutorial_redis_client)
endif()

if(NOT ENABLE_INSTALL_SYSTEM)
    install(TARGETS ${INSTALL_BIN} DESTINATION bin)
endif()