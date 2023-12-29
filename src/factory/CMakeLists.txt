cmake_minimum_required(VERSION 3.21)
project(factory)

set(SRC_LIST 
    factory.cc
    )

add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})