/**
 * @file device_protocol.h
 * @brief 设备协议头文件
 * @details 定义设备协议的基类和相关接口
 *          包括设备状态、连接、断开连接和命令发送等功能
 * @author zakiu
 * @date 2025-07-15
 */
#pragma once
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <atomic>
#include <chrono>
#include <thread>
#include "logger.h"
#include "device_interface.h" // 提供设备接口类
#include "device_util.h"    // 提供设备ID字符串处理函数

// 设备状态
enum class DeviceStatus {
    DISCONNECTED,
    CONNECTED,
    ACTIVE,
    // OFFLINE,
    ERROR
};

/**
 * @brief 设备基类
 * @details 定义设备的基本接口和属性
 *          - 支持连接、断开连接和发送命令
 *          - 支持状态更新和回调机制
 * @note 该类是所有设备协议的基类，具体设备需继承此类实现具体功能
 */
class Device {
public:
    Device(const std::string& id, const std::string& type) 
        : id(id), type(type), status(DeviceStatus::DISCONNECTED) {}
    
    virtual ~Device() {}
    
    virtual bool connect() = 0;
    virtual bool disconnect() = 0;
    virtual bool sendCommand(uint8_t command, const uint8_t *data = nullptr, uint8_t response_cmd = 0, uint32_t timeout_ms = 50) = 0;
    virtual DeviceStatus getStatus() const { return status; }
    virtual bool checkDeviceAlive() {
        // 实现具体设备的心跳检测
        return true; 
    }
    virtual void setInterface(Interface& interface) = 0;

    std::string getId() const { return id; }
    std::string getType() const { return type; }
    
    void setStatusCallback(std::function<void(const std::string&, DeviceStatus)> callback) {
        statusCallback = callback;
    }
    
public:
    void updateStatus(DeviceStatus newStatus) {
        if (status != newStatus) {
            status = newStatus;
            if (statusCallback) {
                statusCallback(id, newStatus);
            }
            LOG_INFO("Device " + id + " status changed to " + statusToString(newStatus));
        }
    }
    

protected:
    
    std::string statusToString(DeviceStatus s) const {
        switch(s) {
            case DeviceStatus::DISCONNECTED: return "DISCONNECTED";
            case DeviceStatus::CONNECTED: return "CONNECTED";
            case DeviceStatus::ACTIVE: return "ACTIVE";
            case DeviceStatus::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }
    
    std::string id;
    std::string type;
    DeviceStatus status;
    
    std::function<void(const std::string&, DeviceStatus)> statusCallback;
};

/**
 * @brief 设备工厂类
 * @details 用于创建不同类型的设备实例
 *          - 支持注册不同协议的设备创建函数
 *          - 提供统一的接口创建设备实例
 * @note 该类是单例模式，保证全局只有一个设备工厂实例
 */
class DeviceFactory {
public:
    using CreateDeviceFunc = std::function<std::unique_ptr<Device>(const std::string&)>;
    
    static DeviceFactory& getInstance() {
        static DeviceFactory instance;
        return instance;
    }
    
    void registerProtocol(const std::string& protocol, CreateDeviceFunc createFunc) {
        creators[protocol] = createFunc;
    }
    
    std::unique_ptr<Device> createDevice(const std::string& protocol, const std::string& id) {
        auto it = creators.find(protocol);
        if (it != creators.end()) {
            return it->second(id);
        }
        LOG_ERROR("Unknown protocol: " + protocol);
        return nullptr;
    }
    
private:
    DeviceFactory() = default;
    std::unordered_map<std::string, CreateDeviceFunc> creators;
};

/**
 * @brief 设备心跳检测器
 * @details 用于定期检查设备状态，确保设备连接的可靠性
 *         - 支持自定义心跳间隔
 *        - 使用多线程实现心跳检测
 * * @note 该类是设备的辅助类，通常与设备实例一起使用
 *         - 可以扩展为支持不同协议的心跳检测逻辑
 *         - 需要在设备连接时启动心跳检测器
 *         - 在设备断开连接时停止心跳检测器
 */
class DeviceHeartbeat {
public:
    DeviceHeartbeat(Device* device, int intervalMs = 5000) 
        : device(device), interval(intervalMs), running(false) {}
    
    ~DeviceHeartbeat() {
        stop();
    }
    
    void start() {
        if (running) return;
        
        running = true;
        heartbeatThread = std::thread([this]() {
            while (running) {
                // 实现心跳检测逻辑
                bool alive = device->checkDeviceAlive();
                
                if (alive) {
                    device->updateStatus(DeviceStatus::ACTIVE);
                } else {
                    device->updateStatus(DeviceStatus::ERROR);
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(interval));
            }
        });
    }
    
    void stop() {
        running = false;
        if (heartbeatThread.joinable()) {
            heartbeatThread.join();
        }
    }
    
    

    private:
    
    Device* device;
    int interval;
    std::atomic<bool> running;
    std::thread heartbeatThread;
};