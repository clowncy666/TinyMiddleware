# --- Day 01:epoll ---
find_package(Threads REQUIRED)
add_executable(day01_test day01_eventfd/test_eventfd.cpp)
# 如果这个测试程序需要用 DDS，就链接它
target_link_libraries(day01_test Threads::Threads)


# --- Day 02:DDShello  ---
# 生成的文件列表 (由 fastddsgen 生成的)
set(IDL_SOURCES
    day02_dds/HelloWorldTypeObjectSupport.cxx
    day02_dds/HelloWorldPubSubTypes.cxx
)
# 发布者程序
add_executable(dds_publisher 
    day02_dds/my_writer.cpp 
    ${IDL_SOURCES}
)
target_link_libraries(dds_publisher fastdds fastcdr)

# 订阅者程序
add_executable(dds_subscriber 
    day02_dds/my_reader.cpp 
    ${IDL_SOURCES}
)
target_link_libraries(dds_subscriber fastdds fastcdr)

# --- Day 03: Integration ---
find_program(FASTDDSGEN_EXE fastddsgen REQUIRED)
set(IDL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/day03_integration/HelloWorld.idl)
set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/day03_gen)
file(MAKE_DIRECTORY ${GEN_DIR})
add_custom_command(
    OUTPUT 
        ${GEN_DIR}/HelloWorld.hpp              
        ${GEN_DIR}/HelloWorldPubSubTypes.cxx
        ${GEN_DIR}/HelloWorldPubSubTypes.hpp
    COMMAND ...
)
add_executable(day03_integration
    day03_integration/main.cpp
    
    ${GEN_DIR}/HelloWorldPubSubTypes.cxx 
    ${GEN_DIR}/HelloWorldTypeObjectSupport.cxx
)

target_include_directories(day03_integration PRIVATE ${GEN_DIR})
target_link_libraries(day03_integration fastdds fastcdr Threads::Threads)


add_executable(day04_refactor day04_refactor/main.cpp)
target_link_libraries(day04_refactor tiny_core Threads::Threads)

add_executable(day05_pool day05_threadpool/main.cpp)
target_link_libraries(day05_pool tiny_core Threads::Threads)


# --- Day 06: Full Reactor ---

# 1. 设置 IDL 生成路径
set(D06_IDL ${CMAKE_CURRENT_SOURCE_DIR}/day06_full_reactor/HelloWorld.idl)
set(D06_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/day06_gen)
file(MAKE_DIRECTORY ${D06_GEN_DIR})

# 2. 生成命令
add_custom_command(
    OUTPUT ${D06_GEN_DIR}/HelloWorld.h ${D06_GEN_DIR}/HelloWorldPubSubTypes.cxx ${D06_GEN_DIR}/HelloWorldPubSubTypes.h ${D06_GEN_DIR}/HelloWorldTypeObjectSupport.cxx ${D06_GEN_DIR}/HelloWorldTypeObjectSupport.h

    COMMAND ${FASTDDSGEN_EXE} -replace -d ${D06_GEN_DIR} ${D06_IDL}
    DEPENDS ${D06_IDL}
)

# 3. 生成可执行文件
add_executable(day06_reactor 
    day06_full_reactor/main.cpp
    ${D06_GEN_DIR}/HelloWorldPubSubTypes.cxx
    ${D06_GEN_DIR}/HelloWorldTypeObjectSupport.cxx
)

# 4. 包含路径
target_include_directories(day06_reactor PRIVATE ${D06_GEN_DIR})

# 5. 【关键】链接：FastDDS + 你的核心库(EventLoop/ThreadPool) + 线程库
target_link_libraries(day06_reactor 
    fastdds fastcdr 
    tiny_core 
    Threads::Threads
)
# --- Day 07: node ---
add_executable(day07_node day07_node_class/main.cpp)

target_link_libraries(day07_node tiny_core fastdds fastcdr Threads::Threads)