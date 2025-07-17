#include <iostream>
#include <unistd.h>
#include "CANInterface.h"
#include "MotorController.h"

int main()
{
    const std::string CAN_INTERFACE = "can1";
    const int MOTOR_ID = 3; // 电机ID (1-32)

    std::cout << "选择CAN模式:" << std::endl;
    std::cout << "1. 标准CAN模式" << std::endl;
    std::cout << "2. CAN FD模式" << std::endl;
    std::cout << "请输入选择 (1 或 2): ";
    
    int choice;
    std::cin >> choice;
    
    bool use_canfd = (choice == 2);
    
    // 初始化 CAN 接口
    CANInterface can_interface(CAN_INTERFACE, use_canfd);
    if (!can_interface.init())
    {
        std::cerr << "CAN interface initialization failed" << std::endl;
        return 1;
    }

    std::cout << "选择运行模式:" << std::endl;
    std::cout << "1. 电机控制模式" << std::endl;
    std::cout << "2. CAN数据监听模式" << std::endl;
    std::cout << "请输入选择 (1 或 2): ";
    
    int run_mode;
    std::cin >> run_mode;
    
    if (run_mode == 2) {
        // CAN数据监听模式
        std::cout << "开始监听CAN数据，按Ctrl+C退出..." << std::endl;
        struct can_frame frame;
        while (true) {
            if (can_interface.receive_frame(frame, 1000)) {
                std::cout << "接收到CAN帧: ID=0x" << std::hex << frame.can_id 
                          << ", DLC=" << std::dec << (int)frame.can_dlc 
                          << ", 数据=[";
                for (int i = 0; i < frame.can_dlc; i++) {
                    std::cout << "0x" << std::hex << (int)frame.data[i];
                    if (i < frame.can_dlc - 1) std::cout << " ";
                }
                std::cout << "]" << std::dec << std::endl;
            }
        }
        return 0;
    }

    // 初始化电机控制
    MotorController motor(can_interface, MOTOR_ID);

    // 控制演示
    // if (motor.enable_motor())
    // {
        sleep(1);

        // 设置速度 100 dps (度/秒)
        if (motor.set_speed(100))
        {
            sleep(3);
        }

        // motor.stop_motor();
        // sleep(1);

        // motor.disable_motor();
    // }

    return 0;
}
