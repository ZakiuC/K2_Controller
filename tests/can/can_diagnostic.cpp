#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

void test_can_interface(const std::string& interface_name, bool use_canfd) {
    std::cout << "\n=== 测试 " << interface_name << " (" 
              << (use_canfd ? "CAN FD" : "标准CAN") << " 模式) ===" << std::endl;
    
    // 创建套接字
    int sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) {
        perror("套接字创建失败");
        return;
    }
    
    // 设置CAN FD模式
    int canfd_mode = use_canfd ? 1 : 0;
    if (setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FD_FRAMES,
                   &canfd_mode, sizeof(canfd_mode)) < 0) {
        perror("CAN FD模式设置失败");
        close(sock);
        return;
    }
    
    // 获取接口信息
    struct ifreq ifr;
    strcpy(ifr.ifr_name, interface_name.c_str());
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("获取接口索引失败");
        close(sock);
        return;
    }
    
    std::cout << "接口索引: " << ifr.ifr_ifindex << std::endl;
    
    // 获取接口状态
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
        perror("获取接口状态失败");
        close(sock);
        return;
    }
    
    std::cout << "接口状态: ";
    if (ifr.ifr_flags & IFF_UP) std::cout << "UP ";
    if (ifr.ifr_flags & IFF_RUNNING) std::cout << "RUNNING ";
    if (ifr.ifr_flags & IFF_LOOPBACK) std::cout << "LOOPBACK ";
    std::cout << std::endl;
    
    // 绑定套接字
    struct sockaddr_can addr;
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("绑定失败");
        close(sock);
        return;
    }
    
    std::cout << "套接字绑定成功" << std::endl;
    std::cout << "CAN模式: " << (use_canfd ? "CAN FD" : "标准CAN") << std::endl;
    
    close(sock);
}

int main() {
    std::cout << "CAN接口诊断工具" << std::endl;
    std::cout << "=================" << std::endl;
    
    // 测试标准CAN模式
    test_can_interface("can1", false);
    
    // 测试CAN FD模式
    test_can_interface("can1", true);
    
    std::cout << "\n建议:" << std::endl;
    std::cout << "1. 如果你的硬件只支持CAN FD，请使用CAN FD模式" << std::endl;
    std::cout << "2. 确保can1接口已经启动: sudo ip link set can1 up" << std::endl;
    std::cout << "3. 如果使用虚拟CAN，创建方法: sudo modprobe vcan && sudo ip link add dev can1 type vcan" << std::endl;
    
    return 0;
}
