#include "control_center.h"
#include "device_manager.h"
#include "logger.h"
#include "command_manager.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

// 简单的终端控制界面
void terminalControl(DeviceManager& dm, ControlCenter& cc) {
    CommandManager cmdManager;  // 创建命令管理器
    
    while (true) {
        std::cout << "\nK2 控制器\n";
        std::cout << "1. 设备列表\n";
        std::cout << "2. 发送指令\n";
        std::cout << "3. 切换控制模式\n";
        std::cout << "4. 切换日志输出 [当前: " 
                  << (Logger::getInstance().isConsoleOutputEnabled() ? "终端+文件" : "仅文件") 
                  << "]\n";
        std::cout << "5. WebSocket 控制\n";
        std::cout << "6. 退出\n";
        std::cout << "选择: ";
        
        int choice;
        std::cin >> choice;
        
        switch(choice) {
            case 1: {
                auto devices = dm.listDevices();
                std::cout << "\n设备:\n";
                for (const auto& id : devices) {
                    auto status = dm.getDeviceStatus(id);
                    std::cout << " - " << id << " [";
                    switch(status) {
                        case DeviceStatus::CONNECTED: std::cout << "已连接"; break;
                        case DeviceStatus::DISCONNECTED: std::cout << "未连接"; break;
                        case DeviceStatus::ACTIVE: std::cout << "在线"; break;
                        case DeviceStatus::ERROR: std::cout << "错误/离线"; break;
                        default: std::cout << "未知";
                    }
                    std::cout << "]\n";
                }
                break;
            }
            case 2: {
                try {
                    std::string deviceId;
                    std::cout << "输入设备id: ";
                    std::cin >> deviceId;
                    
                    // 检查设备是否存在
                    auto devices = dm.listDevices();
                    if (std::find(devices.begin(), devices.end(), deviceId) == devices.end()) {
                        std::cout << "设备 " << deviceId << " 不存在！\n";
                        break;
                    }
                    
                    // 获取设备类型（这里简化处理，假设所有设备都是CAN类型）
                    std::string deviceType = "CAN";
                    
                    // 让用户选择命令
                    CommandInfo selectedCmd = cmdManager.selectCommand(deviceType);
                    
                    // 获取命令参数
                    std::vector<uint8_t> data = cmdManager.getCommandParameters(selectedCmd);
                    
                    // 发送命令
                    std::cout << "\n发送命令: " << selectedCmd.name << " (0x" 
                              << std::hex << static_cast<int>(selectedCmd.command) << std::dec << ")\n";
                    
                    cc.sendCommand(deviceId, selectedCmd.command, data.data());
                    std::cout << "命令发送完成.\n";
                    
                } catch (const std::exception& e) {
                    std::cout << "发送命令时出错: " << e.what() << "\n";
                }
                break;
            }
            case 3: {
                std::cout << "控制模式:\n";
                std::cout << "1. 终端\n";
                std::cout << "2. WebSocket\n";
                std::cout << "3. MQTT\n";
                std::cout << "选择模式: ";
                
                int modeChoice;
                std::cin >> modeChoice;
                
                ControlMode mode;
                switch(modeChoice) {
                    case 1: mode = ControlMode::TERMINAL; break;
                    case 2: mode = ControlMode::WEBSOCKET; break;
                    case 3: mode = ControlMode::MQTT; break;
                    default:
                        std::cout << "无效选择\n";
                        continue;
                }
                cc.setControlMode(mode);
                break;
            }
            case 4: {
                bool currentState = Logger::getInstance().isConsoleOutputEnabled();
                Logger::getInstance().setConsoleOutput(!currentState);
                std::cout << "日志输出已切换为: " 
                          << (!currentState ? "终端+文件" : "仅文件") << "\n";
                break;
            }
            case 5: {
                std::cout << "WebSocket 控制选项:\n";
                std::cout << "1. 启动 WebSocket 客户端\n";
                std::cout << "2. 停止 WebSocket 客户端\n";
                std::cout << "3. 查看连接状态\n";
                std::cout << "选择: ";
                
                int wsChoice;
                std::cin >> wsChoice;
                
                switch(wsChoice) {
                    case 1: {
                        std::string server_ip;
                        unsigned short port;
                        
                        std::cout << "输入服务器IP [默认: 115.190.42.239]: ";
                        std::cin.ignore();
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
                        
                        cc.startWebSocketClient(server_ip, port);
                        cc.setControlMode(ControlMode::WEBSOCKET);
                        break;
                    }
                    case 2:
                        cc.stopWebSocketClient();
                        cc.setControlMode(ControlMode::TERMINAL);
                        break;
                    case 3:
                        std::cout << "WebSocket 连接状态: " 
                                  << (cc.isWebSocketConnected() ? "已连接" : "未连接") << "\n";
                        break;
                    default:
                        std::cout << "无效选择\n";
                }
                break;
            }
            case 6:
                return;
            default:
                std::cout << "无效选择\n";
        }
    }
}

int main() {
    // 初始化日志系统
    Logger::getInstance().setLogFile("logs/device_control.log");
    LOG_INFO("K2 控制器启动...");
    
    // 创建设备管理器
    DeviceManager deviceManager;
    CANInterface can0("can0");
    
    // 初始化CAN接口
    if (!can0.init()) {
        LOG_ERROR("CAN接口初始化失败");
        return -1;
    }
    LOG_INFO("CAN接口初始化成功");
    
    // 添加设备
    deviceManager.addDevice("CAN", "motor_4", can0);
    // deviceManager.addDevice("CAN", "motor2");
    // deviceManager.addDevice("RS232", "relay");
    
    // 连接设备
    deviceManager.connectDevice("motor_4");
    // deviceManager.connectDevice("motor2");
    // deviceManager.connectDevice("relay");
    
    // 等待设备连接稳定
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 发送速度控制命令让电机运行
    int32_t speedControl = 100;
    uint8_t data[8] = {0}; // 初始化所有元素为0
    data[4] = static_cast<uint8_t>(speedControl & 0xFF);
    data[5] = static_cast<uint8_t>((speedControl >> 8) & 0xFF);
    data[6] = static_cast<uint8_t>((speedControl >> 16) & 0xFF);
    data[7] = static_cast<uint8_t>((speedControl >> 24) & 0xFF);
    LOG_INFO("发送速度控制命令: " + std::to_string(speedControl));
    deviceManager.sendCommand("motor_4", MOTOR_SPEED_FEEDBACK_CONTROL, data);
    
    // 等待一段时间让电机开始运行
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    // 创建控制中心
    ControlCenter controlCenter(deviceManager);
    
    // 启动终端控制界面
    std::thread terminalThread([&]() {
        terminalControl(deviceManager, controlCenter);
    });
    
    // 这里可以添加WebSocket和MQTT的初始化代码
    // ...
    
    terminalThread.join();
    
    // 断开所有设备
    for (const auto& id : deviceManager.listDevices()) {
        deviceManager.disconnectDevice(id);
    }
    
    LOG_INFO("K2 控制器已关闭.");
    return 0;
}