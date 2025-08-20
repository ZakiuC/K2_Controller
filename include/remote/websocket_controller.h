#pragma once

#include "websocket_client.h"
#include "device_manager.h"
#include "control_center.h"
#include "device_protocol.h"
#include <memory>
#include <thread>
#include <atomic>

class WebSocketController {
public:
    WebSocketController(DeviceManager& dm, ControlCenter& cc, 
                       const std::string& host = "115.190.42.239", 
                       const std::string& port = "8080");
    ~WebSocketController();
    
    // 启动和停止WebSocket控制
    bool start();
    void stop();
    bool isRunning() const;
    
    // 设备状态更新通知
    void notifyDeviceStatusChange(const std::string& deviceId, const std::string& status);

private:
    DeviceManager& deviceManager_;
    ControlCenter& controlCenter_;
    std::unique_ptr<WebSocketClient> wsClient_;
    std::thread controlThread_;
    std::atomic<bool> running_{false};
    
    // WebSocket消息处理
    void handleWebSocketMessage(const WebSocketMessage& message);
    
    // 命令处理
    void handleDeviceCommand(const WebSocketMessage& message);
    void handleSystemCommand(const WebSocketMessage& message);
    void handleQueryCommand(const WebSocketMessage& message);
    
    // 工具方法
    uint8_t parseCommand(const std::string& commandStr);
    std::vector<uint8_t> parseData(const std::string& dataStr);
    std::string getDeviceStatusString(DeviceStatus status);
};
