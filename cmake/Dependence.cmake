#openssl
find_package(OpenSSL REQUIRED)

if(OPENSSL_FOUND)
    message(STATUS "openssl found")
    include_directories(${OPENSSL_INCLUDE_DIR})
else()
    message(FATAL_ERROR "openssl not found")
endif()

#spdlog 
find_path(  SPDLOG_INCLUDE_DIR 
            NAMES spdlog/spdlog.h 
            PATHS
                /usr/local/include
                /usr/include
                ${CMAKE_PREFIX_PATH}/include
                $ENV{SPDLOG_ROOT}/include
                ${SPDLOG_ROOT}/include
            DOC "The directory where spdlog headers reside"
        )

find_library(   SPDLOG_LIBRARY 
                NAMES spdlog
                PATHS
                    /usr/local/lib
                    /usr/lib
                    ${CMAKE_PREFIX_PATH}/lib
                    $ENV{SPDLOG_ROOT}/lib
                    ${SPDLOG_ROOT}/lib
                DOC "The spdlog library"
            )

if(SPDLOG_INCLUDE_DIR AND SPDLOG_LIBRARY)
    message(STATUS "spdlog found")
    include_directories(${SPDLOG_INCLUDE_DIR})
    set(SPDLOG_FOUND TRUE)
else()
    message(FATAL_ERROR "spdlog not found")
endif()