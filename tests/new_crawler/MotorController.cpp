#include "MotorController.h"
#include <iostream>
#include <thread>

MotorController::MotorController(CANInterface &can_interface, int motor_id)
    : can_interface_(can_interface), motor_id_(motor_id) {}

bool MotorController::enable_motor()
{
    return send_command(0x88); // 运行命令
}

bool MotorController::disable_motor()
{
    return send_command(0x80); // 关闭命令
}

bool MotorController::stop_motor()
{
    return send_command(0x81); // 停止命令
}

bool MotorController::set_speed(int32_t target_speed_dps)
{
    // 转换为协议单位 (0.01 dps/LSB)
    int32_t speed_control = target_speed_dps * 100;

    uint8_t data[8] = {0};
    data[4] = static_cast<uint8_t>(speed_control);
    data[5] = static_cast<uint8_t>(speed_control >> 8);
    data[6] = static_cast<uint8_t>(speed_control >> 16);
    data[7] = static_cast<uint8_t>(speed_control >> 24);

    return send_command(0xA2, data); // 速度闭环命令
}

bool MotorController::send_command(uint8_t command, const uint8_t *data, uint32_t timeout_ms)
{
    struct can_frame frame;
    frame.can_id = 0x140 + motor_id_; // 标准帧 ID
    frame.can_dlc = 8;                // 数据长度固定8字节

    frame.data[0] = command;
    for (int i = 1; i < 8; ++i)
    {
        frame.data[i] = data ? data[i] : 0x00; // 填充数据或默认0
    }
    
    if(!can_interface_.send_frame(frame))
    {
        return false; // 发送失败
    }

    return true; // 发送成功
}
