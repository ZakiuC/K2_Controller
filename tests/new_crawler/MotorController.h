#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include "CANInterface.h"
#include <cstdint>

class MotorController
{
public:
    MotorController(CANInterface &can_interface, int motor_id);
    bool enable_motor();
    bool disable_motor();
    bool stop_motor();
    bool set_speed(int32_t target_speed_dps);

private:
    CANInterface &can_interface_;
    int motor_id_;

    bool send_command(uint8_t command, const uint8_t *data = nullptr, uint32_t timeout_ms = 50);
};

#endif // MOTOR_CONTROLLER_H
