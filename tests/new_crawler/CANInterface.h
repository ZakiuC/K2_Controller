#ifndef CAN_INTERFACE_H
#define CAN_INTERFACE_H

#include <string>
#include <linux/can.h>
#include <linux/can/raw.h>

class CANInterface
{
public:
    CANInterface(const std::string &can_interface, bool use_canfd = false);
    bool init();
    bool send_frame(const struct can_frame &frame);
    bool receive_frame(struct can_frame &frame, int timeout_ms = 250);
    ~CANInterface();

private:
    std::string can_interface_;
    int sock_;
    bool use_canfd_;
    
    // 新增的私有方法
    bool is_JK_platform();
    std::string get_platform_info();
    bool setup_can_interface();
};

#endif // CAN_INTERFACE_H
