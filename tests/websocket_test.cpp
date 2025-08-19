#include "websocket_client.h"
#include "control_center.h"
#include "device_manager.h"
#include "logger.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // 初始化日志系统
    Logger::getInstance().setLogFile("logs/websocket_test.log");
    Logger::getInstance().setConsoleOutput(true);
    Logger::getInstance().log(LogLevel::INFO, "WebSocket 测试程序启动...");
    
    // 创建设备管理器和控制中心
    DeviceManager deviceManager;
    ControlCenter controlCenter(deviceManager);
    
    std::cout << "WebSocket 客户端测试程序\n";
    std::cout << "========================\n";
    
    std::string server_ip;
    unsigned short port;
    
    std::cout << "输入服务器IP [默认: 115.190.42.239]: ";
    std::getline(std::cin, server_ip);
    if (server_ip.empty()) {
        server_ip = "115.190.42.239";
    }
    
    std::cout << "输入端口 [默认: 8080]: ";
    std::string portStr;
    std::getline(std::cin, portStr);
    if (portStr.empty()) {
        port = 8080;
    } else {
        port = static_cast<unsigned short>(std::stoi(portStr));
    }
    
    std::cout << "正在连接到 " << server_ip << ":" << port << "...\n";
    
    // 启动 WebSocket 客户端
    controlCenter.startWebSocketClient(server_ip, port);
    controlCenter.setControlMode(ControlMode::WEBSOCKET);
    
    std::cout << "WebSocket 客户端已启动。输入 'quit' 退出。\n";
    std::cout << "连接状态: " << (controlCenter.isWebSocketConnected() ? "已连接" : "未连接") << "\n";
    
    // 等待用户输入退出命令
    std::string input;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        
        if (input == "quit" || input == "exit") {
            break;
        } else if (input == "status") {
            std::cout << "连接状态: " << (controlCenter.isWebSocketConnected() ? "已连接" : "未连接") << "\n";
        } else if (input == "help") {
            std::cout << "可用命令:\n";
            std::cout << "  status - 查看连接状态\n";
            std::cout << "  quit/exit - 退出程序\n";
            std::cout << "  help - 显示帮助信息\n";
        } else if (!input.empty()) {
            std::cout << "未知命令: " << input << "，输入 'help' 查看帮助\n";
        }
    }
    
    std::cout << "正在关闭 WebSocket 客户端...\n";
    controlCenter.stopWebSocketClient();
    
    Logger::getInstance().log(LogLevel::INFO, "WebSocket 测试程序结束");
    std::cout << "测试程序已退出。\n";
    
    return 0;
}
