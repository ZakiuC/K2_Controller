#!/bin/bash

# K2_Controller启动脚本

echo "K2_Controller启动脚本"
echo "======================"

# 检查可执行文件是否存在
if [ ! -f "/root/K2_Controller/bin/K2_Controler" ]; then
    echo "错误: 可执行文件不存在，请先编译项目"
    echo "运行: cd /root/K2_Controller && mkdir -p build && cd build && cmake .. && make"
    exit 1
fi

# 创建日志目录
mkdir -p /root/K2_Controller/logs

# 显示菜单
echo "1. 启动K2_Controller (终端模式)"
echo "2. 启动K2_Controller (WebSocket模式)"
echo "3. 仅测试WebSocket连接"
echo "4. 查看日志"
echo "5. 退出"
echo ""
read -p "请选择选项 (1-5): " choice

case $choice in
    1)
        echo "启动K2_Controller - 终端模式"
        cd /root/K2_Controller
        ./bin/K2_Controler
        ;;
    2)
        echo "启动K2_Controller - WebSocket模式"
        echo "注意: 启动后请在终端界面选择'3. 切换控制模式' -> '2. WebSocket'"
        cd /root/K2_Controller
        ./bin/K2_Controler
        ;;
    3)
        echo "测试WebSocket连接到 115.190.42.239:8080"
        cd /root/K2_Controller/tests/websocket_chat/build/client
        ./websocket_client 115.190.42.239 8080
        ;;
    4)
        echo "查看日志文件:"
        if [ -f "/root/K2_Controller/logs/device_control.log" ]; then
            tail -f /root/K2_Controller/logs/device_control.log
        else
            echo "日志文件不存在"
        fi
        ;;
    5)
        echo "退出"
        exit 0
        ;;
    *)
        echo "无效选择"
        exit 1
        ;;
esac
