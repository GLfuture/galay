cmake_minimum_required(VERSION 3.21)
project(galay)
set(CMAKE_BUILD_TYPE Debug)
set(CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_STANDARD 20)

find_package(OpenSSL REQUIRED)

if(OPENSSL_FOUND)
  message(STATUS "OpenSSL found")
else()
  message(FATAL_ERROR "OpenSSL not found")
endif()
set(CMAKE_INSTALL_PREFIX ..)
add_subdirectory(galay)

add_subdirectory(test)

set(HEADER_SRC_DIR galay)
set(HEADER_DST_DIR include)


install(DIRECTORY ${HEADER_SRC_DIR}
        DESTINATION ${HEADER_DST_DIR}
        FILES_MATCHING
        PATTERN "*.h"
)

