cmake_minimum_required(VERSION 3.21)
project(util)

set(SRC_LIST 
    stringutil.cc 
    base64.cc
    md5.cc
    sha256.cc
    conf_parser.cc
    file.cc
    )

add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})
link_libraries(${PROJECT_NAME} ssl crypto)