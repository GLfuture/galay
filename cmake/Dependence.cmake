#openssl
find_package(OpenSSL REQUIRED)

if(OPENSSL_FOUND)
    message(STATUS "openssl found")
else()
    message(FATAL_ERROR "openssl not found")
endif()

#spdlog 
find_path(SPDLOG_INCLUDE_DIR NAMES spdlog/spdlog.h)
find_library(SPDLOG_LIBRARY NAMES spdlog)

if(SPDLOG_INCLUDE_DIR AND SPDLOG_LIBRARY)
    message(STATUS "spdlog found")
    set(SPDLOG_FOUND TRUE)
else()
    message(FATAL_ERROR "spdlog not found")
endif()