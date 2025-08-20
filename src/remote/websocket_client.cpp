#define BOOST_BEAST_USE_STD_STRING_VIEW

#include "websocket_client.h"
#include "logger.h"
#include <iostream>
#include <condition_variable>
#include <sstream>
#include <ctime>

WebSocketClient::WebSocketClient(const std::string& host, const std::string& port)
    : ws_(ioc_), host_(host), port_(port) {
    LOG_INFO("WebSocket客户端初始化: " + host + ":" + port);
}

WebSocketClient::~WebSocketClient() {
    disconnect();
}

bool WebSocketClient::connect() {
    try {
        // 解析主机名和端口
        tcp::resolver resolver(ioc_);
        auto const results = resolver.resolve(host_, port_);
        
        // 连接到服务器
        auto ep = net::connect(ws_.next_layer(), results.begin(), results.end());
        
        // 完成WebSocket握手
        ws_.handshake(host_, "/");
        
        connected_ = true;
        running_ = true;
        
        LOG_INFO("WebSocket连接成功: " + host_ + ":" + port_);
        
        // 启动接收和发送线程
        receive_thread_ = std::thread([this]() { receiveLoop(); });
        send_thread_ = std::thread([this]() { sendLoop(); });
        
        // 发送连接确认消息
        sendResponse("connected", "K2_Controller已连接");
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("WebSocket连接失败: " + std::string(e.what()));
        connected_ = false;
        return false;
    }
}

void WebSocketClient::disconnect() {
    if (running_) {
        running_ = false;
        connected_ = false;
        
        // 发送断开连接消息
        sendResponse("disconnected", "K2_Controller断开连接");
        
        // 等待消息发送完成
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 关闭WebSocket连接
        beast::error_code ec;
        ws_.close(websocket::close_code::normal, ec);
        
        if (ec) {
            LOG_ERROR("WebSocket关闭错误: " + ec.message());
        }
        
        // 唤醒发送线程
        send_cv_.notify_all();
        
        // 等待线程结束
        if (receive_thread_.joinable()) {
            receive_thread_.join();
        }
        if (send_thread_.joinable()) {
            send_thread_.join();
        }
        
        LOG_INFO("WebSocket连接已断开");
    }
}

bool WebSocketClient::isConnected() const {
    return connected_;
}

void WebSocketClient::sendMessage(const WebSocketMessage& message) {
    if (!connected_) {
        LOG_WARNING("WebSocket未连接，无法发送消息");
        return;
    }
    
    std::string json_str = messageToJson(message);
    
    {
        std::lock_guard<std::mutex> lock(send_mutex_);
        send_queue_.push(json_str);
    }
    send_cv_.notify_one();
}

void WebSocketClient::sendDeviceCommand(const std::string& deviceId, const std::string& command, const std::string& data) {
    WebSocketMessage msg;
    msg.type = "command";
    msg.deviceId = deviceId;
    msg.command = command;
    msg.data = data;
    sendMessage(msg);
}

void WebSocketClient::sendDeviceStatus(const std::string& deviceId, const std::string& status) {
    WebSocketMessage msg;
    msg.type = "status";
    msg.deviceId = deviceId;
    msg.command = "status_update";
    msg.data = status;
    sendMessage(msg);
}

void WebSocketClient::sendResponse(const std::string& result, const std::string& message) {
    WebSocketMessage msg;
    msg.type = "response";
    msg.command = "system";
    msg.data = "{\"result\":\"" + result + "\",\"message\":\"" + message + "\"}";
    sendMessage(msg);
}

void WebSocketClient::setMessageHandler(MessageHandler handler) {
    std::lock_guard<std::mutex> lock(handler_mutex_);
    message_handler_ = handler;
}

void WebSocketClient::run() {
    if (!connected_) {
        LOG_ERROR("WebSocket未连接，无法运行");
        return;
    }
    
    // 主线程可以处理其他逻辑，或者简单等待
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void WebSocketClient::stop() {
    disconnect();
}

void WebSocketClient::receiveLoop() {
    beast::flat_buffer buffer;
    
    while (running_) {
        beast::error_code ec;
        ws_.read(buffer, ec);
        
        if (ec == websocket::error::closed) {
            if (running_) {
                LOG_INFO("WebSocket连接被服务器关闭");
                connected_ = false;
            }
            break;
        } else if (ec) {
            if (running_) {
                LOG_ERROR("WebSocket读取错误: " + ec.message());
                connected_ = false;
            }
            break;
        }
        
        // 处理收到的消息
        auto msg = beast::buffers_to_string(buffer.data());
        handleIncomingMessage(msg);
        buffer.consume(buffer.size());
    }
}

void WebSocketClient::sendLoop() {
    while (running_) {
        std::unique_lock<std::mutex> lock(send_mutex_);
        
        // 等待有消息要发送或者停止运行
        send_cv_.wait(lock, [this] { return !send_queue_.empty() || !running_; });
        
        if (!running_) break;
        
        while (!send_queue_.empty() && running_ && connected_) {
            std::string message = send_queue_.front();
            send_queue_.pop();
            lock.unlock();
            
            try {
                ws_.write(net::buffer(message));
                LOG_DEBUG("WebSocket发送消息: " + message);
            } catch (const std::exception& e) {
                LOG_ERROR("WebSocket发送错误: " + std::string(e.what()));
                connected_ = false;
                break;
            }
            
            lock.lock();
        }
    }
}

std::string WebSocketClient::messageToJson(const WebSocketMessage& message) {
    std::ostringstream oss;
    oss << "{"
        << "\"type\":\"" << message.type << "\","
        << "\"deviceId\":\"" << message.deviceId << "\","
        << "\"command\":\"" << message.command << "\","
        << "\"data\":\"" << message.data << "\","
        << "\"timestamp\":" << std::time(nullptr)
        << "}";
    return oss.str();
}

WebSocketMessage WebSocketClient::jsonToMessage(const std::string& json) {
    WebSocketMessage message;
    
    try {
        // 简单的JSON解析实现
        size_t pos = 0;
        
        // 查找type字段
        pos = json.find("\"type\":\"");
        if (pos != std::string::npos) {
            pos += 8; // 跳过 "type":"
            size_t end = json.find("\"", pos);
            if (end != std::string::npos) {
                message.type = json.substr(pos, end - pos);
            }
        }
        
        // 查找deviceId字段
        pos = json.find("\"deviceId\":\"");
        if (pos != std::string::npos) {
            pos += 12; // 跳过 "deviceId":"
            size_t end = json.find("\"", pos);
            if (end != std::string::npos) {
                message.deviceId = json.substr(pos, end - pos);
            }
        }
        
        // 查找command字段
        pos = json.find("\"command\":\"");
        if (pos != std::string::npos) {
            pos += 11; // 跳过 "command":"
            size_t end = json.find("\"", pos);
            if (end != std::string::npos) {
                message.command = json.substr(pos, end - pos);
            }
        }
        
        // 查找data字段
        pos = json.find("\"data\":\"");
        if (pos != std::string::npos) {
            pos += 8; // 跳过 "data":"
            size_t end = json.find("\"", pos);
            if (end != std::string::npos) {
                message.data = json.substr(pos, end - pos);
            }
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("JSON解析错误: " + std::string(e.what()));
    }
    
    return message;
}

void WebSocketClient::handleIncomingMessage(const std::string& message) {
    LOG_DEBUG("WebSocket收到消息: " + message);
    
    WebSocketMessage wsMsg = jsonToMessage(message);
    
    {
        std::lock_guard<std::mutex> lock(handler_mutex_);
        if (message_handler_) {
            message_handler_(wsMsg);
        }
    }
}