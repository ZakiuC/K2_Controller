#include "websocket_client.h"
#include "control_center.h"
#include "logger.h"

// WebSocket 命令常量定义
#define WS_TRACK_POSITION_CMD   "0x0001"
#define WS_TRACK_SPEED_CMD      "0x0002"
#define WS_BRUSH_LIFTING_CMD    "0x0003"
#define WS_EIGHT_SWITCH_CMD     "0x0004"
#define WS_PUSHFORWARD_CMD      "0x0005"
#define WS_VIDEO_STREAM_CMD     "0x0006"

WebSocketClient::WebSocketClient(const std::string& uri, unsigned short port)
    : uri_(uri), port_(port), resolver_(ioc_), ws_(ioc_) {
}

WebSocketClient::WebSocketClient(const std::string& uri, unsigned short port, const std::string& role)
    : uri_(uri), port_(port), resolver_(ioc_), ws_(ioc_), role_(role) {
}

WebSocketClient::~WebSocketClient() {
    stop();
}

void WebSocketClient::run() {
    Logger::getInstance().log(LogLevel::INFO, "WebSocket 客户端启动...");
    should_exit_ = false;
    
    main_thread_ = std::thread([this]() {
        try {
            connectForever();
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, 
                "WebSocket 程序异常: " + std::string(e.what()));
        }
    });
}

void WebSocketClient::stop() {
    should_exit_ = true;
    connected_ = false;
    
    // 关闭 WebSocket 连接
    try {
        if (ws_.is_open()) {
            ws_.close(websocket::close_code::normal);
        }
    } catch (...) {}
    
    // 等待线程结束
    if (main_thread_.joinable()) main_thread_.join();
    if (heartbeat_thread_.joinable()) heartbeat_thread_.join();
    if (receive_thread_.joinable()) receive_thread_.join();
}

bool WebSocketClient::isConnected() const {
    return connected_.load();
}

void WebSocketClient::sendMessage(const std::string& content, const std::string& reply) {
    if (!connected_) {
        Logger::getInstance().log(LogLevel::WARNING, "WebSocket 连接未建立，无法发送消息");
        return;
    }
    
    json msg = {
        {"type", "publish"}, 
        {"client_id", client_id_}, 
        {"content", content}, 
        {"reply", reply},
        {"time", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()}
    };
    
    Logger::getInstance().log(LogLevel::INFO, 
        "WebSocket [" + getFormattedTime() + "] 发送: " + msg.dump());
    
    try {
        ws_.write(net::buffer(msg.dump()));
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, 
            "WebSocket 发送失败: " + std::string(e.what()));
        connected_ = false;
    }
}

void WebSocketClient::registerCommandCallback(CommandCallback callback) {
    command_callback_ = callback;
}

void WebSocketClient::setControlCenter(ControlCenter* cc) {
    control_center_ = cc;
}

std::string WebSocketClient::getLocalIP() {
    struct ifaddrs *ifaddrs_ptr = nullptr;
    if (getifaddrs(&ifaddrs_ptr) == -1) return "127.0.0.1";

    std::string result = "127.0.0.1";
    for (struct ifaddrs *ifa = ifaddrs_ptr; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *sin = (struct sockaddr_in *)ifa->ifa_addr;
            std::string ip = inet_ntoa(sin->sin_addr);
            if (ip != "127.0.0.1") {
                result = ip;
                break;
            }
        }
    }
    freeifaddrs(ifaddrs_ptr);
    
    // 获取主机名
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        return std::string("[unknown]") + result;
    }
    hostname[sizeof(hostname) - 1] = '\0';
    return std::string("[") + hostname + "] " + result;
}

std::string WebSocketClient::getFormattedTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()) % 1000000000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(6) << (now_ns.count()/1000);
    
    return ss.str();
}

bool WebSocketClient::handshake() {
    try {
        json auth_msg = {
            {"type", "auth"},
            {"ip", getLocalIP()},
            {"role", role_}
        };
        
        ws_.write(net::buffer(auth_msg.dump()));
        Logger::getInstance().log(LogLevel::INFO, 
            "WebSocket 已发送认证信息: " + auth_msg.dump());

        beast::flat_buffer buffer;
        ws_.read(buffer);
        std::string response = beast::buffers_to_string(buffer.data());
        Logger::getInstance().log(LogLevel::DEBUG, 
            "WebSocket 服务器返回: " + response);
        
        json data = json::parse(response);

        if (data.value("status", "") == "authorized") {
            if (data.contains("client_id")) {
                client_id_ = data["client_id"].is_string()
                                 ? data["client_id"].get<std::string>()
                                 : std::to_string(data["client_id"].get<int>());
                connected_ = true;
                Logger::getInstance().log(LogLevel::INFO, 
                    "WebSocket 已授权，ID: " + client_id_ + "，角色: " + role_);
                return true;
            }
        }
        
        Logger::getInstance().log(LogLevel::ERROR, 
            "WebSocket 认证失败: " + response);
        return false;
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, 
            "WebSocket 认证异常: " + std::string(e.what()));
        return false;
    }
}

void WebSocketClient::connectForever() {
    while (!should_exit_) {
        try {
            // 解析地址
            auto const results = resolver_.resolve(uri_, std::to_string(port_));

            // 建立 TCP 连接
            net::connect(ws_.next_layer(), results.begin(), results.end());

            // WebSocket 握手
            ws_.handshake(uri_, "/");

            Logger::getInstance().log(LogLevel::INFO, "WebSocket 成功连接到服务器");

            // 认证
            if (!handshake()) {
                ws_.close(websocket::close_code::normal);
                std::this_thread::sleep_for(std::chrono::seconds(reconnect_delay_));
                continue;
            }

            // 启动接收线程
            receive_thread_ = std::thread(&WebSocketClient::receiveLoop, this);

            // 等待连接断开
            if (receive_thread_.joinable()) {
                receive_thread_.join();
            }
            
            connected_ = false;

            // 关闭连接
            try {
                ws_.close(websocket::close_code::normal);
            } catch (...) {}

        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, 
                "WebSocket 连接失败: " + std::string(e.what()) + "，" + 
                std::to_string(reconnect_delay_) + "s 后重连...");
        }

        if (!should_exit_) {
            std::this_thread::sleep_for(std::chrono::seconds(reconnect_delay_));
        }
    }
}

void WebSocketClient::receiveLoop() {
    beast::flat_buffer buffer;
    while (connected_ && !should_exit_) {
        try {
            buffer.clear();
            ws_.read(buffer);
            std::string msg = beast::buffers_to_string(buffer.data());
            
            Logger::getInstance().log(LogLevel::INFO, 
                "WebSocket [" + getFormattedTime() + "] 收到广播: " + msg);
            
            json data = json::parse(msg);
            handleReceivedData(data);
            
        } catch (const beast::system_error& se) {
            if (se.code() != websocket::error::closed) {
                Logger::getInstance().log(LogLevel::ERROR, 
                    "WebSocket 读取错误: " + std::string(se.what()));
            }
            connected_ = false;
            break;
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, 
                "WebSocket 接收异常: " + std::string(e.what()));
            connected_ = false;
            break;
        }
    }
}

void WebSocketClient::handleReceivedData(const json& data) {
    std::string contents = data.value("content", "");
    if (contents.empty()) {
        Logger::getInstance().log(LogLevel::WARNING, "WebSocket 收到空内容");
        return;
    }

    // 解析命令格式: cmd@param1#param2#param3...
    std::string cmd;
    std::string token;
    std::vector<std::string> params;
    std::stringstream ss(contents);
    
    std::getline(ss, cmd, '@');  // 获取命令
    while (std::getline(ss, token, '#')) {
        params.push_back(token);
    }

    Logger::getInstance().log(LogLevel::INFO, 
        "WebSocket 处理命令: " + cmd + " 参数数量: " + std::to_string(params.size()));

    // 处理命令
    std::string result = processCommand(cmd, params);

    // 如果需要回复
    if (data.value("reply", "") == "yes") {
        sendMessage(result, "no");
    }
}

std::string WebSocketClient::processCommand(const std::string& cmd, const std::vector<std::string>& params) {
    std::string result = "OK";

    try {
        // 如果注册了自定义回调，优先使用
        if (command_callback_) {
            return command_callback_(cmd, params);
        }

        // 默认命令处理
        if (cmd == WS_TRACK_POSITION_CMD) {
            Logger::getInstance().log(LogLevel::INFO, "WebSocket 履带电机位置模式控制");
            if (params.size() != 4) {
                result = "履带位置控制参数数量错误，需要4个参数";
            } else {
                // 这里您需要根据实际的设备控制接口来实现
                // 例如: control_center_->sendCommand("track_motor", POSITION_CMD, params);
                result = "履带位置控制执行完成";
            }
        }
        else if (cmd == WS_TRACK_SPEED_CMD) {
            Logger::getInstance().log(LogLevel::INFO, "WebSocket 履带电机速度模式控制");
            if (params.size() != 3) {
                result = "履带速度控制参数数量错误，需要3个参数";
            } else {
                // 实际设备控制实现
                result = "履带速度控制执行完成";
            }
        }
        else if (cmd == WS_BRUSH_LIFTING_CMD) {
            Logger::getInstance().log(LogLevel::INFO, "WebSocket 毛刷升降控制");
            if (params.size() != 2) {
                result = "毛刷升降控制参数数量错误，需要2个参数";
            } else {
                // 实际设备控制实现
                result = "毛刷升降控制执行完成";
            }
        }
        else if (cmd == WS_EIGHT_SWITCH_CMD) {
            Logger::getInstance().log(LogLevel::INFO, "WebSocket 8路继电器控制");
            if (params.size() != 1) {
                result = "8路继电器控制参数数量错误，需要1个参数";
            } else {
                // 实际设备控制实现
                result = "8路继电器控制执行完成";
            }
        }
        else if (cmd == WS_PUSHFORWARD_CMD) {
            Logger::getInstance().log(LogLevel::INFO, "WebSocket PWM推进电机控制");
            result = "PWM推进电机控制执行完成";
        }
        else if (cmd == WS_VIDEO_STREAM_CMD) {
            Logger::getInstance().log(LogLevel::INFO, "WebSocket 摄像头视频流控制");
            if (params.size() != 1) {
                result = "摄像头控制参数数量错误，需要1个参数";
            } else {
                // 实际摄像头控制实现
                result = "摄像头控制执行完成";
            }
        }
        else {
            Logger::getInstance().log(LogLevel::WARNING, 
                "WebSocket 未知命令: " + cmd);
            result = "未知命令: " + cmd;
        }
    } catch (const std::exception& e) {
        result = "命令执行异常: " + std::string(e.what());
        Logger::getInstance().log(LogLevel::ERROR, "WebSocket " + result);
    }

    return result;
}
