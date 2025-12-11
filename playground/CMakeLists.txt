find_package(Threads REQUIRED)
add_executable(day01_test day01_eventfd/test_eventfd.cpp)
# 如果这个测试程序需要用 DDS，就链接它
target_link_libraries(day01_test Threads::Threads)



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