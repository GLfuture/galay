project(test_server)
include_directories(../src)
set(CMAKE_BUILD_TYPE Debug)
set(SRC_LIST 
    test_tcp_server.cc
    test_http_server.cc)
foreach(SRC ${SRC_LIST})
    string(REPLACE ".cc" "" EXECUTABLE_NAME ${SRC})
    add_executable(${EXECUTABLE_NAME} ${SRC})
    target_link_directories(${EXECUTABLE_NAME} PRIVATE  ${CMAKE_SOURCE_DIR}/build/src)
    target_link_libraries(${EXECUTABLE_NAME} galay-static ssl crypto)
endforeach()

