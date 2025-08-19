#!/bin/bash

echo "K2_Controller 编译脚本 (包含 WebSocket 支持)"
echo "==============================================="

# 检查是否需要安装依赖
if ! dpkg -l | grep -q libnlohmann-json3-dev || ! dpkg -l | grep -q libboost-dev; then
    echo "警告: 检测到缺少依赖库"
    echo "是否自动安装依赖? (y/n)"
    read -r response
    if [[ "$response" =~ ^[Yy]$ ]]; then
        ./install_deps.sh
    else
        echo "请手动安装依赖库后再编译："
        echo "sudo apt-get install libboost-all-dev libnlohmann-json3-dev"
        exit 1
    fi
fi

# 创建构建目录
if [ ! -d "build" ]; then
    mkdir build
fi

cd build

# 运行 CMake 配置
echo "运行 CMake 配置..."
cmake -DENABLE_REMOTE_CONTROL=ON -DBUILD_TESTS=ON ..

if [ $? -ne 0 ]; then
    echo "CMake 配置失败!"
    exit 1
fi

# 编译项目
echo "编译项目..."
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo ""
    echo "编译成功!"
    echo "主程序: bin/K2_Controler"
    echo "WebSocket测试程序: bin/websocket_test"
    echo ""
    echo "运行程序："
    echo "  主程序: ./bin/K2_Controler"
    echo "  WebSocket测试: ./bin/websocket_test"
else
    echo "编译失败!"
    exit 1
fi
