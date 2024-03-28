cmake_minimum_required(VERSION 3.21)
project(middleware)

set(SRC_LIST "")
set(DEPEND_LIBS "")

if(MYSQL_FOUND)
    list(APPEND SRC_LIST SyncMySql.cc)
    list(APPEND DEPEND_LIBS mysqlclient)
endif()

add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} PRIVATE ${DEPEND_LIBS})