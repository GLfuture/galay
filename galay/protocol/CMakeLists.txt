cmake_minimum_required(VERSION 3.21)
project(protocol)

set(SRC_LIST 
    http1_1.cc
    tcp.cc
    )

add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})