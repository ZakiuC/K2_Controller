/**
 * @file control_center.cpp
 * @brief 控制中心实现文件
 * @details 提供设备控制的统一接口，支持多种控制模式（终端、WebSocket、MQTT）
 *          通过注册命令处理器实现灵活的命令分发
 * @author zakiu
 * @date 2025-07-15
 */
#include "control_center.h"
#include "logger.h"

/**
 * @brief 构造函数，初始化控制中心
 * @param dm 设备管理器的引用
 */
ControlCenter::ControlCenter(DeviceManager& dm) : deviceManager(dm), currentMode(ControlMode::TERMINAL) {}

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
