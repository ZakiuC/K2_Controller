cmake_minimum_required(VERSION 3.10)
project(K2_Controler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 包含目录
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/core
    ${PROJECT_SOURCE_DIR}/include/protocols
    ${PROJECT_SOURCE_DIR}/include/devices
    ${PROJECT_SOURCE_DIR}/include/utils
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/config
)

# 源文件
file(GLOB_RECURSE SOURCES 
    "src/core/*.cpp"
    "src/protocols/*.cpp"
    "src/devices/*.cpp"
    "src/utils/*.cpp"
)

# 主程序
add_executable(K2_Controler src/main.cpp ${SOURCES})

# 远程控制模块 (可选)
option(ENABLE_REMOTE_CONTROL "Enable remote control features" ON)
if(ENABLE_REMOTE_CONTROL)
    file(GLOB REMOTE_SOURCES "src/remote/*.cpp")
    target_sources(K2_Controler PRIVATE ${REMOTE_SOURCES})
    
    # # 查找并链接WebSocket库
    # find_package(Libwebsockets REQUIRED)
    # target_link_libraries(K2_Controler PRIVATE websockets)
    
    # # 查找并链接MQTT库
    # find_package(PahoMqttCpp REQUIRED)
    # target_link_libraries(K2_Controler PRIVATE PahoMqttCpp)
endif()

# 链接系统库
find_package(Threads REQUIRED)
target_link_libraries(K2_Controler PRIVATE Threads::Threads)

# 安装目标
install(TARGETS K2_Controler DESTINATION bin)