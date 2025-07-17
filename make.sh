#!/bin/bash


# 进入构建目录
cd ./build

# 运行 CMake 构建系统
cmake ..

# 使用与 CPU 核心数相等的并行线程来构建
make -j$(nproc)

# 返回上一级目录
cd ..
