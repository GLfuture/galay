cmake_minimum_required(VERSION 3.21)
project(galay)

set(CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INSTALL_PREFIX /usr/local)
set(HEADER_SRC_DIR galay)
set(HEADER_DST_DIR include)

include(cmake/Option.cmake)

if(ENABLE_DEBUG)
  set(CMAKE_BUILD_TYPE Debug)
else()
  set(CMAKE_BUILD_TYPE Release)
endif()

#查找依赖
include(cmake/Dependence.cmake)

#添加子目录
add_subdirectory(galay)
add_subdirectory(tutorial)

if(ENABLE_GTEST)
  message(STATUS "add test directory")
  add_subdirectory(test)
endif()

install(DIRECTORY ${HEADER_SRC_DIR}
        DESTINATION ${HEADER_DST_DIR}
        COMPONENT devel
        FILES_MATCHING
        PATTERN "*.h*"
        PATTERN "*.inl"
)

