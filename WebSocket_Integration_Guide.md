# K2_Controller WebSocket 集成说明

## 概述

已成功将您的 WebSocket 客户端代码集成到 K2_Controller 项目中。现在您的控制器支持通过 WebSocket 进行远程控制。

## 功能特性

- **自动重连**: 网络断开时自动重连服务器
- **命令解析**: 支持标准的命令格式 `cmd@param1#param2#param3`
- **统一管理**: 通过 ControlCenter 统一管理所有控制模式
- **日志记录**: 完整的日志记录功能
- **线程安全**: 多线程安全的实现

## 支持的命令

| 命令代码 | 命令名称 | 参数说明 | 示例 |
|---------|----------|----------|------|
| 0x0001  | 履带位置模式 | 4个参数: 角度,速度,电流1,电流2 | `0x0001@90.5#1000#100#200` |
| 0x0002  | 履带速度模式 | 3个参数: 速度,电流1,电流2 | `0x0002@500#150#150` |
| 0x0003  | 毛刷升降 | 2个参数: 位置1,位置2 | `0x0003@100#200` |
| 0x0004  | 8路继电器 | 1个参数: 状态位 | `0x0004@11110000` |
| 0x0005  | PWM推进电机 | 可变参数 | `0x0005@参数...` |
| 0x0006  | 摄像头视频流 | 1个参数: 开关(1/0) | `0x0006@1` |

## 安装和编译

### 1. 安装依赖

```bash
# 自动安装依赖
./install_deps.sh

# 或手动安装
sudo apt-get update
sudo apt-get install libboost-all-dev libnlohmann-json3-dev
```

### 2. 编译项目

```bash
# 使用新的 WebSocket 构建脚本
./make_websocket.sh

# 或手动编译
mkdir build && cd build
cmake -DENABLE_REMOTE_CONTROL=ON -DBUILD_TESTS=ON ..
make -j$(nproc)
```

## 使用方法

### 1. 主程序集成使用

```bash
./bin/K2_Controler
```

在主程序菜单中：
1. 选择 "5. WebSocket 控制"
2. 选择 "1. 启动 WebSocket 客户端"
3. 输入服务器信息（默认: 115.190.42.239:8080）
4. 选择 "3. 切换控制模式" -> "2. WebSocket"

### 2. 独立测试程序

```bash
./bin/websocket_test
```

用于单独测试 WebSocket 连接和命令处理。

## 配置文件

### WebSocket 配置 (`config/websocket_config.h`)

```cpp
#define WEBSOCKET_SERVER_IP     "115.190.42.239"
#define WEBSOCKET_SERVER_PORT   8080
#define WEBSOCKET_PING_INTERVAL 20
#define WEBSOCKET_RECONNECT_DELAY 5
```

## 代码结构

```
K2_Controller/
├── include/remote/websocket_client.h    # WebSocket 客户端头文件
├── src/remote/websocket_client.cpp      # WebSocket 客户端实现
├── config/websocket_config.h            # WebSocket 配置
├── tests/websocket_test.cpp             # WebSocket 测试程序
├── install_deps.sh                      # 依赖安装脚本
└── make_websocket.sh                    # 构建脚本
```

## 集成要点

### 1. ControlCenter 集成

WebSocket 客户端已经完全集成到 ControlCenter 中：

```cpp
// 启动 WebSocket 客户端
controlCenter.startWebSocketClient("115.190.42.239", 8080);

// 切换到 WebSocket 控制模式
controlCenter.setControlMode(ControlMode::WEBSOCKET);

// 检查连接状态
bool connected = controlCenter.isWebSocketConnected();
```

### 2. 命令处理

您可以自定义命令处理逻辑：

```cpp
// 在 ControlCenter::handleWebSocketCommand 中添加您的设备控制代码
std::string ControlCenter::handleWebSocketCommand(const std::string& cmd, 
                                                  const std::vector<std::string>& params) {
    if (cmd == "0x0001") {  // 履带位置控制
        // 调用您的设备控制接口
        // deviceManager.sendCommand("track_motor", POSITION_CMD, data);
        return "履带位置控制执行完成";
    }
    // 其他命令处理...
}
```

### 3. 设备接口适配

根据您的实际设备接口，在 `handleWebSocketCommand` 方法中调用相应的设备控制函数。

## 调试和日志

- 所有 WebSocket 活动都会记录到日志文件
- 可以通过 `Logger::getInstance().setConsoleOutput(true)` 启用终端输出
- 调试信息包括连接状态、收发消息、命令执行结果等

## 注意事项

1. 确保服务器端已经运行并可访问
2. 网络防火墙需要允许对应端口的连接
3. 如果使用自定义设备接口，需要在命令处理函数中实现具体的设备控制逻辑
4. 建议先使用测试程序验证 WebSocket 连接正常

## 故障排除

### 编译错误
- 检查依赖库是否正确安装
- 确认 CMake 版本 >= 3.10

### 连接问题
- 检查服务器 IP 和端口是否正确
- 确认网络连通性
- 查看日志文件获取详细错误信息

### 命令执行问题
- 检查命令格式是否正确
- 确认参数数量和类型
- 查看日志文件中的命令解析信息

## 扩展开发

如需添加新的命令或修改现有命令处理逻辑，请：

1. 在 `websocket_client.h` 中添加新的命令常量
2. 在 `ControlCenter::handleWebSocketCommand` 中添加对应的处理逻辑
3. 根据需要更新设备管理器接口调用

这样您就可以通过 WebSocket 远程控制您的 K2_Controller 了！
