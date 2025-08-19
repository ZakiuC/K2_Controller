#include "modbus_rtu.h"
#include "modbus_config.h"
#include <iostream>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <iomanip>

ModbusRTU::ModbusRTU() : fd(-1) {}

ModbusRTU::~ModbusRTU() {
    close();
}

uint16_t ModbusRTU::calculateCRC16(const uint8_t* data, int length) {
    uint16_t crc = 0xFFFF;
    for (int pos = 0; pos < length; pos++) {
        crc ^= (uint16_t)data[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

bool ModbusRTU::sendData(const uint8_t* data, int length) {
    int bytes_written = write(fd, data, length);
    if (bytes_written != length) {
        std::cerr << "发送数据失败: " << strerror(errno) << std::endl;
        return false;
    }
    
    // 刷新缓冲区
    tcdrain(fd);
    return true;
}

int ModbusRTU::findModbusFrame(const uint8_t* buffer, int total_length, 
                              uint8_t expected_slave_id, uint8_t expected_function_code) {
    for (int i = 0; i <= total_length - 8; i++) {
        if (buffer[i] == expected_slave_id && buffer[i + 1] == expected_function_code) {
            return i; // 找到可能的Modbus帧起始位置
        }
    }
    return -1; // 未找到
}

int ModbusRTU::receiveData(uint8_t* buffer, int max_length, int timeout_ms) {
    int total_bytes = 0;
    int attempts = 0;
    const int max_attempts = 10;
    
    while (total_bytes < max_length && attempts < max_attempts) {
        fd_set readfds;
        struct timeval timeout;
        
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;
        
        int ret = select(fd + 1, &readfds, NULL, NULL, &timeout);
        if (ret <= 0) {
            if (total_bytes > 0) {
                // 已经接收到一些数据，可能接收完成
                break;
            }
            std::cerr << "接收数据超时或错误，尝试次数: " << attempts + 1 << std::endl;
            attempts++;
            continue;
        }
        
        int bytes_read = read(fd, buffer + total_bytes, max_length - total_bytes);
        if (bytes_read < 0) {
            std::cerr << "读取数据失败: " << strerror(errno) << std::endl;
            return -1;
        }
        
        if (bytes_read > 0) {
            total_bytes += bytes_read;
            std::cout << "接收到 " << bytes_read << " 字节，总计 " << total_bytes << " 字节" << std::endl;
            
            // 检查是否可能是完整的Modbus响应
            if (total_bytes >= 8) {
                // 查找可能的Modbus帧
                int frame_start = findModbusFrame(buffer, total_bytes, SLAVE_ID, 0x06);
                if (frame_start == -1) {
                    frame_start = findModbusFrame(buffer, total_bytes, SLAVE_ID, 0x03);
                }
                if (frame_start == -1) {
                    frame_start = findModbusFrame(buffer, total_bytes, SLAVE_ID, 0x10);
                }
                
                if (frame_start >= 0) {
                    // 找到Modbus帧，检查是否完整
                    int remaining_bytes = total_bytes - frame_start;
                    uint8_t function_code = buffer[frame_start + 1];
                    
                    if (function_code == 0x03 && remaining_bytes >= 5) {
                        // 读寄存器响应，检查长度
                        if (remaining_bytes >= (3 + buffer[frame_start + 2] + 2)) {
                            // 将有效的Modbus帧移到缓冲区开头
                            memmove(buffer, buffer + frame_start, remaining_bytes);
                            return remaining_bytes;
                        }
                    } else if ((function_code == 0x06 || function_code == 0x10) && remaining_bytes >= 8) {
                        // 写寄存器响应，固定8字节
                        memmove(buffer, buffer + frame_start, remaining_bytes);
                        return remaining_bytes;
                    }
                }
            }
            
            // 短暂等待看是否还有更多数据
            usleep(10000); // 10ms
        }
        
        attempts++;
    }
    
    return total_bytes;
}

void ModbusRTU::printHex(const char* prefix, const uint8_t* data, int length) {
    std::cout << prefix;
    for (int i = 0; i < length; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << (int)data[i] << " ";
    }
    std::cout << std::dec << std::endl;
}

bool ModbusRTU::init() {
    // 打开串口
    fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        std::cerr << "无法打开串口 " << SERIAL_PORT << ": " 
                  << strerror(errno) << std::endl;
        return false;
    }
    
    // 配置串口参数
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        std::cerr << "获取串口属性失败: " << strerror(errno) << std::endl;
        ::close(fd);
        fd = -1;
        return false;
    }
    
    // 设置波特率
    cfsetospeed(&tty, BAUD_RATE);
    cfsetispeed(&tty, BAUD_RATE);
    
    // 8N1 配置
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8位数据位
    tty.c_iflag &= ~IGNBRK;                         // 禁用break处理
    tty.c_lflag = 0;                                // 关闭本地echo
    tty.c_oflag = 0;                                // 关闭输出处理
    tty.c_cc[VMIN] = 0;                             // 非阻塞读取
    tty.c_cc[VTIME] = 5;                            // 0.5秒读取超时
    
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);         // 关闭软件流控
    tty.c_cflag |= (CLOCAL | CREAD);                // 忽略modem控制，启用读取
    tty.c_cflag &= ~(PARENB | PARODD);              // 无校验
    tty.c_cflag &= ~CSTOPB;                         // 1个停止位
    tty.c_cflag &= ~CRTSCTS;                        // 关闭硬件流控
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        std::cerr << "设置串口属性失败: " << strerror(errno) << std::endl;
        ::close(fd);
        fd = -1;
        return false;
    }
    
    // 清空串口缓冲区
    tcflush(fd, TCIOFLUSH);
    
    std::cout << "串口 " << SERIAL_PORT << " 初始化成功" << std::endl;
    return true;
}

void ModbusRTU::flushBuffer() {
    if (fd >= 0) {
        tcflush(fd, TCIOFLUSH);
        usleep(10000); // 10ms延时
    }
}

void ModbusRTU::close() {
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}

bool ModbusRTU::readHoldingRegisters(uint16_t start_addr, uint16_t num_regs, 
                                    std::vector<uint16_t>& values) {
    if (fd < 0) {
        std::cerr << "串口未初始化" << std::endl;
        return false;
    }
    
    // 验证读取数量
    if (num_regs > MAX_REGISTER_COUNT) {
        std::cerr << "读取寄存器数量超过最大值 " << MAX_REGISTER_COUNT << std::endl;
        return false;
    }
    
    // 构建Modbus RTU请求帧
    uint8_t request[8];
    request[0] = SLAVE_ID;                    // 从设备地址
    request[1] = 0x03;                        // 功能码：读保持寄存器
    request[2] = (start_addr >> 8) & 0xFF;    // 起始地址高字节
    request[3] = start_addr & 0xFF;           // 起始地址低字节
    request[4] = (num_regs >> 8) & 0xFF;      // 寄存器数量高字节
    request[5] = num_regs & 0xFF;             // 寄存器数量低字节
    
    // 计算CRC
    uint16_t crc = calculateCRC16(request, 6);
    request[6] = crc & 0xFF;                  // CRC低字节
    request[7] = (crc >> 8) & 0xFF;           // CRC高字节
    
    printHex("发送读取请求: ", request, 8);
    
    // 清空接收缓冲区
    flushBuffer();
    
    // 发送请求
    if (!sendData(request, 8)) {
        return false;
    }
    
    // 等待一段时间让设备处理
    usleep(50000); // 50ms延时
    
    // 接收响应
    uint8_t response[256];
    int response_len = receiveData(response, sizeof(response));
    if (response_len < 5) {
        std::cerr << "接收响应数据长度不足，实际长度: " << response_len << std::endl;
        return false;
    }
    
    printHex("接收响应: ", response, response_len);
    
    // 检查响应长度是否合理 (应该是 3 + num_regs * 2 + 2)
    int expected_len = 3 + num_regs * 2 + 2;
    if (response_len != expected_len) {
        std::cerr << "响应长度不匹配，期望: " << expected_len 
                  << "，实际: " << response_len << std::endl;
    }
    
    // 验证响应
    if (response[0] != SLAVE_ID) {
        std::cerr << "从设备地址不匹配" << std::endl;
        return false;
    }
    
    if (response[1] != 0x03) {
        if (response[1] & 0x80) {
            std::cerr << "Modbus异常响应，异常码: 0x" << std::hex 
                      << (int)response[2] << std::dec << std::endl;
        } else {
            std::cerr << "功能码不匹配" << std::endl;
        }
        return false;
    }
    
    uint8_t byte_count = response[2];
    if (byte_count != num_regs * 2) {
        std::cerr << "数据字节数不匹配" << std::endl;
        return false;
    }
    
    // 验证CRC
    uint16_t received_crc = (response[response_len - 1] << 8) | response[response_len - 2];
    uint16_t calculated_crc = calculateCRC16(response, response_len - 2);
    
    std::cout << "CRC调试信息:" << std::endl;
    std::cout << "  响应长度: " << response_len << std::endl;
    std::cout << "  接收CRC: 0x" << std::hex << received_crc << std::dec << std::endl;
    std::cout << "  计算CRC: 0x" << std::hex << calculated_crc << std::dec << std::endl;
    
    if (received_crc != calculated_crc) {
        std::cerr << "CRC校验失败，接收: 0x" << std::hex << received_crc 
                  << "，计算: 0x" << calculated_crc << std::dec << std::endl;
        return false;
    }
    
    // 提取寄存器值
    values.clear();
    for (int i = 0; i < num_regs; i++) {
        uint16_t value = (response[3 + i * 2] << 8) | response[4 + i * 2];
        values.push_back(value);
    }
    
    return true;
}

bool ModbusRTU::writeSingleRegister(uint16_t reg_addr, uint16_t value) {
    if (fd < 0) {
        std::cerr << "串口未初始化" << std::endl;
        return false;
    }
    
    // 构建Modbus RTU请求帧
    uint8_t request[8];
    request[0] = SLAVE_ID;                    // 从设备地址
    request[1] = 0x06;                        // 功能码：写单个寄存器
    request[2] = (reg_addr >> 8) & 0xFF;      // 寄存器地址高字节
    request[3] = reg_addr & 0xFF;             // 寄存器地址低字节
    request[4] = (value >> 8) & 0xFF;         // 寄存器值高字节
    request[5] = value & 0xFF;                // 寄存器值低字节
    
    // 计算CRC
    uint16_t crc = calculateCRC16(request, 6);
    request[6] = crc & 0xFF;                  // CRC低字节
    request[7] = (crc >> 8) & 0xFF;           // CRC高字节
    
    printHex("发送写入请求: ", request, 8);
    
    // 清空接收缓冲区
    flushBuffer();
    
    // 发送请求
    if (!sendData(request, 8)) {
        return false;
    }
    
    // 等待一段时间让设备处理
    usleep(50000); // 50ms延时
    
    // 接收响应
    uint8_t response[8];
    int response_len = receiveData(response, sizeof(response));
    if (response_len != 8) {
        std::cerr << "接收响应数据长度错误" << std::endl;
        return false;
    }
    
    printHex("接收响应: ", response, response_len);
    
    // 验证响应（写单个寄存器的响应应该与请求相同）
    if (memcmp(request, response, 8) != 0) {
        std::cerr << "写入响应验证失败" << std::endl;
        return false;
    }
    
    return true;
}

bool ModbusRTU::writeMultipleRegisters(uint16_t start_addr, const std::vector<uint16_t>& values) {
    if (fd < 0) {
        std::cerr << "串口未初始化" << std::endl;
        return false;
    }
    
    uint16_t num_regs = values.size();
    
    // 验证写入数量
    if (num_regs > MAX_REGISTER_COUNT) {
        std::cerr << "写入寄存器数量超过最大值 " << MAX_REGISTER_COUNT << std::endl;
        return false;
    }
    
    uint8_t byte_count = num_regs * 2;
    
    // 构建Modbus RTU请求帧
    std::vector<uint8_t> request(9 + byte_count);
    request[0] = SLAVE_ID;                    // 从设备地址
    request[1] = 0x10;                        // 功能码：写多个寄存器
    request[2] = (start_addr >> 8) & 0xFF;    // 起始地址高字节
    request[3] = start_addr & 0xFF;           // 起始地址低字节
    request[4] = (num_regs >> 8) & 0xFF;      // 寄存器数量高字节
    request[5] = num_regs & 0xFF;             // 寄存器数量低字节
    request[6] = byte_count;                  // 字节数
    
    // 填充寄存器值
    for (int i = 0; i < num_regs; i++) {
        request[7 + i * 2] = (values[i] >> 8) & 0xFF;    // 高字节
        request[8 + i * 2] = values[i] & 0xFF;           // 低字节
    }
    
    // 计算CRC
    uint16_t crc = calculateCRC16(request.data(), request.size() - 2);
    request[request.size() - 2] = crc & 0xFF;         // CRC低字节
    request[request.size() - 1] = (crc >> 8) & 0xFF;  // CRC高字节
    
    printHex("发送写入请求: ", request.data(), request.size());
    
    // 清空接收缓冲区
    flushBuffer();
    
    // 发送请求
    if (!sendData(request.data(), request.size())) {
        return false;
    }
    
    // 等待一段时间让设备处理
    usleep(50000); // 50ms延时
    
    // 接收响应
    uint8_t response[8];
    int response_len = receiveData(response, sizeof(response));
    if (response_len != 8) {
        std::cerr << "接收响应数据长度错误" << std::endl;
        return false;
    }
    
    printHex("接收响应: ", response, response_len);
    
    // 验证响应
    if (response[0] != SLAVE_ID || response[1] != 0x10) {
        std::cerr << "写入响应验证失败" << std::endl;
        return false;
    }
    
    // 验证CRC
    uint16_t received_crc = (response[7] << 8) | response[6];
    uint16_t calculated_crc = calculateCRC16(response, 6);
    if (received_crc != calculated_crc) {
        std::cerr << "响应CRC校验失败" << std::endl;
        return false;
    }
    
    return true;
}
