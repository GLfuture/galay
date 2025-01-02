option(ENABLE_DEBUG "Enable debug" OFF)
option(ENABLE_INSTALL_SYSTEM "Enable install system" ON) 
option(ENABLE_BUILD_ETCD "Enable build mysql" OFF)
option(ENABLE_BUILD_MYSQL "Enable build mysql" OFF)
option(ENABLE_BUILD_REDIS "Enable build redis" ON)


#gtest
find_path(GTEST_INCLUDE_DIR NAMES gtest)
find_library(GTEST_LIB NAMES gtest)
if (GTEST_INCLUDE_DIR AND GTEST_LIB) 
  option(ENABLE_GTEST "Enable gtest" ON)
else()
  option(ENABLE_GTEST "Not Enable gtest" OFF)
endif()

#mysql
find_path(MYSQL_INCLUDE_DIR NAMES mysql)
find_library(MYSQL_LIBRARY NAMES mysqlclient)

if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARY AND ENABLE_BUILD_MYSQL)
  set(MYSQL_FOUND TRUE)
endif()

if(MYSQL_FOUND)
  option(ENABLE_MYSQL "Enable mysql" ON)
else()
  option(ENABLE_MYSQL "Not Enable mysql" OFF)
endif()

#redis
find_path(REDIS_INCLUDE_DIR NAMES hiredis)
find_library(REDIS_LIBRARY NAMES hiredis)

if(REDIS_INCLUDE_DIR AND REDIS_LIBRARY AND ENABLE_BUILD_REDIS)
  set(REDIS_FOUND TRUE)
endif()

if(REDIS_FOUND)
  find_package(Hiredis REQUIRED)
  option(ENABLE_REDIS "Enable redis" ON)
else()
  option(ENABLE_REDIS "Not Enable redis" OFF)
endif()

#find etcd
find_path(ETCD_INCLUDE_DIR NAMES etcd)
find_library(ETCD_LIBRARY NAMES etcd-cpp-api)
find_library(CPPREST_LIBRARY NAMES cpprest)

if(ETCD_INCLUDE_DIR AND ETCD_LIBRARY AND CPPREST_LIBRARY AND ENABLE_BUILD_ETCD)
  set(ETCD_FOUND TRUE)
endif()

if(ETCD_FOUND)
  find_package(etcd-cpp-api REQUIRED)
  option(ENABLE_ETCD "Enable etcd" ON)
else()
  option(ENABLE_ETCD "Not Enable etcd" OFF)
endif()

#json
find_path(NLOHMANN_JSON_INCLUDE_DIR NAMES nlohmann/json.hpp)
if(NLOHMANN_JSON_INCLUDE_DIR)
  set(NLOHMANN_JSON_FOUND TRUE)
endif()

if(NLOHMANN_JSON_FOUND)
  option(ENABLE_NLOHMANN_JSON "Enable nlohmann/json" ON)
else()
  option(ENABLE_NLOHMANN_JSON "Not Enable nlohmann/json" OFF)
endif()

# all
if(ENABLE_MYSQL OR ENABLE_ETCD OR ENABLE_REDIS)
  message(STATUS "Enable middleware")
  option(ENABLE_MIDDLEWARE "Enable middleware" ON)
else()
  option(ENABLE_MIDDLEWARE "Not Enable middleware" OFF)
endif()

# log

option(ENABLE_TRACE_LOG "Enable trace log" ON)

