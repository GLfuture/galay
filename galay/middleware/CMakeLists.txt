cmake_minimum_required(VERSION 3.21)
project(middleware)

set(SRC_LIST "")
set(DEPEND_LIBS "")

if(ENABLE_MYSQL)
    message(STATUS "add etcd module")
    list(APPEND SRC_LIST mysql.cc)
    list(APPEND DEPEND_LIBS mysqlclient)
else()
    message(WARNING "skip mysql module")
endif()

if(ENABLE_ETCD)
    message(STATUS "add etcd module")
    list(APPEND SRC_LIST etcd.cc)
    list(APPEND DEPEND_LIBS etcd-cpp-api cpprest)
else()
    message(WARNING "skip etcd module")
endif()

add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} PRIVATE ${DEPEND_LIBS})