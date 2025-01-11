cmake_minimum_required(VERSION 3.21)
project(kernel)

file(GLOB SRC_LIST "*.cc")

find_library(SSL_LIB NAMES ssl)
find_library(CRYPTO_LIB NAMES crypto)

set(DEPEND_LIBS ${SSL_LIB} ${CRYPTO_LIB})

if (LIBAIO_FOUND)
    find_library(LIBAIO_LIB NAMES aio)
    set(DEPEND_LIBS ${DEPEND_LIBS} ${LIBAIO_LIB})
endif ()

add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} PRIVATE ${DEPEND_LIBS})
if(ENABLE_TRACE_LOG)
    message(STATUS "ENABLE_DISPLAY_GALAY_LOG ON")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "ENABLE_DISPLAY_GALAY_LOG")
endif()

install(TARGETS kernel EXPORT GalayTargets)