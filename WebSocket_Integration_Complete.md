# K2_Controller WebSocket 集成完成

## 恭喜！

您的 WebSocket 客户端代码已成功集成到 K2_Controller 项目中。现在您可以通过 WebSocket 远程控制您的设备了。

## 🎉 集成完成情况

✅ **WebSocket 客户端**: 完全集成
✅ **自动重连功能**: 已实现
✅ **命令解析**: 支持您原有的命令格式
✅ **日志记录**: 完整的日志系统
✅ **线程安全**: 多线程安全实现
✅ **统一管理**: 通过 ControlCenter 统一管理
✅ **编译成功**: 主程序和测试程序都已生成

## 📁 新增文件

```
K2_Controller/
├── include/remote/websocket_client.h        # WebSocket 客户端头文件
├── src/remote/websocket_client.cpp          # WebSocket 客户端实现
├── config/websocket_config.h                # WebSocket 配置文件
├── tests/websocket_test.cpp                 # WebSocket 测试程序
├── install_deps.sh                          # 依赖安装脚本
├── make_websocket.sh                        # WebSocket 构建脚本
├── WebSocket_Integration_Guide.md           # 详细集成指南
└── bin/
    ├── K2_Controler                         # 主程序（已更新）
    └── websocket_test                       # WebSocket 测试程序
```

## 🚀 使用方法

### 1. 启动主程序（包含 WebSocket 功能）

```bash
cd /root/K2_Controller
./bin/K2_Controler
```

在主菜单中：
- 选择 "5. WebSocket 控制"
- 选择 "1. 启动 WebSocket 客户端"
- 输入服务器信息（默认: 115.190.42.239:8080）
- 选择 "3. 切换控制模式" -> "2. WebSocket"

### 2. 运行独立测试程序

```bash
cd /root/K2_Controller
./bin/websocket_test
```

### 3. 支持的 WebSocket 命令

| 命令代码 | 功能 | 参数格式 | 示例 |
|---------|------|----------|------|
| `0x0001` | 履带位置模式 | `角度@速度#电流1#电流2` | `0x0001@90.5#1000#100#200` |
| `0x0002` | 履带速度模式 | `速度@电流1#电流2` | `0x0002@500#150#150` |
| `0x0003` | 毛刷升降 | `位置1@位置2` | `0x0003@100#200` |
| `0x0004` | 8路继电器 | `状态位` | `0x0004@11110000` |
| `0x0005` | PWM推进电机 | `参数...` | `0x0005@参数...` |
| `0x0006` | 摄像头视频流 | `开关(1/0)` | `0x0006@1` |

## 🔧 配置选项

### 修改服务器配置

编辑 `config/websocket_config.h`:

```cpp
#define WEBSOCKET_SERVER_IP     "您的服务器IP"
#define WEBSOCKET_SERVER_PORT   您的端口
```

### 添加新的命令

1. 在 `include/remote/websocket_client.h` 中添加命令常量
2. 在 `src/core/control_center.cpp` 的 `handleWebSocketCommand` 方法中添加处理逻辑
3. 根据需要调用相应的设备管理器接口

## 🔍 日志和调试

- 日志文件位置: `logs/device_control.log` 或 `logs/websocket_test.log`
- 可以通过程序菜单切换终端输出
- 所有 WebSocket 活动都会被记录

## 🚨 故障排除

### 如果连接失败
1. 检查服务器 IP 和端口是否正确
2. 确认网络连通性：`ping 115.190.42.239`
3. 检查防火墙设置
4. 查看日志文件获取详细错误信息

### 如果命令不生效
1. 检查命令格式是否正确
2. 确认参数数量和类型
3. 查看日志中的命令解析信息
4. 确保已切换到 WebSocket 控制模式

### 重新编译（如果修改了代码）
```bash
cd /root/K2_Controller
./make_websocket.sh
```

## 🎯 下一步

1. **自定义命令处理**: 在 `ControlCenter::handleWebSocketCommand` 中实现您的具体设备控制逻辑
2. **参数验证**: 根据需要添加更严格的参数验证
3. **状态反馈**: 实现设备状态主动上报功能
4. **错误处理**: 完善异常情况的处理

## 📞 技术支持

如果您在使用过程中遇到任何问题，请：

1. 查看日志文件获取详细错误信息
2. 使用测试程序验证基本连接功能
3. 检查服务器端是否正常运行
4. 确认网络连接是否正常

---

**🎊 集成完成！您现在可以通过 WebSocket 远程控制您的 K2_Controller 了！**
