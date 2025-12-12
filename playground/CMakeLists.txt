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