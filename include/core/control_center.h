#pragma once
#include "device_manager.h"
#include <functional>
#include <mutex>
#include <queue>
#include <map>
#include <memory>

// 前向声明
class WebSocketClient;

enum class ControlMode {
    TERMINAL,
    WEBSOCKET,
    MQTT
};

class ControlCenter {
public:
    using CommandHandler = std::function<void(const std::string&, uint8_t command, const uint8_t* data)>;

    ControlCenter(DeviceManager& dm);
    ~ControlCenter(); // 添加析构函数声明

    void setControlMode(ControlMode mode);
    ControlMode getControlMode() const;
    void registerCommandHandler(ControlMode mode, CommandHandler handler);
    void sendCommand(const std::string& deviceId, uint8_t command, const uint8_t *data = nullptr) ;
    void processIncomingCommand(ControlMode source, const std::string& deviceId, uint8_t command, const uint8_t *data = nullptr);

    // WebSocket 相关方法
    void startWebSocketClient(const std::string& server_ip, unsigned short port);
    void stopWebSocketClient();
    bool isWebSocketConnected() const;

private:
    std::string modeToString(ControlMode mode) const;
    std::string handleWebSocketCommand(const std::string& cmd, const std::vector<std::string>& params);

    DeviceManager& deviceManager;
    ControlMode currentMode;
    mutable std::mutex modeMutex;
    std::map<ControlMode, CommandHandler> commandHandlers;
    std::mutex handlerMutex;
    
    // WebSocket 客户端
    std::unique_ptr<WebSocketClient> websocketClient;
};
