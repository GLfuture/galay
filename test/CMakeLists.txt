project(test_server)
include_directories(../src)
set(CMAKE_BUILD_TYPE Debug)
set(SRC_LIST 
    test_http_server.cc
    test_https_server.cc
    test_http_client.cc
    test_https_client.cc
    test_timer.cc
    test_threadpool.cc
    test_self_define_protocol_server.cc
    test_self_define_protocol_client.cc
    test_dns_client.cc
    )
foreach(SRC ${SRC_LIST})
    string(REPLACE ".cc" "" EXECUTABLE_NAME ${SRC})
    add_executable(${EXECUTABLE_NAME} ${SRC})
    target_link_directories(${EXECUTABLE_NAME} PRIVATE  ${CMAKE_SOURCE_DIR}/build/src)
    target_link_libraries(${EXECUTABLE_NAME} galay-static ssl crypto)
endforeach()

