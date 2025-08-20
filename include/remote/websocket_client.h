#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include <queue>
#include <condition_variable>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class DeviceManager;

// WebSocket消息结构
struct WebSocketMessage {
    std::string deviceId;
    std::string command;
    std::string data;
    std::string type; // "command", "query", "response"
};

class WebSocketClient {
public:
    using MessageHandler = std::function<void(const WebSocketMessage&)>;
    
    WebSocketClient(const std::string& host, const std::string& port);
    ~WebSocketClient();
    
    // 连接和断开
    bool connect();
    void disconnect();
    bool isConnected() const;
    
    // 消息发送和接收
    void sendMessage(const WebSocketMessage& message);
    void sendDeviceCommand(const std::string& deviceId, const std::string& command, const std::string& data = "");
    void sendDeviceStatus(const std::string& deviceId, const std::string& status);
    void sendResponse(const std::string& result, const std::string& message = "");
    
    // 设置消息处理器
    void setMessageHandler(MessageHandler handler);
    
    // 运行WebSocket客户端（阻塞调用）
    void run();
    
    // 停止运行
    void stop();

private:
    // 网络相关
    net::io_context ioc_;
    websocket::stream<tcp::socket> ws_;
    std::string host_;
    std::string port_;
    
    // 线程管理
    std::thread receive_thread_;
    std::thread send_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};
    
    // 消息队列
    std::queue<std::string> send_queue_;
    std::mutex send_mutex_;
    std::condition_variable send_cv_;
    
    // 消息处理
    MessageHandler message_handler_;
    std::mutex handler_mutex_;
    
    // 同步输出
    std::mutex cout_mutex_;
    
    // 内部方法
    void receiveLoop();
    void sendLoop();
    std::string messageToJson(const WebSocketMessage& message);
    WebSocketMessage jsonToMessage(const std::string& json);
    void handleIncomingMessage(const std::string& message);
};
