
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(MIDDLEWARE)
if(MYSQL_FOUND OR ETCD_FOUND)
    list(APPEND MIDDLEWARE middleware)
endif()

set(DEPEND_LIBS kernel util factory protocol config server client ${MIDDLEWARE} security)
set(STATIC_LIB ${PROJECT_NAME}-static)
set(SHARED_LIB ${PROJECT_NAME}-shared)

# 创建静态库
add_library(${STATIC_LIB} STATIC "")
target_link_libraries(${STATIC_LIB} PRIVATE ${DEPEND_LIBS})

# 创建共享库
add_library(${SHARED_LIB} SHARED "")
target_link_libraries(${SHARED_LIB} PRIVATE ${DEPEND_LIBS})

# 添加依赖库的源文件到静态库和共享库
foreach(depend_lib ${DEPEND_LIBS})
    add_subdirectory(${depend_lib})
    target_sources(${STATIC_LIB} PRIVATE $<TARGET_OBJECTS:${depend_lib}>)
    target_sources(${SHARED_LIB} PRIVATE $<TARGET_OBJECTS:${depend_lib}>)
endforeach()



install(TARGETS ${STATIC_LIB} DESTINATION lib)
install(TARGETS ${SHARED_LIB} DESTINATION lib)

target_compile_features(${STATIC_LIB} PRIVATE cxx_std_20)
target_compile_features(${SHARED_LIB} PRIVATE cxx_std_20)
