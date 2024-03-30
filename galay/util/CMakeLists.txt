cmake_minimum_required(VERSION 3.21)
project(util)

set(SRC_LIST 
    stringutil.cc parser.cc file.cc random.cc)

add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})
link_libraries(${PROJECT_NAME})