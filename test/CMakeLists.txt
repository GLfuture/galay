project(test)
set(CMAKE_BUILD_TYPE Debug)
set(SRC_LIST 
    test_conf_parser.cc
    test_wait_group.cc
    )
foreach(SRC ${SRC_LIST})
    string(REPLACE ".cc" "" EXECUTABLE_NAME ${SRC})
    add_executable(${EXECUTABLE_NAME} ${SRC})
    target_link_directories(${EXECUTABLE_NAME} PRIVATE  ${CMAKE_SOURCE_DIR}/build/galay)
    target_link_libraries(${EXECUTABLE_NAME} galay-static)
endforeach()