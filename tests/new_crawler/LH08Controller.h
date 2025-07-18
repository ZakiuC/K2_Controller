#pragma once

#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <vector>
#include <iostream>
#include <string>
#include "RS232Interface.h"

#define BAUDRATE B57600  // 设置波特率
#define RS232_PORT "/dev/ttyS6"  // 串口路径

// 8路继电器控制类
class LH08Controller {
public:
    LH08Controller(): message(13), fd(-1) {
        message[0] = 0x24;
        message[1] = 0x01;
        message[2] = 0x0A;
    }

    // 打开串口
    bool openRS232Port(const char* port = RS232_PORT) {
        fd = setup_serial_port(port, BAUDRATE);
        if(fd >= 0) {
            std::cout << "串口已打开: " << fd << " (设备: " << port << ")" << std::endl;
            return true;
        } else {
            std::cout << "串口打开失败: " << port << std::endl;
            return false;
        }
    }

    // 使用已有的文件描述符
    void openRS232Port(int fd) {
        this->fd = fd;
        std::cout << "串口已打开: " << this->fd << std::endl;
    }

    // 关闭串口
    void closeRS232Port(void) {
        if(fd >= 0) {
            close(fd);
            fd = -1;
        }
    }

    // 设置某个继电器的状态，1~8表示继电器的位置，status表示继电器状态（0x01为关闭，0x02为开启）
    void setStatus(uint8_t pos, uint8_t status) {    
        if(pos < 1 || pos > 8) {
            std::cout << "无效的位置，位置范围: 1~8。" << std::endl;
            return;
        }
        this->status[pos-1] = status;
        message[3] = 0x01;
        createMsg();  // 生成消息并发送
    }

    // 获取所有继电器的状态
    void getStatus(void) {
        message[3] = 0x00;  // 查询状态
        createMsg();  // 生成消息并发送
    }

    // 通过传入的字符串设置8个继电器的状态
    void setStatus08(std::string str);

private:
    uint8_t RS0x24_sum8(std::vector<uint8_t> &data, size_t length);  // 校验和函数
    void createMsg(void);   // 组装消息
    int fd;  // 串口文件描述符
    std::vector<uint8_t> status = std::vector<uint8_t>(8);  // 存储8路继电器的状态
    std::vector<uint8_t> message;  // 命令消息
    std::vector<uint8_t> recvMsg = std::vector<uint8_t>(13);  // 接收到的消息
};


