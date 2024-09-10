cmake_minimum_required(VERSION 3.10)
project(test)
set(CMAKE_BUILD_TYPE Debug)
file(GLOB SRC_LIST "*.cc")

foreach(SRC ${SRC_LIST})
    string(REGEX REPLACE ".*/([^/]*)" "\\1" FILE ${SRC})
    list(APPEND SRC_FILES ${FILE})
endforeach()


foreach(SRC ${SRC_FILES})
    string(REPLACE ".cc" "" EXECUTABLE_NAME ${SRC})
    add_executable(${EXECUTABLE_NAME} ${SRC})
    target_link_directories(${EXECUTABLE_NAME} PRIVATE  ${CMAKE_SOURCE_DIR}/build/galay)
    target_link_libraries(${EXECUTABLE_NAME} galay-static gtest_main gtest)
endforeach()