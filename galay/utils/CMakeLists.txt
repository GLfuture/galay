cmake_minimum_required(VERSION 3.21)
project(utils)

file(GLOB SRC_LIST "*.cc")

if(ENABLE_NLOHMANN_JSON)
    message(STATUS "USE_NLOHMANN_JSON ON")
    set (USE_NLOHMANN_JSON_DEFINE "USE_NLOHMANN_JSON")
endif(ENABLE_NLOHMANN_JSON)

add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${USE_NLOHMANN_JSON_DEFINE})
install(TARGETS utils EXPORT galayTargets)