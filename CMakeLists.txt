cmake_minimum_required(VERSION 3.10)
project(TinyMiddleware)

# 设置 C++ 标准 (C++14 或 C++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 寻找 Fast DDS 库 (核心步骤)
#find_package(fastrtps REQUIRED)
#find_package(fastcdr REQUIRED)

# 包含头文件路径
include_directories(${CMAKE_SOURCE_DIR}/include)

# 添加子目录
add_subdirectory(src)
add_subdirectory(apps)
add_subdirectory(playground) # 你的练手代码