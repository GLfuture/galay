set(CMAKE_BUILD_TYPE Debug)
add_executable(test test.cc)
target_link_directories(test PRIVATE  ${CMAKE_SOURCE_DIR}/build/src)
target_link_libraries(test galay-static.a ssl crypto)