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
add_subdirectory(src)

add_subdirectory(test)