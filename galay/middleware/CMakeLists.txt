cmake_minimum_required(VERSION 3.21)
project(middleware)

set(SRC_LIST "")
set(DEPEND_LIBS "")

if(ENABLE_MYSQL)
    message(STATUS "Add mysql module!")
    find_library(MYSQL_LIB NAMES mysqlclient)
    list(APPEND SRC_LIST Mysql.cc)
    list(APPEND DEPEND_LIBS ${MYSQL_LIB})
else()
    message(WARNING "Skip mysql module!")
endif()

if(ENABLE_ETCD)
    message(STATUS "Add etcd module!")
    find_library(CPPREST_LIB NAMES cpprest)
    find_library(ETCD_LIB NAMES etcd-cpp-api)
    list(APPEND SRC_LIST Etcd.cc)
    list(APPEND DEPEND_LIBS ${ETCD_LIB} ${CPPREST_LIB})
else()
    message(WARNING "Skip etcd module!")
endif()

if(ENABLE_REDIS)
    message(STATUS "Add redis module!")
    find_library(HIREDIS_LIB NAMES hiredis)
    list(APPEND SRC_LIST Redis.cc)
    list(APPEND DEPEND_LIBS ${HIREDIS_LIB})
else()
    message(WARNING "Skip redis module!")
endif()


add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} PRIVATE ${DEPEND_LIBS})

install(TARGETS middleware EXPORT galayTargets)