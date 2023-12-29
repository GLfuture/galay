cmake_minimum_required(VERSION 3.21)
project(config)

set(SRC_LIST 
    config.cc
    )

add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})