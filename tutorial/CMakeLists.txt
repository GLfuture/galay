project(tutorial)
set(CMAKE_BUILD_TYPE Debug)
set(SRC_LIST 
    tutorial_http_client.cc
    tutorial_echo_server.cc
    tutorial_proxy_server.cc
    tutorial_ratelimiter.cc
    tutorial_smtp_proxy_server.cc
    tutorial_dns_client.cc
    tutorial_https_server.cc
    )
foreach(SRC ${SRC_LIST})
    string(REPLACE ".cc" "" EXECUTABLE_NAME ${SRC})
    add_executable(${EXECUTABLE_NAME} ${SRC})
    target_link_directories(${EXECUTABLE_NAME} PRIVATE  ${CMAKE_SOURCE_DIR}/build/galay)
    target_link_libraries(${EXECUTABLE_NAME} galay-static)
endforeach()

# if(MYSQL_FOUND)
#     add_executable(test_mysql_client test_msyql_client.cc)
#     target_link_directories(test_mysql_client PRIVATE  ${CMAKE_SOURCE_DIR}/build/galay)
#     target_link_libraries(test_mysql_client galay-static)
# endif()

if(ETCD_FOUND)
    add_executable(tutorial_etcd_discover_client tutorial_etcd_discover_client.cc)
    target_link_directories(tutorial_etcd_discover_client PRIVATE  ${CMAKE_SOURCE_DIR}/build/galay)
    target_link_libraries(tutorial_etcd_discover_client  galay-static)
    add_executable(tutorial_etcd_register_client tutorial_etcd_register_client.cc)
    target_link_directories(tutorial_etcd_register_client PRIVATE  ${CMAKE_SOURCE_DIR}/build/galay)
    target_link_libraries(tutorial_etcd_register_client  galay-static)
endif()

if(MYSQL_FOUND)
    add_executable(tutorial_mysql_client tutorial_mysql_client.cc)
    target_link_directories(tutorial_mysql_client PRIVATE  ${CMAKE_SOURCE_DIR}/build/galay)
    target_link_libraries(tutorial_mysql_client  galay-static)
endif()