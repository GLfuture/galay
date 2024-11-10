# 添加依赖库的源文件
foreach(depend_lib ${DEPEND_LIBS})
    add_subdirectory(${depend_lib})
endforeach()