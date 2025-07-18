cmake_minimum_required(VERSION 3.10)
project(test_controller)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/bin)

# 包含目录
include_directories(
    ${PROJECT_SOURCE_DIR}
)

# 明确指定源文件
set(SOURCES
    main.cpp
    CANInterface.cpp
    MotorController.cpp
    RS232Interface.cpp
    LH08Controller.cpp
)


# 主程序
add_executable(test_controller ${SOURCES})


# 链接系统库
find_package(Threads REQUIRED)
target_link_libraries(test_controller PRIVATE Threads::Threads)

# 安装目标
install(TARGETS test_controller DESTINATION bin)
