# Modbus DTU 通信测试程序

这是一个用于测试Modbus RTU通信的C++程序，已经重构为模块化结构。

## 项目结构

```
485_DTU/
├── main.cpp           # 主程序入口
├── modbus_rtu.h       # ModbusRTU类声明
├── modbus_rtu.cpp     # ModbusRTU类实现
├── modbus_test.h      # 测试函数声明
├── modbus_test.cpp    # 测试函数实现
├── modbus_config.h    # 配置参数声明
├── modbus_config.cpp  # 配置参数定义
├── CMakeLists.txt     # CMake构建文件
├── Makefile          # Make构建文件
└── README.md         # 本文件
```

## 模块说明

### 1. modbus_config.h/cpp
包含所有Modbus RTU的配置参数：
- 串口设备路径
- 通信参数（波特率、数据位、停止位等）
- 从设备ID
- 寄存器地址配置

### 2. modbus_rtu.h/cpp
ModbusRTU类的完整实现，包含：
- 串口初始化和配置
- CRC16校验算法
- 数据发送和接收
- Modbus功能码实现（0x03读取、0x06单写、0x10多写）

### 3. modbus_test.h/cpp
测试函数实现，包含完整的设备测试流程。

### 4. main.cpp
简化的主程序，只负责调用测试函数和异常处理。

## 编译方法

### 使用Make编译
```bash
# 编译项目
make

# 编译并运行
make run

# 编译调试版本
make debug

# 清理编译文件
make clean

# 查看帮助
make help
```

### 使用CMake编译
```bash
# 创建构建目录
mkdir -p build
cd build

# 配置和编译
cmake ..
make

# 运行程序
./bin/modbus_dtu
```

## 配置修改

如需修改通信参数，请编辑 `modbus_config.h` 文件中的相关常量：

- `SERIAL_PORT`: 串口设备路径
- `BAUD_RATE`: 波特率
- `SLAVE_ID`: 从设备地址
- `READ_REG_ADDR`: 读取寄存器起始地址
- `WRITE_REG_ADDR`: 写入寄存器起始地址
- `MAX_REGISTER_COUNT`: 最大寄存器操作数量

## 功能特性

- 支持Modbus RTU协议
- 自动CRC16校验
- 智能帧检测和解析
- 详细的调试信息输出
- 错误处理和异常管理
- 模块化设计，易于扩展和维护

## 使用注意事项

1. 确保串口设备存在且有访问权限
2. 根据实际硬件调整配置参数
3. 程序需要root权限或串口设备访问权限
