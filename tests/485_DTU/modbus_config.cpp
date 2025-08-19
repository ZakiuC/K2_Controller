#include "modbus_config.h"
#include <termios.h>

// Modbus RTU 配置参数实现
const char* SERIAL_PORT = "/dev/ttyS0";
const int BAUD_RATE = B9600;
const uint8_t SLAVE_ID = 0x0A;
const uint16_t READ_REG_ADDR = 3000;
const uint16_t WRITE_REG_ADDR = 3100;
const uint16_t MAX_REGISTER_COUNT = 30;  // 最大读写寄存器数量
