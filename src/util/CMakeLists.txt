cmake_minimum_required(VERSION 3.21)
project(util)

set(SRC_LIST 
    stringutil.cc 
    base64.cc 
    )

add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})