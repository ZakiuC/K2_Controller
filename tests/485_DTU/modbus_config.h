#ifndef MODBUS_CONFIG_H
#define MODBUS_CONFIG_H

#include <termios.h>
#include <cstdint>

// Modbus RTU 配置参数
extern const char* SERIAL_PORT;
extern const int BAUD_RATE;
extern const uint8_t SLAVE_ID;
extern const uint16_t READ_REG_ADDR;
extern const uint16_t WRITE_REG_ADDR;
extern const uint16_t MAX_REGISTER_COUNT;

#endif // MODBUS_CONFIG_H
