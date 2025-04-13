cmake_minimum_required(VERSION 3.21)
project(protocol)

file(GLOB SRC_LIST "*/*.cc")

add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})
install(TARGETS ${PROJECT_NAME} EXPORT GalayTargets)

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)