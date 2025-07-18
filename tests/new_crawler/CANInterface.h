#ifndef CAN_INTERFACE_H
#define CAN_INTERFACE_H

#include <string>
#include <linux/can.h>

class CANInterface
{
public:
    CANInterface(const std::string &can_interface);
    bool init();
    bool send_frame(const struct can_frame &frame);
    bool receive_frame(struct can_frame &frame, int timeout_ms = 250);
    ~CANInterface();

private:
    std::string can_interface_;
    int sock_;
};

#endif // CAN_INTERFACE_H
