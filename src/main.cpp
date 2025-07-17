#include "control_center.h"
#include "device_manager.h"
#include "logger.h"
#include <iostream>
#include <thread>
#include <vector>

// 简单的终端控制界面
void terminalControl(DeviceManager& dm, ControlCenter& cc) {
    while (true) {
        std::cout << "\nK2 控制器\n";
        std::cout << "1. 设备列表\n";
        std::cout << "2. 发送指令\n";
        std::cout << "3. 切换控制模式\n";
        std::cout << "4. 退出\n";
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
                std::string deviceId;
                std::cout << "输入设备id: ";
                std::cin >> deviceId;
                
                // 简化命令输入
                uint8_t command = 0x00;
                cc.sendCommand(deviceId, command);
                std::cout << "命令发送.\n";
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
            case 4:
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
    
    // 添加设备
    deviceManager.addDevice("CAN", "motor_1", can0);
    // deviceManager.addDevice("CAN", "motor2");
    // deviceManager.addDevice("RS232", "relay");
    
    // 连接设备
    deviceManager.connectDevice("motor_1");
    // deviceManager.connectDevice("motor2");
    // deviceManager.connectDevice("relay");
    
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