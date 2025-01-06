project(security)

file(GLOB SRC_LIST "*.cc")

add_library(${PROJECT_NAME} OBJECT ${SRC_LIST})
link_libraries(${PROJECT_NAME} ssl crypto)
install(TARGETS security EXPORT GalayTargets)