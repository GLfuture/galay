cmake_minimum_required(VERSION 3.21)
project(protocol)

file(GLOB SRC_LIST "*.cc")

add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})
install(TARGETS protocol EXPORT GalayTargets)