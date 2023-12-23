cmake_minimum_required(VERSION 3.21)
project(client)

set(SRC_LIST 
    client.cc
    )

add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})