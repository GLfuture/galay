cmake_minimum_required(VERSION 3.21)
project(kernel)

set(SRC_LIST 
    callback.cc
    iofunction.cc
    error.cc
    task.cc
    timer.cc
    threadpool.cc
    scheduler.cc
    )

set(DEPEND_LIBS ssl crypto)

add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} ${DEPEND_LIBS})