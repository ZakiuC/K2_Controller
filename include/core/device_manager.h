#pragma once
#include <unordered_map>
#include <mutex>
#include <vector>
#include "can_device.h"
#include "rs232_device.h"

class DeviceManager {
public:
    DeviceManager();
    bool addDevice(const std::string& protocol, const std::string& id, Interface& interface);
    bool removeDevice(const std::string& id);
    bool connectDevice(const std::string& id);
    bool disconnectDevice(const std::string& id);
    bool sendCommand(const std::string& id, uint8_t command);
    std::vector<std::string> listDevices() const;
    DeviceStatus getDeviceStatus(const std::string& id) const;

private:
    void handleDeviceStatusChange(const std::string& id, DeviceStatus status);

    std::unordered_map<std::string, std::unique_ptr<Device>> devices;
    mutable std::mutex devicesMutex;
};
