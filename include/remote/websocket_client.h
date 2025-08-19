#pragma once
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <future>
#include <chrono>
#include <memory>
#include <functional>
#include <iomanip>
#include <sstream>
#include <vector>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// 如果您的项目中没有这些库，需要安装
// sudo apt-get install libnlohmann-json3-dev libboost-beast-dev
#include <nlohmann/json.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_context.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using json = nlohmann::json;

// WebSocket 命令定义
#define WS_TRACK_POSITION_CMD   "0x0001"    // 履带位置模式
#define WS_TRACK_SPEED_CMD      "0x0002"    // 履带速度模式
#define WS_BRUSH_LIFTING_CMD    "0x0003"    // 毛刷升降
#define WS_EIGHT_SWITCH_CMD     "0x0004"    // 八路继电器
#define WS_PUSHFORWARD_CMD      "0x0005"    // PWM推进电机
#define WS_VIDEO_STREAM_CMD     "0x0006"    // 摄像头视频流

class ControlCenter;  // 前向声明

class WebSocketClient {
public:
    // 命令回调函数类型
    using CommandCallback = std::function<std::string(const std::string& cmd, const std::vector<std::string>& params)>;

    explicit WebSocketClient(const std::string& uri, unsigned short port);
    explicit WebSocketClient(const std::string& uri, unsigned short port, const std::string& role);
    ~WebSocketClient();

    // 基本操作
    void run();
    void stop();
    bool isConnected() const;
    
    // 消息发送
    void sendMessage(const std::string& content, const std::string& reply = "no");
    
    // 命令处理注册
    void registerCommandCallback(CommandCallback callback);
    
    // 设置 ControlCenter 引用（用于设备控制）
    void setControlCenter(ControlCenter* cc);

private:
    // 网络相关
    const std::string uri_;
    const unsigned short port_;
    const int ping_interval_ = 20;
    const int reconnect_delay_ = 5;

    net::io_context ioc_;
    tcp::resolver resolver_;
    websocket::stream<tcp::socket> ws_;

    // 状态管理
    std::atomic<bool> connected_{false};
    std::string client_id_;
    std::string role_ = "subscriber";  // 默认为订阅者
    bool should_exit_ = false;

    // 线程管理
    std::thread main_thread_;
    std::thread heartbeat_thread_;
    std::thread receive_thread_;

    // 回调和控制
    CommandCallback command_callback_;
    ControlCenter* control_center_ = nullptr;

    // 内部方法
    std::string getLocalIP();
    std::string getFormattedTime();
    bool handshake();
    void connectForever();
    void receiveLoop();
    void heartbeatLoop();
    void handleReceivedData(const json& data);
    std::string processCommand(const std::string& cmd, const std::vector<std::string>& params);
};
