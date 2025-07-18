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

# 诊断工具源文件
set(DIAGNOSTIC_SOURCES
    can_diagnostic.cpp
)

# 主程序
add_executable(test_controller ${SOURCES})

# 诊断工具程序
add_executable(can_diagnostic ${DIAGNOSTIC_SOURCES})

# 链接系统库
find_package(Threads REQUIRED)
<<<<<<< HEAD:tests/new_crawler/CMakeLists.txt
target_link_libraries(test_controller PRIVATE Threads::Threads)

# 安装目标
install(TARGETS test_controller DESTINATION bin)
=======
target_link_libraries(can PRIVATE Threads::Threads)
target_link_libraries(can_diagnostic PRIVATE Threads::Threads)

# 安装目标
install(TARGETS can can_diagnostic DESTINATION bin)
>>>>>>> origin/main:tests/can/CMakeLists.txt
