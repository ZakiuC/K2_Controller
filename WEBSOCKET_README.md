# K2_Controller WebSocket控制功能

## 概述

K2_Controller现在支持通过WebSocket进行远程控制。WebSocket服务器已部署在 `115.190.42.239:8080`。

## 功能特性

### 1. 实时双向通信
- 支持从Web界面或其他WebSocket客户端控制设备
- 实时接收设备状态更新
- 支持多种命令类型：设备控制、系统查询、状态监控

### 2. 消息格式

所有WebSocket消息采用JSON格式：

```json
{
  "type": "command|system|query|response",
  "deviceId": "设备ID",
  "command": "命令",
  "data": "数据",
  "timestamp": 时间戳
}
```

### 3. 支持的命令类型

#### 系统命令 (type: "system")
- `get_devices` - 获取设备列表
- `get_status` - 获取系统状态
- `set_control_mode` - 设置控制模式为WebSocket

#### 设备命令 (type: "command")
- 发送设备控制命令
- 命令格式支持十进制和十六进制 (如: "0x00", "255")
- 数据格式支持空格分隔的十六进制字符串或JSON数组

#### 查询命令 (type: "query")
- `device_status` - 查询特定设备状态

## 使用方法

### 1. 编译项目

```bash
cd /root/K2_Controller
mkdir -p build
cd build
cmake ..
make
```

### 2. 启动K2_Controller

使用启动脚本：
```bash
cd /root/K2_Controller
./start_k2.sh
```

或直接运行：
```bash
cd /root/K2_Controller
./bin/K2_Controler
```

### 3. 启用WebSocket模式

在终端界面中：
1. 选择 "3. 切换控制模式"
2. 选择 "2. WebSocket"
3. 选择 "5. WebSocket状态" 来检查连接状态

### 4. 测试WebSocket连接

#### 方法1: 使用测试客户端
```bash
cd /root/K2_Controller/tests/websocket_chat/build/client
./websocket_client 115.190.42.239 8080
```

#### 方法2: 使用Python测试脚本
```bash
cd /root/K2_Controller
python3 test_websocket.py
```

## WebSocket消息示例

### 1. 获取设备列表
发送:
```json
{
  "type": "system",
  "command": "get_devices", 
  "deviceId": "",
  "data": ""
}
```

响应:
```json
{
  "type": "response",
  "command": "system",
  "data": "[{\"id\":\"motor_4\",\"status\":\"connected\"}]"
}
```

### 2. 发送设备命令
发送:
```json
{
  "type": "command",
  "command": "0x00",
  "deviceId": "motor_4", 
  "data": "00 00 00 00 64 00 00 00"
}
```

响应:
```json
{
  "type": "response",
  "command": "system",
  "data": "{\"result\":\"success\",\"message\":\"设备 motor_4 命令 0x00 已发送\"}"
}
```

### 3. 查询系统状态
发送:
```json
{
  "type": "system",
  "command": "get_status",
  "deviceId": "",
  "data": ""
}
```

响应:
```json
{
  "type": "response", 
  "command": "system",
  "data": "{\"control_mode\":\"WEBSOCKET\",\"connected_devices\":1,\"websocket_connected\":true}"
}
```

## Web界面集成

### JavaScript示例

```javascript
// 连接WebSocket
const ws = new WebSocket('ws://115.190.42.239:8080');

ws.onopen = function(event) {
    console.log('WebSocket连接已建立');
    
    // 获取设备列表
    ws.send(JSON.stringify({
        type: "system",
        command: "get_devices",
        deviceId: "",
        data: ""
    }));
};

ws.onmessage = function(event) {
    const message = JSON.parse(event.data);
    console.log('收到消息:', message);
    
    // 处理响应
    if (message.type === "response") {
        // 更新界面
        updateUI(message);
    }
};

// 发送设备命令
function sendDeviceCommand(deviceId, command, data) {
    ws.send(JSON.stringify({
        type: "command",
        command: command,
        deviceId: deviceId,
        data: data
    }));
}

// 示例：控制电机
sendDeviceCommand("motor_4", "0x00", "00 00 00 00 64 00 00 00");
```

## 故障排除

### 1. 连接失败
- 检查服务器地址和端口 (115.190.42.239:8080)
- 确认网络连接正常
- 查看日志文件: `tail -f /root/K2_Controller/logs/device_control.log`

### 2. 命令无响应
- 确认已切换到WebSocket控制模式
- 检查设备ID是否正确
- 验证命令格式是否正确

### 3. 编译错误
- 确保安装了Boost库: `sudo apt-get install libboost-dev libboost-system-dev`
- 检查CMake版本: `cmake --version` (需要3.10+)

## 日志监控

查看实时日志：
```bash
tail -f /root/K2_Controller/logs/device_control.log
```

## 安全注意事项

1. WebSocket连接未加密，仅适用于受信任的网络环境
2. 建议在生产环境中添加认证机制
3. 限制访问来源IP地址
4. 定期监控连接状态和异常行为

## 扩展开发

### 添加新的WebSocket命令

1. 在 `WebSocketController::handleWebSocketMessage()` 中添加新的命令类型
2. 实现对应的处理函数
3. 更新消息格式文档
4. 重新编译项目

### 集成其他协议

WebSocket控制器设计为模块化，可以轻松集成：
- MQTT支持
- HTTP REST API
- 其他实时通信协议
