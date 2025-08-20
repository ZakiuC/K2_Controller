#include "websocket_controller.h"
#include "logger.h"
#include <sstream>
#include <iomanip>

WebSocketController::WebSocketController(DeviceManager& dm, ControlCenter& cc, 
                                       const std::string& host, const std::string& port)
    : deviceManager_(dm), controlCenter_(cc) {
    wsClient_ = std::make_unique<WebSocketClient>(host, port);
    
    // 设置WebSocket消息处理器
    wsClient_->setMessageHandler([this](const WebSocketMessage& msg) {
        handleWebSocketMessage(msg);
    });
    
    LOG_INFO("WebSocket控制器初始化完成");
}

WebSocketController::~WebSocketController() {
    stop();
}

bool WebSocketController::start() {
    if (running_) {
        LOG_WARNING("WebSocket控制器已在运行");
        return true;
    }
    
    // 连接WebSocket服务器
    if (!wsClient_->connect()) {
        LOG_ERROR("WebSocket连接失败");
        return false;
    }
    
    running_ = true;
    
    // 启动控制线程
    controlThread_ = std::thread([this]() {
        LOG_INFO("WebSocket控制线程启动");
        wsClient_->run();
        LOG_INFO("WebSocket控制线程退出");
    });
    
    // 注册到控制中心
    controlCenter_.registerCommandHandler(ControlMode::WEBSOCKET, 
        [this](const std::string& deviceId, uint8_t command, const uint8_t* data) {
            // 当控制中心处于WebSocket模式时，通过WebSocket发送命令确认
            std::string commandStr = "0x" + std::to_string(command);
            std::string dataStr = "";
            if (data) {
                std::stringstream ss;
                ss << std::hex;
                for (int i = 0; i < 8; ++i) {  // 假设数据长度为8
                    ss << std::setfill('0') << std::setw(2) << static_cast<int>(data[i]);
                    if (i < 7) ss << " ";
                }
                dataStr = ss.str();
            }
            wsClient_->sendResponse("command_executed", 
                "设备 " + deviceId + " 命令 " + commandStr + " 已执行");
        });
    
    LOG_INFO("WebSocket控制器启动成功");
    return true;
}

void WebSocketController::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    wsClient_->stop();
    
    if (controlThread_.joinable()) {
        controlThread_.join();
    }
    
    LOG_INFO("WebSocket控制器已停止");
}

bool WebSocketController::isRunning() const {
    return running_ && wsClient_->isConnected();
}

void WebSocketController::notifyDeviceStatusChange(const std::string& deviceId, const std::string& status) {
    if (isRunning()) {
        wsClient_->sendDeviceStatus(deviceId, status);
    }
}

void WebSocketController::handleWebSocketMessage(const WebSocketMessage& message) {
    LOG_DEBUG("处理WebSocket消息: type=" + message.type + ", device=" + message.deviceId + 
              ", command=" + message.command);
    
    if (message.type == "command") {
        handleDeviceCommand(message);
    } else if (message.type == "system") {
        handleSystemCommand(message);
    } else if (message.type == "query") {
        handleQueryCommand(message);
    } else {
        LOG_WARNING("未知的WebSocket消息类型: " + message.type);
        wsClient_->sendResponse("error", "未知的消息类型: " + message.type);
    }
}

void WebSocketController::handleDeviceCommand(const WebSocketMessage& message) {
    try {
        uint8_t command = parseCommand(message.command);
        std::vector<uint8_t> data = parseData(message.data);
        
        // 通过控制中心处理命令
        controlCenter_.processIncomingCommand(ControlMode::WEBSOCKET, message.deviceId, 
                                            command, data.empty() ? nullptr : data.data());
        
        wsClient_->sendResponse("success", 
            "设备 " + message.deviceId + " 命令 " + message.command + " 已发送");
        
        LOG_INFO("WebSocket设备命令已处理: " + message.deviceId + " -> " + message.command);
        
    } catch (const std::exception& e) {
        std::string error = "设备命令处理失败: " + std::string(e.what());
        LOG_ERROR(error);
        wsClient_->sendResponse("error", error);
    }
}

void WebSocketController::handleSystemCommand(const WebSocketMessage& message) {
    if (message.command == "get_devices") {
        // 获取设备列表
        auto devices = deviceManager_.listDevices();
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < devices.size(); ++i) {
            if (i > 0) oss << ",";
            oss << "{\"id\":\"" << devices[i] << "\","
                << "\"status\":\"" << getDeviceStatusString(deviceManager_.getDeviceStatus(devices[i])) << "\"}";
        }
        oss << "]";
        
        wsClient_->sendResponse("device_list", oss.str());
        
    } else if (message.command == "set_control_mode") {
        // 设置控制模式为WebSocket
        controlCenter_.setControlMode(ControlMode::WEBSOCKET);
        wsClient_->sendResponse("success", "控制模式已切换至WebSocket");
        
    } else if (message.command == "get_status") {
        // 获取系统状态
        std::ostringstream oss;
        oss << "{"
            << "\"control_mode\":\"WEBSOCKET\","
            << "\"connected_devices\":" << deviceManager_.listDevices().size() << ","
            << "\"websocket_connected\":" << (wsClient_->isConnected() ? "true" : "false")
            << "}";
        
        wsClient_->sendResponse("system_status", oss.str());
        
    } else {
        wsClient_->sendResponse("error", "未知的系统命令: " + message.command);
    }
}

void WebSocketController::handleQueryCommand(const WebSocketMessage& message) {
    if (message.command == "device_status") {
        auto status = deviceManager_.getDeviceStatus(message.deviceId);
        wsClient_->sendDeviceStatus(message.deviceId, getDeviceStatusString(status));
        
    } else {
        wsClient_->sendResponse("error", "未知的查询命令: " + message.command);
    }
}

uint8_t WebSocketController::parseCommand(const std::string& commandStr) {
    try {
        // 支持十六进制和十进制格式
        if (commandStr.substr(0, 2) == "0x" || commandStr.substr(0, 2) == "0X") {
            return static_cast<uint8_t>(std::stoul(commandStr, nullptr, 16));
        } else {
            return static_cast<uint8_t>(std::stoul(commandStr));
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("无效的命令格式: " + commandStr);
    }
}

std::vector<uint8_t> WebSocketController::parseData(const std::string& dataStr) {
    std::vector<uint8_t> data;
    
    if (dataStr.empty()) {
        return data;
    }
    
    try {
        // 支持JSON数组格式或空格分隔的十六进制字符串
        if (dataStr[0] == '[') {
            // 简单的JSON数组解析
            std::string content = dataStr.substr(1, dataStr.length() - 2); // 去掉 [ ]
            std::istringstream iss(content);
            std::string item;
            
            while (std::getline(iss, item, ',')) {
                // 去掉空格
                item.erase(0, item.find_first_not_of(" \t"));
                item.erase(item.find_last_not_of(" \t") + 1);
                
                int value = std::stoi(item);
                data.push_back(static_cast<uint8_t>(value));
            }
        } else {
            // 空格分隔的十六进制字符串格式
            std::istringstream iss(dataStr);
            std::string hex;
            
            while (iss >> hex) {
                uint8_t value;
                if (hex.substr(0, 2) == "0x" || hex.substr(0, 2) == "0X") {
                    value = static_cast<uint8_t>(std::stoul(hex, nullptr, 16));
                } else {
                    value = static_cast<uint8_t>(std::stoul(hex, nullptr, 16));
                }
                data.push_back(value);
            }
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("无效的数据格式: " + dataStr);
    }
    
    return data;
}

std::string WebSocketController::getDeviceStatusString(DeviceStatus status) {
    switch (status) {
        case DeviceStatus::CONNECTED: return "connected";
        case DeviceStatus::DISCONNECTED: return "disconnected";
        case DeviceStatus::ACTIVE: return "active";
        case DeviceStatus::ERROR: return "error";
        default: return "unknown";
    }
}
