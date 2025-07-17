/**
 * @file device_manager.cpp
 * @brief 设备管理器实现文件
 * @details 提供设备的统一管理功能，包括设备的添加、删除、连接控制和命令发送
 *          支持多种设备协议（CAN、RS232），使用工厂模式创建设备实例
 * @author zakiu
 * @date 2025-07-15
 */

#include "device_manager.h"
#include <regex>

/**
 * @brief DeviceManager构造函数
 * @details 初始化设备管理器，注册支持的设备协议
 *          - 注册CAN设备创建函数
 *          - 注册RS232设备创建函数
 *          使用工厂模式实现设备的统一创建
 */
DeviceManager::DeviceManager() {
    DeviceFactory::getInstance().registerProtocol("CAN", 
        [](const std::string& id) -> std::unique_ptr<Device> {
            return std::make_unique<CANDevice>(id);
        });
    
    DeviceFactory::getInstance().registerProtocol("RS232", 
        [](const std::string& id) -> std::unique_ptr<Device> {
            return std::make_unique<RS232Device>(id);
        });
}

/**
 * @brief 添加新设备到管理器
 * @details 线程安全地添加设备实例到设备管理器
 *          - 检查设备ID是否已存在
 *          - 使用工厂模式创建对应协议的设备
 *          - 设置设备状态变化回调函数
 *          - 将设备添加到管理容器中
 * @param protocol 设备协议类型（如"CAN"、"RS232"）
 * @param id 设备唯一标识符
 * @param interface 设备接口引用
 * @return bool 添加成功返回true，失败返回false
 */
bool DeviceManager::addDevice(const std::string& protocol, const std::string& id, Interface& interface) {
    // 检查 id 是否符合 "device_<number>" 格式
    std::regex idPattern("^device_\\d+$");
    if (!std::regex_match(id, idPattern)) {
        LOG_WARNING("设备id [" + id + "] 格式无效，请使用: device_<number>");
        return false;
    }

    std::lock_guard<std::mutex> lock(devicesMutex);
    if (devices.find(id) != devices.end()) {
        LOG_WARNING("设备id [" + id + "] 已存在");
        return false;
    }

    auto device = DeviceFactory::getInstance().createDevice(protocol, id);
    if (!device) {
        LOG_ERROR("创建设备失败: [" + id + "]");
        return false;
    }

    device->setStatusCallback([this](const std::string& id, DeviceStatus status) {
        handleDeviceStatusChange(id, status);
    });
    devices[id] = std::move(device);
    devices[id]->setInterface(interface); // 设置设备接口
    LOG_INFO("已添加设备: [" + id + "] (" + protocol + ")");
    return true;
}

/**
 * @brief 从管理器中移除设备
 * @details 线程安全地移除指定ID的设备
 *          - 查找设备是否存在
 *          - 断开设备连接
 *          - 从管理容器中删除设备
 * @param id 要移除的设备唯一标识符
 * @return bool 移除成功返回true，设备不存在返回false
 */
bool DeviceManager::removeDevice(const std::string& id) {
    std::lock_guard<std::mutex> lock(devicesMutex);
    auto it = devices.find(id);
    if (it == devices.end()) {
        LOG_WARNING("设备未找到: [" + id + "]");
        return false;
    }
    it->second->disconnect();
    devices.erase(it);
    LOG_INFO("已移除设备: [" + id + "]");
    return true;
}

/**
 * @brief 连接指定设备
 * @details 线程安全地建立与指定设备的连接
 *          - 验证设备是否存在
 *          - 调用设备的连接方法
 * @param id 要连接的设备唯一标识符
 * @return bool 连接成功返回true，设备不存在或连接失败返回false
 */
bool DeviceManager::connectDevice(const std::string& id) {
    std::lock_guard<std::mutex> lock(devicesMutex);
    auto it = devices.find(id);
    if (it == devices.end()) {
        LOG_WARNING("设备未找到: [" + id + "]");
        return false;
    }
    return it->second->connect();
}

/**
 * @brief 断开指定设备连接
 * @details 线程安全地断开与指定设备的连接
 *          - 验证设备是否存在
 *          - 调用设备的断开连接方法
 * @param id 要断开连接的设备唯一标识符
 * @return bool 断开成功返回true，设备不存在或断开失败返回false
 */
bool DeviceManager::disconnectDevice(const std::string& id) {
    std::lock_guard<std::mutex> lock(devicesMutex);
    auto it = devices.find(id);
    if (it == devices.end()) {
        LOG_WARNING("设备未找到: [" + id + "]");
        return false;
    }
    return it->second->disconnect();
}

/**
 * @brief 向指定设备发送命令
 * @details 线程安全地向指定设备发送二进制命令数据
 *          - 验证设备是否存在
 *          - 调用设备的命令发送方法
 * @param id 目标设备的唯一标识符
 * @param command 要发送的二进制命令数据
 * @return bool 发送成功返回true，设备不存在或发送失败返回false
 */
bool DeviceManager::sendCommand(const std::string& id, uint8_t command) {
    std::lock_guard<std::mutex> lock(devicesMutex);
    auto it = devices.find(id);
    if (it == devices.end()) {
        LOG_WARNING("设备未找到: [" + id + "]");
        return false;
    }
    return it->second->sendCommand(command);
}

/**
 * @brief 获取所有已管理设备的ID列表
 * @details 线程安全地返回当前管理器中所有设备的ID
 *          返回的是设备ID的副本，不会受到后续设备变更的影响
 * @return std::vector<std::string> 包含所有设备ID的字符串向量
 */
std::vector<std::string> DeviceManager::listDevices() const {
    std::lock_guard<std::mutex> lock(devicesMutex);
    std::vector<std::string> ids;
    for (const auto& pair : devices) {
        ids.push_back(pair.first);
    }
    return ids;
}

/**
 * @brief 获取指定设备的当前状态
 * @details 线程安全地查询指定设备的连接状态
 *          如果设备不存在，返回DISCONNECTED状态
 * @param id 要查询状态的设备唯一标识符
 * @return DeviceStatus 设备状态枚举值
 *         - DISCONNECTED: 设备未连接或不存在
 *         - CONNECTED: 设备已连接
 *         - ACTIVE: 设备处于活动状态
 *         - ERROR: 设备出现错误
 */
DeviceStatus DeviceManager::getDeviceStatus(const std::string& id) const {
    std::lock_guard<std::mutex> lock(devicesMutex);
    auto it = devices.find(id);
    if (it == devices.end()) {
        return DeviceStatus::DISCONNECTED;
    }
    return it->second->getStatus();
}

/**
 * @brief 处理设备状态变化
 * @details 当设备状态发生变化时调用此方法
 *          - 记录状态变化日志
 *          - 可以扩展为触发其他事件或通知机制
 * @param id 设备唯一标识符
 * @param status 新的设备状态
 * @note 此方法由设备实例在状态变化时调用
 *       - 例如连接成功、断开连接、发生错误等
 */
void DeviceManager::handleDeviceStatusChange(const std::string& id, DeviceStatus status) {
    LOG_INFO("设备状态更新: [" + id + "] -> " + 
             (status == DeviceStatus::CONNECTED ? "已连接" : "未连接"));
}
