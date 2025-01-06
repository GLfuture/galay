cmake_minimum_required(VERSION 3.21)
project(Galay VERSION 1.0)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CXX_STANDARD_REQUIRED on)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 20)

set(HEADER_SRC_DIR galay)
set(HEADER_DST_DIR include)

include(cmake/Option.cmake)

if(ENABLE_INSTALL_SYSTEM)
    set(CMAKE_INSTALL_PREFIX /usr/local)
else()
    set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install)
endif()
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")

if(ENABLE_DEBUG)
    set(CMAKE_BUILD_TYPE Debug)
    message(STATUS "Debug mode enabled")
else()
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "Release mode enabled")
endif()

#查找依赖
include(cmake/Dependence.cmake)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(MIDDLEWARE)
if(ENABLE_MIDDLEWARE)
    list(APPEND MIDDLEWARE middleware)
endif()

include_directories(thirdparty)

set(THIRDPARTY_INCLUDE_DIRS concurrentqueue)

set(DEPEND_LIBS kernel utils protocol ${MIDDLEWARE} security common helper)
set(STATIC_LIB ${PROJECT_NAME}-static)
set(SHARED_LIB ${PROJECT_NAME})

include_directories(${PROJECT_SOURCE_DIR})

#添加子目录
add_subdirectory(galay)

# 初始化对象库列表
set(OBJECT_LIBS)

foreach(depend_lib ${DEPEND_LIBS})
    message(STATUS "Processing dependency: ${depend_lib}")
    if(TARGET ${depend_lib})
        list(APPEND OBJECT_LIBS $<TARGET_OBJECTS:${depend_lib}>)
    else()
        message(FATAL_ERROR "Target ${depend_lib}_obj does not exist")
    endif()
endforeach()


# 创建静态库
add_library(${STATIC_LIB} STATIC ${OBJECT_LIBS})
target_link_libraries(${STATIC_LIB} PRIVATE ${DEPEND_LIBS})

# 创建共享库
add_library(${SHARED_LIB} SHARED ${OBJECT_LIBS})
target_link_libraries(${SHARED_LIB} PRIVATE ${DEPEND_LIBS})

target_compile_features(${STATIC_LIB} PRIVATE cxx_std_20)
target_compile_features(${SHARED_LIB} PRIVATE cxx_std_20)


install(TARGETS ${STATIC_LIB} DESTINATION lib EXPORT ${PROJECT_NAME}Targets)
install(TARGETS ${SHARED_LIB} DESTINATION lib EXPORT ${PROJECT_NAME}Targets)

# 生成版本配置文件
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(${PROJECT_NAME}Config.cmake.in
                ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        INSTALL_DESTINATION 
                ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}/
)

install(EXPORT 
                ${PROJECT_NAME}Targets
        FILE
                ${PROJECT_NAME}Targets.cmake
        NAMESPACE 
                "${PROJECT_NAME}::"
        DESTINATION
                ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
        COMPONENT
                Devel
)

install(
        FILES
                ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
        DESTINATION
                ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
        COMPONENT
                Devel
)


add_subdirectory(tutorial)

#test
if(ENABLE_GTEST)
    message(STATUS "Add test directory")
    add_subdirectory(test)
endif()

install(DIRECTORY ${HEADER_SRC_DIR}
    DESTINATION ${HEADER_DST_DIR}
    COMPONENT devel
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.inl"
    PATTERN "*.tcc"
)

foreach(THIRDPARTY_INCLUDE_DIR ${THIRDPARTY_INCLUDE_DIRS})
    install(DIRECTORY thirdparty/${THIRDPARTY_INCLUDE_DIR}
            DESTINATION ${HEADER_DST_DIR}
            COMPONENT devel
            FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
            PATTERN "*.inl"
            PATTERN "*.tcc"
    )
endforeach()
