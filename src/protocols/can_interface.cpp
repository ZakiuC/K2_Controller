#include "can_interface.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/can/raw.h>
#include <cstdlib>
#include <sys/select.h>
#include "logger.h"

CANInterface::CANInterface(const std::string &can_interface)
    : can_interface_(can_interface), sock_(-1) {}

bool CANInterface::init()
{
    system(("sudo ip link set " + can_interface_ + " down").c_str());
    system(("sudo ip link set " + can_interface_ + " up type can bitrate 1000000 dbitrate 3000000 fd on").c_str());
    
    // 创建 CAN 套接字
    if ((sock_ = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
    {
        LOG_ERROR("CAN 套接字创建失败: " + std::string(strerror(errno)));
        return false;
    }

    // 获取接口索引
    struct ifreq ifr;
    std::strcpy(ifr.ifr_name, can_interface_.c_str());
    if (ioctl(sock_, SIOCGIFINDEX, &ifr) < 0)
    {
        LOG_ERROR("CAN I/O 控制失败: " + std::string(strerror(errno)));
        close(sock_);
        return false;
    }

    // 绑定套接字到 CAN 接口
    struct sockaddr_can addr;
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(sock_, (struct sockaddr *)&addr, sizeof(addr)))
    {
        LOG_ERROR("绑定套接字到 CAN 接口失败: " + std::string(strerror(errno)));
        close(sock_);
        return false;
    }

    return true;
}

CANInterface::~CANInterface()
{
    if (sock_ != -1)
        close(sock_);
}

bool CANInterface::send_frame(const struct can_frame &frame)
{
    if (write(sock_, &frame, sizeof(frame)) != sizeof(frame))
    {
        LOG_ERROR("CAN 帧发送失败");
        return false;
    }
    return true;
}

bool CANInterface::receive_frame(struct can_frame &frame, int timeout_ms)
{
    fd_set set;
    struct timeval timeout;
    FD_ZERO(&set);
    FD_SET(sock_, &set);

    timeout.tv_sec = 0;
    timeout.tv_usec = timeout_ms * 1000;

    if (select(sock_ + 1, &set, NULL, NULL, &timeout) <= 0)
    {
        return false; // Timeout or error
    }

    if (read(sock_, &frame, sizeof(frame)) < 0)
    {
        LOG_ERROR("CAN 帧接收失败: " + std::string(strerror(errno)));
        return false;
    }
    return true;
}
