#include "LH08Controller.h"
#include "RS232Interface.h"

// 计算校验和
uint8_t LH08Controller::RS0x24_sum8(std::vector<uint8_t> &data, size_t length) {
    uint8_t sum = 0;
    for(int i = 0; i < length; i++) {
        sum += data[i];
    }
    return sum;
}

// 生成消息并发送
void LH08Controller::createMsg(void) {
    if(message[3] != 0x00) {   // 如果是设置状态
        for(int i = 0; i < 8; i++) {
            message[i+4] = status[i];  // 设置继电器状态
        }
    }
    message[12] = RS0x24_sum8(message, 12);  // 计算并添加校验和
    write(fd, message.data(), message.size());  // 发送消息
    ssize_t ret = read(fd, recvMsg.data(), recvMsg.size());  // 读取响应
    if(ret < 0) {
        std::cout << "错误: 读取的字节数小于0。" << std::endl;
        return;
    }
    uint8_t check_byte = RS0x24_sum8(recvMsg, 12);  // 校验响应的校验和
    if(check_byte != recvMsg[12]) {
        std::cout << "校验失败!" << std::endl;
    }
}

// 设置8个继电器的状态
void LH08Controller::setStatus08(std::string str) {
    short pos = std::stoi(str, nullptr, 16);    // 将十六进制字符串转换为整数
    for(int i = 0; i < 8; i++) {
        if(pos & (0x01 << i)) {  // 如果位为1，则开启继电器
            message[3+i+1] = 0x02;
        } else {
            message[3+i+1] = 0x01;  // 如果位为0，则关闭继电器
        }
    }
    message[3] = 0x01;
    message[12] = RS0x24_sum8(message, 12);  // 计算校验和
    write(fd, message.data(), message.size());  // 发送消息
    usleep(30000);  // 延时30ms
    ssize_t ret = read(fd, recvMsg.data(), recvMsg.size());  // 读取响应
    if(ret < 0) {
        std::cout << "错误: 读取的字节数小于0。" << std::endl;
        return;
    }
    uint8_t check_byte = RS0x24_sum8(recvMsg, 12);  // 校验响应的校验和
    if(check_byte != recvMsg[12]) {
        std::cout << "校验失败!" << std::endl;
    }
}
