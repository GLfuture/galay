cmake_minimum_required(VERSION 3.21)
project(server)

set(SRC_LIST 
    server.cc
    )

add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})