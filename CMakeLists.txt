cmake_minimum_required(VERSION 3.21)
project(galay)
set(CMAKE_BUILD_TYPE Debug)
set(CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_STANDARD 20)


set(CMAKE_INSTALL_PREFIX /user/local)

set(HEADER_SRC_DIR galay)
set(HEADER_DST_DIR include)

#查找依赖

include(cmake/FindSpdlog.cmake)

find_package(OpenSSL REQUIRED)
find_package(spdlog REQUIRED)

if(OPENSSL_FOUND)
  message(STATUS "openssl found")
else()
  message(FATAL_ERROR "openssl not found")
endif()

if(SPDLOG_FOUND)
  message(STATUS "spdlog found")
else()
  message(FATAL_ERROR "spdlog not found")
endif()

#find mysql
find_path(MYSQL_INCLUDE_DIR NAMES mysql)
find_library(MYSQL_LIBRARY NAMES mysqlclient)

if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARY)
    set(MYSQL_FOUND TRUE)
endif()

if(MYSQL_FOUND)
  message(STATUS "mysql found")
else()
  message(WARNING "mysql not found")
endif()

#find etcd
find_path(ETCD_INCLUDE_DIR NAMES etcd)
find_library(ETCD_LIBRARY NAMES etcd-cpp-api)
find_library(CPPREST_LIBRARY NAMES cpprest)

if(ETCD_INCLUDE_DIR AND ETCD_LIBRARY AND CPPREST_LIBRARY)
  set(ETCD_FOUND TRUE)
endif()

if(ETCD_FOUND)
  message(STATUS "etcd found")
else()
  message(WARNING "etcd not found")
endif()

#find kafka


#添加子目录
add_subdirectory(galay)
add_subdirectory(tutorial)
add_subdirectory(test)

install(DIRECTORY ${HEADER_SRC_DIR}
        DESTINATION ${HEADER_DST_DIR}
        COMPONENT devel
        FILES_MATCHING
        PATTERN "*.h*"
        PATTERN "*.inl"
)

