cmake_minimum_required(VERSION 3.21)
project(common)

file(GLOB SRC_LIST "*.cc")

list(LENGTH SRC_LIST SRC_LIST_LEN)
if(SRC_LIST_LEN GREATER 0)
    add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})
    link_libraries(${PROJECT_NAME})
endif()
install(TARGETS ${PROJECT_NAME} EXPORT GalayTargets)

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)