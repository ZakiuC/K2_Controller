#pragma once
#include "device_protocol.h"

// RS232 设备实现
class RS232Device : public Device {
public:
    RS232Device(const std::string& id);
    
    bool connect() override;
    bool disconnect() override;
    bool sendCommand(uint8_t command, const uint8_t *data = nullptr, uint8_t response_cmd = 0, uint32_t timeout_ms = 50) override;
    void setInterface(Interface& interface) override {};
    
private:
    std::unique_ptr<DeviceHeartbeat> heartbeat;
    Interface* deviceInterface = nullptr;
};
