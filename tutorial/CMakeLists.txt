project(test_server)
set(CMAKE_BUILD_TYPE Debug)
set(SRC_LIST 
    test_http_server.cc
    test_https_server.cc
    test_http_client.cc
    test_https_client.cc
    test_timer.cc
    test_threadpool.cc
    test_tcp_self_protocol_server.cc
    test_tcp_self_protocol_client.cc
    test_dns_client.cc
    test_udp_self_protocol_server.cc
    test_udp_self_protocol_client.cc
    test_smtp_client.cc
    test_conf_parse.cc
    )
foreach(SRC ${SRC_LIST})
    string(REPLACE ".cc" "" EXECUTABLE_NAME ${SRC})
    add_executable(${EXECUTABLE_NAME} ${SRC})
    target_link_directories(${EXECUTABLE_NAME} PRIVATE  ${CMAKE_SOURCE_DIR}/build/galay)
    target_link_libraries(${EXECUTABLE_NAME} galay-static)
endforeach()

if(MYSQL_FOUND)
    add_executable(test_mysql_client test_msyql_client.cc)
    target_link_directories(test_mysql_client PRIVATE  ${CMAKE_SOURCE_DIR}/build/galay)
    target_link_libraries(test_mysql_client galay-static)
endif()