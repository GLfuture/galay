cmake_minimum_required(VERSION 3.21)
project(kernel)

file(GLOB SRC_LIST "*.cc")

set(DEPEND_LIBS ssl crypto)

if (LIBAIO_FOUND)
    set(DEPEND_LIBS ${DEPEND_LIBS} aio)
endif ()

add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} ${DEPEND_LIBS})
if(ENABLE_TRACE_LOG)
    message(STATUS "ENABLE_DISPLAY_GALAY_LOG ON")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "ENABLE_DISPLAY_GALAY_LOG")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "ENABLE_DISPLAY_GALAY_LOG")
endif()

install(TARGETS kernel EXPORT galayTargets)