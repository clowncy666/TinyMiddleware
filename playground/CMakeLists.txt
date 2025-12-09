# 这里的代码仅仅是为了测试，不会打包进最终产品

# 示例：添加一个名为 day01_test 的可执行程序
add_executable(day01_test day01_eventfd/test_eventfd.cpp)
# 如果这个测试程序需要用 DDS，就链接它
target_link_libraries(day01_test fastrtps fastcdr)


add_executable(day02_dds day02_dds_hello/dds_helloworld.cpp)
target_link_libraries(day02_dds fastrtps fastcdr)

add_executable(day03_threadpool day03_threadpool/test_poll.cpp)
target_link_libraries(day03_threadpool fastrtps fastcdr)