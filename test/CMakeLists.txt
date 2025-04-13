cmake_minimum_required(VERSION 3.21)
project(MyTest)
set(CMAKE_BUILD_TYPE Debug)

# 检测 nlohmann/json.hpp
find_path(NLOHMANN_JSON_INCLUDE_DIR
    NAMES nlohmann/json.hpp
    HINTS ${CMAKE_SOURCE_DIR}
    PATH_SUFFIXES include
)
if(NLOHMANN_JSON_INCLUDE_DIR)
    message(STATUS "Found nlohmann/json.hpp at ${NLOHMANN_JSON_INCLUDE_DIR}")
    set(HAVE_NLOHMANN_JSON_HPP 1)
else()
    message(WARNING "nlohmann/json.hpp not found, test_conf_parser may be skipped")
endif()

file(GLOB SRC_LIST "*.cc")

foreach(SRC ${SRC_LIST})
    string(REGEX REPLACE ".*/([^/]*)" "\\1" FILE ${SRC})
    list(APPEND SRC_FILES ${FILE})
endforeach()

find_library(GTEST_LIBRARY NAMES gtest)
find_library(GTEST_MAIN_LIBRARY NAMES gtest_main)

foreach(SRC ${SRC_FILES})
    string(REPLACE ".cc" "" EXECUTABLE_NAME ${SRC})
    add_executable(${EXECUTABLE_NAME} ${SRC})
    target_include_directories(${EXECUTABLE_NAME} PRIVATE ${CMAKE_SOURCE_DIR})
    
    # 添加 json.hpp 的包含路径（如果存在）
    if(NLOHMANN_JSON_INCLUDE_DIR)
        target_include_directories(${EXECUTABLE_NAME}
            PRIVATE ${NLOHMANN_JSON_INCLUDE_DIR}
        )
    endif()
    
    target_link_libraries(${EXECUTABLE_NAME}
        galay-static
        ${GTEST_LIBRARY}
        ${GTEST_MAIN_LIBRARY}
    )

    set_target_properties(${EXECUTABLE_NAME} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    
    # 仅在 test_conf_parser.cc 存在且检测到 json.hpp 时添加宏
    if(SRC STREQUAL "test_conf_parser.cc" AND HAVE_NLOHMANN_JSON_HPP)
        target_compile_definitions(${EXECUTABLE_NAME}
            PRIVATE INCLUDE_NLOHMANN_JSON_HPP
        )
    endif()
endforeach()