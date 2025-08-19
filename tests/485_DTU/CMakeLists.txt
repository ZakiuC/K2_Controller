cmake_minimum_required(VERSION 3.10)
project(modbus_dtu)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# 包含头文件目录
include_directories(.)

# 创建可执行文件
add_executable(modbus_dtu
    main.cpp
    modbus_rtu.cpp
    modbus_test.cpp
    modbus_config.cpp
)

# 链接必要的库
target_link_libraries(modbus_dtu)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
