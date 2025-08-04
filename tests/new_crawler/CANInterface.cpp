#include "CANInterface.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/can/raw.h>
#include <cstdlib> // For system calls
#include <sys/select.h>
#include <fstream>
#include <sys/utsname.h>

CANInterface::CANInterface(const std::string &can_interface, bool use_canfd)
    : can_interface_(can_interface), sock_(-1), use_canfd_(use_canfd) {}

bool CANInterface::init()
{
    // 打印平台信息
    std::cout << "杰克平台检测: " << (is_JK_platform() ? "Yes" : "No") << std::endl;    
    // 设置CAN接口
    if (!setup_can_interface()) {
        std::cerr << "Failed to setup CAN interface" << std::endl;
        return false;
    }

    // 创建 CAN 套接字
    if ((sock_ = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
    {
        perror("创建 CAN 套接字失败");
        return false;
    }

    // 如果使用 CAN FD，设置相关选项
    if (use_canfd_)
    {
        int canfd_on = 1;
        if (setsockopt(sock_, SOL_CAN_RAW, CAN_RAW_FD_FRAMES,
                       &canfd_on, sizeof(canfd_on)) < 0)
        {
            perror("CAN FD enable failed");
            close(sock_);
            return false;
        }
        std::cout << "CAN FD mode enabled" << std::endl;
    }
    else
    {
        // 确保禁用 CAN FD 模式，只接收标准 CAN 帧
        int canfd_off = 0;
        if (setsockopt(sock_, SOL_CAN_RAW, CAN_RAW_FD_FRAMES,
                       &canfd_off, sizeof(canfd_off)) < 0)
        {
            perror("CAN FD disable failed");
            close(sock_);
            return false;
        }
        std::cout << "Standard CAN mode enabled" << std::endl;
    }

    // 获取接口索引
    struct ifreq ifr;
    std::strcpy(ifr.ifr_name, can_interface_.c_str());
    if (ioctl(sock_, SIOCGIFINDEX, &ifr) < 0)
    {
        perror("I/O 控制失败");
        close(sock_);
        return false;
    }

    // 绑定套接字到 CAN 接口
    struct sockaddr_can addr;
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(sock_, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("CAN 接口绑定失败");
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
        perror("CAN 帧发送失败");
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
        perror("CAN 帧接收失败");
        return false;
    }
    return true;
}


// 检测是否为杰克的板子
bool CANInterface::is_JK_platform()
{
    std::ifstream file("/proc/device-tree/model");
    if (file.is_open()) {
        std::string model;
        std::getline(file, model);
        file.close();
        return model.find("Rockchip") != std::string::npos;
    }
    
    // 备用检测方法 - 检查CPU信息
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.find("Rockchip") != std::string::npos || 
                line.find("RK35") != std::string::npos ||
                line.find("RK33") != std::string::npos) {
                cpuinfo.close();
                return true;
            }
        }
        cpuinfo.close();
    }
    
    return false;
}

// 获取平台信息
std::string CANInterface::get_platform_info()
{
    struct utsname sys_info;
    if (uname(&sys_info) == 0) {
        return std::string(sys_info.sysname) + " " + 
               std::string(sys_info.release) + " " + 
               std::string(sys_info.machine);
    }
    return "Unknown";
}

// 设置CAN接口
bool CANInterface::setup_can_interface()
{
    // 先关闭接口
    std::string down_cmd = "sudo ip link set " + can_interface_ + " down";
    system(down_cmd.c_str());
    
    std::string up_cmd;
    
    if (is_JK_platform()) {
        // 使用CAN FD配置
        up_cmd = "sudo ip link set " + can_interface_ + 
                " up type can bitrate 1000000 dbitrate 3000000 fd on";
    } else {
        // 使用标准CAN配置
        up_cmd = "sudo ip link set " + can_interface_ + 
                " up type can bitrate 1000000";
    }
    
    std::cout << "执行: " << up_cmd << std::endl;
    int result = system(up_cmd.c_str());
    
    if (result != 0) {
        std::cerr << "CAN 配置失败: " << result << std::endl;
        return false;
    }
    
    return true;
}
