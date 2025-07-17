#pragma once
#include "device_manager.h"
#include <functional>
#include <mutex>
#include <queue>
#include <map>

enum class ControlMode {
    TERMINAL,
    WEBSOCKET,
    MQTT
};

class ControlCenter {
public:
    using CommandHandler = std::function<void(const std::string&, uint8_t command)>;

    ControlCenter(DeviceManager& dm);

    void setControlMode(ControlMode mode);
    ControlMode getControlMode() const;
    void registerCommandHandler(ControlMode mode, CommandHandler handler);
    void sendCommand(const std::string& deviceId, uint8_t command);
    void processIncomingCommand(ControlMode source, const std::string& deviceId, uint8_t command);

private:
    std::string modeToString(ControlMode mode) const;

    DeviceManager& deviceManager;
    ControlMode currentMode;
    mutable std::mutex modeMutex;
    std::map<ControlMode, CommandHandler> commandHandlers;
    std::mutex handlerMutex;
};
