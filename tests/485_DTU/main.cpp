#include <iostream>
#include "modbus_test.h"

int main() {
    try {
        testModbusRTU();
    } catch (const std::exception& e) {
        std::cerr << "程序异常: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}