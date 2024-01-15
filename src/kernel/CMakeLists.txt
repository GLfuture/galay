cmake_minimum_required(VERSION 3.21)
project(kernel)

set(SRC_LIST 
    engine.cc
    iofunction.cc
    error.cc
    task.cc
    timer.cc
    scheduler.cc
    )

add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} ssl crypto)