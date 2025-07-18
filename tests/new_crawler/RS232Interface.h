#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include <mutex>
#include <sstream>


// 设置串口
int setup_serial_port(const char *port, speed_t baudrate);

