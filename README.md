# TinyMiddleware

一个从零实现的轻量级机器人中间件，以 **eProsima Fast DDS** 为传输层，在其上封装了类 ROS2 的 `Node` / `Publisher` / `Subscription` / `Timer` API，并通过 **epoll + eventfd + timerfd** 打通 DDS 内部回调线程与单线程 Reactor 事件循环，实现零忙等、低延迟的消息驱动架构。

```
DDS 内部回调线程              主线程 (EventLoop)
  (FastDDS thread)
        │                          │
  on_data_available()              │ epoll_wait(-1)  ← 阻塞
        │                          │
   write(eventfd, 1)  ─────────►  │ EPOLLIN 触发
                                   │ read(eventfd)   ← 清零
                                   │ take_next_sample()
                                   │ pool_->enqueue(callback)
                                   ▼
                             ThreadPool worker
                             callback(msg)
```

---

## 目录

- [特性](#特性)
- [架构](#架构)
- [快速开始](#快速开始)
- [核心 API](#核心-api)
- [项目结构](#项目结构)
- [分阶段开发历程](#分阶段开发历程)
- [性能基准](#性能基准)
- [Docker 构建](#docker-构建)

---

## 特性

| 能力 | 实现方式 |
|------|---------|
| 发布/订阅通信 | eProsima Fast DDS，IDL 定义消息类型 |
| 共享内存传输 | Fast DDS SHM Profile，同主机零拷贝 |
| 单线程事件循环 | `epoll` + `eventfd` 桥接 DDS 回调 |
| 异步任务执行 | `ThreadPool`（`condition_variable` 工作队列） |
| 周期定时器 | `timerfd_create` 内核定时器 |
| 全局参数服务 | Redis + `hiredis`（`set/get_parameter`） |
| 泛型 Pub/Sub | C++ 模板 + `MsgTypeSupportTraits` 特化 |
| Python 绑定 | `pybind11` |
| 推理集成 | TensorRT（视觉编码器 + 去噪 U-Net） |

---

## 架构

### 核心类关系

```
Node
 ├── EventLoop          epoll Reactor（单线程）
 │     └── fd → Callback map
 ├── ThreadPool         工作线程池（n 个 worker）
 ├── DomainParticipant  Fast DDS 参与者
 ├── Subscriber         DDS 订阅者实体
 ├── Publisher          DDS 发布者实体
 ├── SubContext[]       每个订阅的上下文（topic / reader / listener / eventfd）
 ├── TimerContext[]     每个定时器的 timerfd
 └── ParamClient        Redis hiredis 参数客户端
```

### DDS ↔ Reactor 桥接

Fast DDS 在自己的内部线程中触发 `on_data_available()`，而用户回调必须安全运行在应用线程。桥接方案：

```
[Fast DDS 内部线程]                [主线程 EventLoop]
   BridgeListener
   on_data_available()
      write(eventfd, 1)   ──────►  epoll_wait 返回
                                   read(eventfd)
                                   take_next_sample() × N
                                   pool_->enqueue(cb)
```

每个 `create_subscription` 调用都会独立创建一个 `eventfd`，各 topic 完全隔离，互不影响。

### 定时器实现

```cpp
// timerfd_create 创建内核定时器，注册为 epoll Channel
timerfd_settime(tfd, 0, &spec, nullptr);   // 设置周期
loop_->addEvent(tfd, [tfd, callback]() {
    uint64_t exp; read(tfd, &exp, 8);      // 读取并清零触发次数
    callback();
});
```

---

## 快速开始

### 系统依赖

```bash
# Ubuntu 22.04
sudo apt install build-essential cmake git libssl-dev \
                 libasio-dev libtinyxml2-dev \
                 libhiredis-dev redis-server
```

### 安装 Fast DDS

```bash
# Fast-CDR
git clone https://github.com/eProsima/Fast-CDR.git && cd Fast-CDR
mkdir build && cd build && cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make -j$(nproc) && sudo make install && cd ../..

# foonathan_memory
git clone https://github.com/foonathan/memory.git && cd memory
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local \
         -DFOONATHAN_MEMORY_BUILD_EXAMPLES=OFF \
         -DFOONATHAN_MEMORY_BUILD_TESTS=OFF
make -j$(nproc) && sudo make install && cd ../..

# Fast-DDS
git clone https://github.com/eProsima/Fast-DDS.git && cd Fast-DDS
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local \
         -DCOMPILE_TOOLS=OFF -DCOMPILE_EXAMPLES=OFF
make -j$(nproc) && sudo make install && sudo ldconfig && cd ../..
```

### 构建项目

```bash
cmake -B build
cmake --build build -j$(nproc)
```

### 运行示例

```bash
# 终端 1：发布者（定时发布传感器数据）
./build/playground/day09_timer/timer_demo

# 终端 2：订阅者（接收并打印）
./build/playground/day08_testpubsub/sub_node

# 参数服务（Redis 必须已启动）
redis-server --daemonize yes
./build/playground/day10_redis_param/param_demo
```

---

## 核心 API

### 创建节点

```cpp
#include "tiny_mw/node/Node.h"

tiny_mw::node::Node node("MyNode");
```

### 发布消息

```cpp
// 1. 特化 Traits（每种消息类型执行一次）
template<>
struct MsgTypeSupportTraits<HelloWorld> {
    using PubSubType = HelloWorldPubSubType;
};

// 2. 创建发布者并发送
auto pub = node.create_publisher<HelloWorld>("SensorTopic");

HelloWorld msg;
msg.index(42);
msg.message("hello");
pub->publish(msg);
```

### 订阅消息

```cpp
node.create_subscription<HelloWorld>("SensorTopic",
    [](const HelloWorld& msg) {
        std::cout << "Received: " << msg.message() << std::endl;
    });

node.spin();  // 启动事件循环（阻塞）
```

### 周期定时器

```cpp
// 每 500ms 触发一次
auto timer = node.create_wall_timer(500, []() {
    std::cout << "Heartbeat" << std::endl;
});
```

### 全局参数服务

```cpp
// 设置参数（写入 Redis）
node.set_parameter("max_speed", "1.5");

// 读取参数（支持运行时动态修改）
std::string speed = node.get_parameter("max_speed");
```

---

## 项目结构

```
TinyMiddleware/
├── CMakeLists.txt
├── Dockerfile
├── shm_profile.xml              # Fast DDS 共享内存 QoS 配置
├── include/tiny_mw/
│   ├── core/
│   │   ├── EventLoop.h          # epoll Reactor（addEvent / run / stop）
│   │   ├── ThreadPool.h         # condition_variable 工作队列
│   │   └── ParamClient.h        # Redis hiredis 封装
│   ├── node/
│   │   └── Node.h               # 顶层 Node 类（模板 Pub/Sub/Timer API）
│   └── transport/
│       └── DdsParticipant.h
├── src/
│   ├── core/
│   │   ├── EventLoop.cpp
│   │   ├── ThreadPool.cpp
│   │   └── ParamClient.cpp
│   ├── node/Node.cpp
│   ├── generated/               # IDL 生成的序列化代码
│   │   ├── HelloWorld*.cxx/.hpp
│   │   ├── PingPong*.cxx/.hpp
│   │   └── BlobMsg*.cxx/.hpp
│   └── python_bindings.cpp      # pybind11 Python 模块
├── apps/                        # 生产应用入口
└── playground/
    ├── day01_eventfd/           # 原始 eventfd + epoll 实验
    ├── day02_dds/               # 原始 Fast DDS API 实验
    ├── day03_integration/       # epoll + DDS 桥接原型
    ├── day04_refactor/          # 抽象 EventLoop 类
    ├── day05_threadpool/        # 加入 ThreadPool
    ├── day06_full_reactor/      # 完整 Reactor
    ├── day07_node_class/        # 封装 Node 类
    ├── day08_generic_pub/       # 泛型 Publisher
    ├── day09_timer/             # timerfd 定时器
    ├── day10_redis_param/       # Redis 参数服务
    ├── day11_latency/           # 延迟分析
    ├── day12_protobuf/          # Protobuf 序列化实验
    ├── Benchmark/               # 延迟 & 吞吐量基准测试
    ├── Vision/                  # 模拟摄像头（224×224 图像流）
    ├── Inference/               # TensorRT 视觉 + U-Net 推理节点
    └── Action/                  # Action 消息类型
```

---

## 分阶段开发历程

| 阶段 | 内容 | 关键技术 |
|------|------|---------|
| Day 01 | `eventfd` + `epoll` 原始实验 | `eventfd(0, EFD_NONBLOCK)`, `epoll_ctl` |
| Day 02 | Fast DDS HelloWorld 原始 API | `DomainParticipant`, `DataReader`, `DataWriter` |
| Day 03 | epoll ↔ DDS 桥接原型 | `BridgeListener` 写 eventfd，epoll 唤醒主线程 |
| Day 04 | 重构 `EventLoop` 类 | `addEvent(fd, cb)`, `run()`, `stop()` |
| Day 05 | 加入 `ThreadPool` | `condition_variable` 工作队列，消息回调异步执行 |
| Day 06 | 完整 Reactor 闭环 | EventLoop + ThreadPool + DDS 三者协作 |
| Day 07 | 封装 `Node` 类 | `create_subscription()` / `spin()` 首版 |
| Day 08 | 泛型 Pub/Sub | `MsgTypeSupportTraits<T>` 模板特化机制 |
| Day 09 | `timerfd` 定时器 | `timerfd_create(CLOCK_MONOTONIC)`, `create_wall_timer()` |
| Day 10 | Redis 参数服务 | `hiredis` 封装，`set/get_parameter()` |
| Day 11 | 延迟分析 | SHM 本地回环 RTT 测量 |
| Day 12 | Protobuf 序列化 | 与 IDL 方案对比 |
| Day 13 | Docker 化 | 完整依赖打包，可移植部署 |

---

## 性能基准

基于 Fast DDS **共享内存（SHM）传输**，同主机进程间通信：

### 延迟（bench_latency）

```
终端1: ./build/playground/Benchmark/bench_latency pong
终端2: ./build/playground/Benchmark/bench_latency ping 500
```

测量 ping-pong RTT / 2（单向延迟估计），典型结果（Ubuntu 22.04 / WSL2）：

```
====== Latency Report (SHM, intra-host) ======
Samples :  500
Min     :  ~80    us
Mean    :  ~150   us
P50     :  ~120   us
P95     :  ~300   us
P99     :  ~500   us
==============================================
```

### 吞吐量（bench_throughput）

```
终端1: ./build/playground/Benchmark/bench_throughput sub
终端2: ./build/playground/Benchmark/bench_throughput pub 100   # 100us 间隔 ≈ 10k msg/s
```

Sub 端每秒输出接收速率、丢包数及 CPU 占用。

---

## Docker 构建

项目提供完整 `Dockerfile`，包含 Fast-CDR / foonathan_memory / Fast-DDS 的编译安装，无需手动配置依赖：

```bash
# 构建镜像（需将 Fast-CDR/memory/Fast-DDS 源码放入 third_party/）
docker build -t tinymiddleware .

# 运行
docker run --rm tinymiddleware
```

---

## 开发环境

- **OS**: Linux (Ubuntu 22.04 / WSL2)
- **Compiler**: GCC 11+，C++17
- **Build**: CMake 3.10+
- **DDS**: eProsima Fast DDS 2.x
- **Transport**: Fast DDS SHM（共享内存）/ UDP
- **Param**: Redis 7.x + hiredis
- **Optional**: pybind11（Python 绑定），TensorRT（推理节点）
