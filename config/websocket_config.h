#pragma once

// WebSocket 服务器配置
#define WEBSOCKET_SERVER_IP     "115.190.42.239"
#define WEBSOCKET_SERVER_PORT   8080

// WebSocket 客户端配置
#define WEBSOCKET_PING_INTERVAL     20      // 心跳间隔（秒）
#define WEBSOCKET_RECONNECT_DELAY   5       // 重连延迟（秒）
#define WEBSOCKET_CLIENT_ROLE       "subscriber"  // 客户端角色

// WebSocket 命令超时配置
#define WEBSOCKET_COMMAND_TIMEOUT   5000    // 命令超时时间（毫秒）

// 调试选项
#define WEBSOCKET_DEBUG_MODE        1       // 是否启用调试模式
