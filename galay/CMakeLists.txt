include_directories(${CMAKE_CURRENT_SOURCE_DIR})
# 添加依赖库的源文件
foreach(depend_lib ${DEPEND_LIBS})
    add_subdirectory(${depend_lib})
endforeach()