project(security)

set(SRC_LIST 
    base64.cc md5.cc sha256.cc sha512.cc salt.cc)

add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})
link_libraries(${PROJECT_NAME} ssl crypto)