cmake_minimum_required(VERSION 3.21)
project(galay)
set(CMAKE_BUILD_TYPE Debug)
set(CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_STANDARD 20)


set(CMAKE_INSTALL_PREFIX ..)

set(HEADER_SRC_DIR galay)
set(HEADER_DST_DIR include)

#查找依赖

include(cmake/FindSpdlog.cmake)

find_package(OpenSSL REQUIRED)
find_package(spdlog REQUIRED)

if(OPENSSL_FOUND)
  message(STATUS "OpenSSL found")
else()
  message(FATAL_ERROR "OpenSSL not found")
endif()

if(SPDLOG_FOUND)
  message(STATUS "Spdlog found")
else()
  message(FATAL_ERROR "Spdlog not found")
endif()

#find mysql
find_path(MYSQL_INCLUDE_DIR NAMES mysql/mysql.h)
find_library(MYSQL_LIBRARY NAMES mysqlclient)

if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARY)
    set(MYSQL_FOUND TRUE)
endif()

if(MYSQL_FOUND)
  message(STATUS "MySQL found")
else()
  message(FATAL_ERROR "MySQL not found")
endif()

#添加子目录
add_subdirectory(galay)
add_subdirectory(tutorial)

install(DIRECTORY ${HEADER_SRC_DIR}
        DESTINATION ${HEADER_DST_DIR}
        FILES_MATCHING
        PATTERN "*.h"
)

