/**
 * @file control_center.cpp
 * @brief 控制中心实现文件
 * @details 提供设备控制的统一接口，支持多种控制模式（终端、WebSocket、MQTT）
 *          通过注册命令处理器实现灵活的命令分发
 * @author zakiu
 * @date 2025-07-15
 */
#include "control_center.h"
#include "websocket_client.h"
#include "logger.h"

// WebSocket 命令常量（如果不在头文件中定义）
#ifndef WS_TRACK_POSITION_CMD
#define WS_TRACK_POSITION_CMD   "0x0001"
#define WS_TRACK_SPEED_CMD      "0x0002"
#define WS_BRUSH_LIFTING_CMD    "0x0003"
#define WS_EIGHT_SWITCH_CMD     "0x0004"
#define WS_PUSHFORWARD_CMD      "0x0005"
#define WS_VIDEO_STREAM_CMD     "0x0006"
#endif

/**
 * @brief 构造函数，初始化控制中心
 * @param dm 设备管理器的引用
 */
ControlCenter::ControlCenter(DeviceManager& dm) : deviceManager(dm), currentMode(ControlMode::TERMINAL) {}

/**
 * @brief 析构函数，清理资源
 */
ControlCenter::~ControlCenter() {
    stopWebSocketClient();
}

/**
 * @brief 设置控制模式
 * @param mode 要设置的控制模式
 * @details 此方法是线程安全的，使用互斥锁保护模式变更
 */
void ControlCenter::setControlMode(ControlMode mode) {
    std::lock_guard<std::mutex> lock(modeMutex);
    currentMode = mode;
    LOG_INFO("控制模式切换至: " + modeToString(mode));
}

/**
 * @brief 获取当前控制模式
 * @return 当前的控制模式
 * @details 此方法是线程安全的，使用互斥锁保护读取操作
 */
ControlMode ControlCenter::getControlMode() const {
    std::lock_guard<std::mutex> lock(modeMutex);
    return currentMode;
}

/**
 * @brief 注册命令处理器
 * @param mode 控制模式
 * @param handler 命令处理函数
 * @details 为指定的控制模式注册命令处理函数，此方法是线程安全的
 */
void ControlCenter::registerCommandHandler(ControlMode mode, CommandHandler handler) {
    std::lock_guard<std::mutex> lock(handlerMutex);
    commandHandlers[mode] = handler;
}

/**
 * @brief 发送命令到指定设备
 * @param deviceId 目标设备的ID
 * @param command 要发送的命令数据
 * @details 根据当前控制模式决定命令处理方式：
 *          - TERMINAL模式：直接通过设备管理器发送
 *          - 其他模式：使用已注册的命令处理器
 */
void ControlCenter::sendCommand(const std::string& deviceId, uint8_t command, const uint8_t *data) {
    if (getControlMode() == ControlMode::TERMINAL) {
        deviceManager.sendCommand(deviceId, command, data);
    } else {
        std::lock_guard<std::mutex> lock(handlerMutex);
        auto it = commandHandlers.find(getControlMode());
        if (it != commandHandlers.end()) {
            it->second(deviceId, command, data);
        } else {
            LOG_ERROR("当前模式没有命令处理器");
        }
    }
}

/**
 * @brief 处理来自外部控制源的命令
 * @param source 命令来源的控制模式
 * @param deviceId 目标设备的ID  
 * @param command 要发送的命令数据
 * @details 只有当命令来源与当前激活的控制模式一致时，才会执行命令
 */
void ControlCenter::processIncomingCommand(ControlMode source, const std::string& deviceId, uint8_t command, const uint8_t *data) {
    if (source == getControlMode()) {
        deviceManager.sendCommand(deviceId, command, data);
    } else {
        LOG_WARNING("接收到来自非激活控制源的命令");
    }
}

/**
 * @brief 将控制模式转换为字符串表示
 * @param mode 要转换的控制模式
 * @return 控制模式的字符串表示
 * @details 用于日志输出和调试显示
 */
std::string ControlCenter::modeToString(ControlMode mode) const {
    switch(mode) {
        case ControlMode::TERMINAL: return "终端";
        case ControlMode::WEBSOCKET: return "WEBSOCKET";
        case ControlMode::MQTT: return "MQTT";
        default: return "未知";
    }
}

/**
 * @brief 启动 WebSocket 客户端
 * @param server_ip 服务器IP地址
 * @param port 服务器端口
 */
void ControlCenter::startWebSocketClient(const std::string& server_ip, unsigned short port) {
    if (websocketClient && websocketClient->isConnected()) {
        LOG_WARNING("WebSocket 客户端已经在运行");
        return;
    }

    websocketClient = std::make_unique<WebSocketClient>(server_ip, port, "subscriber");
    websocketClient->setControlCenter(this);
    
    // 注册 WebSocket 命令回调
    websocketClient->registerCommandCallback([this](const std::string& cmd, const std::vector<std::string>& params) -> std::string {
        return handleWebSocketCommand(cmd, params);
    });
    
    websocketClient->run();
    LOG_INFO("WebSocket 客户端已启动，连接到: " + server_ip + ":" + std::to_string(port));
}

/**
 * @brief 停止 WebSocket 客户端
 */
void ControlCenter::stopWebSocketClient() {
    if (websocketClient) {
        websocketClient->stop();
        websocketClient.reset();
        LOG_INFO("WebSocket 客户端已停止");
    }
}

/**
 * @brief 检查 WebSocket 连接状态
 * @return true 如果已连接，否则返回 false
 */
bool ControlCenter::isWebSocketConnected() const {
    return websocketClient && websocketClient->isConnected();
}

/**
 * @brief 处理 WebSocket 命令
 * @param cmd 命令字符串
 * @param params 参数列表
 * @return 执行结果字符串
 */
std::string ControlCenter::handleWebSocketCommand(const std::string& cmd, const std::vector<std::string>& params) {
    try {
        // 这里您需要根据实际的设备控制需求来实现具体的命令处理逻辑
        if (cmd == WS_TRACK_POSITION_CMD) {
            if (params.size() != 4) {
                return "履带位置控制参数数量错误，需要4个参数";
            }
            // 示例：控制履带电机位置
            // deviceManager.sendCommand("track_motor", POSITION_CMD, data);
            return "履带位置控制执行完成";
        }
        else if (cmd == WS_TRACK_SPEED_CMD) {
            if (params.size() != 3) {
                return "履带速度控制参数数量错误，需要3个参数";
            }
            // 示例：控制履带电机速度
            return "履带速度控制执行完成";
        }
        else if (cmd == WS_BRUSH_LIFTING_CMD) {
            if (params.size() != 2) {
                return "毛刷升降控制参数数量错误，需要2个参数";
            }
            // 示例：控制毛刷升降
            return "毛刷升降控制执行完成";
        }
        else if (cmd == WS_EIGHT_SWITCH_CMD) {
            if (params.size() != 1) {
                return "8路继电器控制参数数量错误，需要1个参数";
            }
            // 示例：控制8路继电器
            return "8路继电器控制执行完成";
        }
        else if (cmd == WS_PUSHFORWARD_CMD) {
            // 示例：PWM推进电机控制
            return "PWM推进电机控制执行完成";
        }
        else if (cmd == WS_VIDEO_STREAM_CMD) {
            if (params.size() != 1) {
                return "摄像头控制参数数量错误，需要1个参数";
            }
            // 示例：摄像头控制
            return "摄像头控制执行完成";
        }
        else {
            return "未知命令: " + cmd;
        }
    } catch (const std::exception& e) {
        return "命令执行异常: " + std::string(e.what());
    }
}
