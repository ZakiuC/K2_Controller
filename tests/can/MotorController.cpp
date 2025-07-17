#include "MotorController.h"
#include <iostream>
#include <thread>
#include <chrono>

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
    // 清空接收缓冲区中可能的残留数据
    struct can_frame temp_frame;
    while (can_interface_.receive_frame(temp_frame, 10)) // 10ms超时，快速清空
    {
        std::cout << "清空缓冲区: ID=0x" << std::hex << temp_frame.can_id 
                  << ", 数据=[0x" << (int)temp_frame.data[0] << "]" << std::dec << std::endl;
    }

    struct can_frame frame;
    frame.can_id = 0x140 + motor_id_; // 标准帧 ID
    frame.can_dlc = 8;                // 数据长度固定8字节

    frame.data[0] = command;
    for (int i = 1; i < 8; ++i)
    {
        frame.data[i] = data ? data[i] : 0x00; // 填充数据或默认0
    }
    
    // 打印发送的CAN帧数据
    std::cout << "发送CAN帧: ID=0x" << std::hex << frame.can_id 
              << ", DLC=" << std::dec << (int)frame.can_dlc 
              << ", 数据=[";
    for (int i = 0; i < frame.can_dlc; i++)
    {
        std::cout << "0x" << std::hex << (int)frame.data[i];
        if (i < frame.can_dlc - 1) std::cout << " ";
    }
    std::cout << "]" << std::dec << std::endl;
    
    if(!can_interface_.send_frame(frame))
    {
        return false; // 发送失败
    }

    // 记录当前时间
    auto start_time = std::chrono::steady_clock::now();
    // 循环检查是否收到响应
    while (true)
    {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
        
        // 超时检查
        if (elapsed_time >= timeout_ms)
        {
            std::cerr << "等待响应超时: 命令=0x" 
                      << std::hex << static_cast<int>(command)
                      << ", 耗时=" << std::dec << elapsed_time << "ms" << std::endl;
            return false;
        }

        if (can_interface_.receive_frame(frame, 50)) // 每50ms检查一次
        {
            // 计算响应时间（从发送到接收的时间间隔）
            auto response_time = std::chrono::steady_clock::now();
            auto response_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(response_time - start_time).count();
            
            // 打印接收的CAN帧数据
            std::cout << "接收CAN帧: ID=0x" << std::hex << frame.can_id 
                      << ", DLC=" << std::dec << (int)frame.can_dlc 
                      << ", 数据=[";
            for (int i = 0; i < frame.can_dlc; i++)
            {
                std::cout << "0x" << std::hex << (int)frame.data[i];
                if (i < frame.can_dlc - 1) std::cout << " ";
            }
            std::cout << "], 响应时间=" << std::dec << response_elapsed << "ms" << std::endl;

            // 检查返回的帧是否正确 - 更严格的验证
            if (frame.can_id == (0x140 + motor_id_))
            {
                // 对于不同命令，采用不同的验证策略
                if (command == 0x88 || command == 0x80 || command == 0x81) 
                {
                    // 对于使能/禁用/停止命令，检查命令字节是否匹配
                    if (frame.data[0] == command)
                    {
                        std::cout << "命令响应匹配，执行成功" << std::endl;
                        return true;
                    }
                }
                else if (command == 0xA2) 
                {
                    // 对于速度控制命令，响应可能不是原命令，而是状态反馈
                    // 检查是否收到了任何合理的响应
                    std::cout << "速度命令已发送，收到响应" << std::endl;
                    return true;
                }
                else
                {
                    // 其他命令的默认处理
                    if (frame.data[0] == command)
                    {
                        return true;
                    }
                }
                
                // 如果ID匹配但命令不匹配，可能是状态反馈，继续等待
                std::cout << "收到同ID帧但命令不匹配，继续等待..." << std::endl;
            }
            else
            {
                // ID不匹配，可能是其他设备的数据，忽略
                std::cout << "收到其他设备数据，忽略" << std::endl;
            }
        }
    }
}
