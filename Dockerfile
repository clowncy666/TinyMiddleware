# 1. 基础镜像
FROM ubuntu:22.04

# 2. 环境变量
ENV DEBIAN_FRONTEND=noninteractive

# 3. 安装系统依赖
# (这里为了稳妥，添加了阿里云镜像源加速 apt 下载，既然你网络不好，这对你有帮助)
RUN sed -i 's/archive.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list && \
    sed -i 's/security.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list && \
    apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    libssl-dev \
    libasio-dev \
    libtinyxml2-dev \
    redis-server \
    libhiredis-dev \
    protobuf-compiler \
    libprotobuf-dev \
    libpcre3-dev \
    unzip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /tmp

# =========================================================
#  核心修改：使用 COPY 代替 git clone
#  前提：你的 third_party 目录下必须已经有 Fast-CDR, memory, Fast-DDS 三个文件夹
# =========================================================

# 4. 拷贝并安装 Fast-CDR
COPY third_party/Fast-CDR /tmp/Fast-CDR
RUN cd /tmp/Fast-CDR && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local && \
    make -j$(nproc) && \
    make install && \
    cd /tmp && rm -rf Fast-CDR

# 5. 拷贝并安装 foonathan_memory
COPY third_party/memory /tmp/memory
RUN cd /tmp/memory && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local -DFOONATHAN_MEMORY_BUILD_EXAMPLES=OFF -DFOONATHAN_MEMORY_BUILD_TESTS=OFF -DFOONATHAN_MEMORY_BUILD_TOOLS=OFF && \
    make -j$(nproc) && \
    make install && \
    cd /tmp && rm -rf memory

# 6. 拷贝并安装 Fast-DDS
COPY third_party/Fast-DDS /tmp/Fast-DDS
RUN cd /tmp/Fast-DDS && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local -DCOMPILE_TOOLS=OFF -DCOMPILE_EXAMPLES=OFF && \
    make -j$(nproc) && \
    make install && \
    ldconfig && \
    cd /tmp && rm -rf Fast-DDS

# =========================================================
#  下面是编译你自己的项目
# =========================================================

# 7. 设置工作目录
WORKDIR /app

# 8. 拷贝代码
# 注意：这步会把你项目下的所有文件拷进去。
# 建议在项目根目录建一个 .dockerignore 文件，里面写上 third_party，
# 否则它会把那几百兆源码又拷一次进 /app 目录（虽然不影响运行，但镜像会变大）。
COPY . /app

# 9. 编译项目
RUN rm -rf build && mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc)

# 10. 启动命令
CMD service redis-server start && ./build/playground/day12_proto