set(SOURCES
    core/EventLoop.cpp
)

# 【核心指令】创建一个库 (STATIC 表示静态库 .a，SHARED 表示动态库 .so)
# 这里我们要生成 libtiny_core.a
add_library(tiny_core STATIC ${SOURCES})

# 告诉 CMake 这个库的头文件在哪里
# PUBLIC 意味着：凡是链接了这个库的人，都能自动找到 include/ 目录
target_include_directories(tiny_core PUBLIC 
    ${CMAKE_SOURCE_DIR}/include
)