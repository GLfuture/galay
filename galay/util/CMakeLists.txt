cmake_minimum_required(VERSION 3.21)
project(util)

file(GLOB SRC_LIST "*.cc")

add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})
link_libraries(${PROJECT_NAME})