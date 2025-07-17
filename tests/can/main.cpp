#include <iostream>
#include <unistd.h>
#include "CANInterface.h"
#include "MotorController.h"

int main()
{
    const std::string CAN_INTERFACE = "can1";
    const int MOTOR_ID = 3; // 电机ID (1-32)

    // 初始化 CAN 接口
    CANInterface can_interface(CAN_INTERFACE);
    if (!can_interface.init())
    {
        std::cerr << "CAN interface initialization failed" << std::endl;
        return 1;
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
