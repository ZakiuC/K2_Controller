#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

#include <vector>
#include <cstdint>

class ModbusRTU {
private:
    int fd;
    
    // CRC16 计算
    uint16_t calculateCRC16(const uint8_t* data, int length);
    
    // 发送数据
    bool sendData(const uint8_t* data, int length);
    
    // 查找Modbus响应帧的起始位置
    int findModbusFrame(const uint8_t* buffer, int total_length, 
                       uint8_t expected_slave_id, uint8_t expected_function_code);
    
    // 接收数据
    int receiveData(uint8_t* buffer, int max_length, int timeout_ms = 3000);
    
    // 打印十六进制数据
    void printHex(const char* prefix, const uint8_t* data, int length);

public:
    ModbusRTU();
    ~ModbusRTU();
    
    // 初始化串口
    bool init();
    
    // 清空串口缓冲区
    void flushBuffer();
    
    // 关闭串口
    void close();
    
    // 读取保持寄存器 (功能码 0x03)
    bool readHoldingRegisters(uint16_t start_addr, uint16_t num_regs, 
                              std::vector<uint16_t>& values);
    
    // 写单个寄存器 (功能码 0x06)
    bool writeSingleRegister(uint16_t reg_addr, uint16_t value);
    
    // 写多个寄存器 (功能码 0x10)
    bool writeMultipleRegisters(uint16_t start_addr, const std::vector<uint16_t>& values);
};

#endif // MODBUS_RTU_H
