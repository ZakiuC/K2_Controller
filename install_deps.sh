#!/bin/bash

echo "K2_Controller WebSocket 依赖安装脚本"
echo "======================================"

# 更新包管理器
echo "更新包管理器..."
sudo apt-get update

# 安装 Boost 库
echo "安装 Boost 库..."
sudo apt-get install -y libboost-all-dev

# 安装 nlohmann/json 库
echo "安装 nlohmann/json 库..."
sudo apt-get install -y libnlohmann-json3-dev

# 检查安装结果
echo ""
echo "检查安装结果："

# 检查 Boost
if dpkg -l | grep -q libboost-dev; then
    echo "✓ Boost 库已安装"
else
    echo "✗ Boost 库安装失败"
fi

# 检查 nlohmann/json
if dpkg -l | grep -q libnlohmann-json3-dev; then
    echo "✓ nlohmann/json 库已安装"
else
    echo "✗ nlohmann/json 库安装失败"
fi

echo ""
echo "依赖安装完成！现在可以编译项目了："
echo "cd /root/K2_Controller"
echo "./make.sh"
