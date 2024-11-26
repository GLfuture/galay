cmake_minimum_required(VERSION 3.21)
project(kernel)

file(GLOB SRC_LIST "*.cc")

set(DEPEND_LIBS ssl crypto aio)

add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} ${DEPEND_LIBS})

install(TARGETS kernel EXPORT galayTargets)