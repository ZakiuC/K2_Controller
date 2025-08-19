#include "modbus_test.h"
#include "modbus_rtu.h"
#include "modbus_config.h"
#include <iostream>
#include <iomanip>
#include <unistd.h>

void testModbusRTU() {
    ModbusRTU modbus;
    
    std::cout << "=== Modbus RTU 通信测试 ===" << std::endl;
    std::cout << "设备配置:" << std::endl;
    std::cout << "  串口: " << SERIAL_PORT << std::endl;
    std::cout << "  波特率: 9600" << std::endl;
    std::cout << "  数据位: 8" << std::endl;
    std::cout << "  停止位: 1" << std::endl;
    std::cout << "  校验: 无" << std::endl;
    std::cout << "  从设备ID: 0x" << std::hex << (int)SLAVE_ID << std::dec << std::endl;
    std::cout << "  读寄存器地址: 0x" << std::hex << READ_REG_ADDR << std::dec << std::endl;
    std::cout << "  写寄存器地址: 0x" << std::hex << WRITE_REG_ADDR << std::dec << std::endl;
    std::cout << "  最大寄存器数量: " << MAX_REGISTER_COUNT << std::endl;
    std::cout << std::endl;
    
    // 初始化串口
    if (!modbus.init()) {
        std::cerr << "串口初始化失败" << std::endl;
        return;
    }
    
    // 等待一下让串口稳定
    usleep(100000); // 100ms
    
    // 测试读取寄存器
    std::cout << "--- 测试读取寄存器 ---" << std::endl;
    std::vector<uint16_t> read_values;
    if (modbus.readHoldingRegisters(READ_REG_ADDR, 30, read_values)) {
        std::cout << "读取成功，寄存器值:" << std::endl;
        for (size_t i = 0; i < read_values.size(); i++) {
            std::cout << "  0x" << std::hex << (READ_REG_ADDR + i) 
                      << ": 0x" << read_values[i] << " (" << std::dec 
                      << read_values[i] << ")" << std::endl;
        }
    } else {
        std::cout << "读取失败" << std::endl;
    }
    
    // 测试写单个寄存器
    std::cout << "--- 测试写单个寄存器 ---" << std::endl;
    uint16_t write_value = 0x00;
    if (modbus.writeSingleRegister(WRITE_REG_ADDR, write_value)) {
        std::cout << "写入单个寄存器成功: 0x" << std::hex << WRITE_REG_ADDR 
                  << " = 0x" << write_value << std::dec << std::endl;
    } else {
        std::cout << "写入单个寄存器失败" << std::endl;
    }
    
    std::cout << std::endl;
}
