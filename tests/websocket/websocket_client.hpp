// client.cpp - 面向对象版本
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <future>
#include <chrono>
#include <memory>
#include <nlohmann/json.hpp>
#include <iomanip>
#include <sstream>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>

// Boost Beast
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

class WebSocketClient;
#define WS_TRANCK_POSTION_CMD  "0x0001"     // 履带位置模式
#define WS_TRANCK_SPEED_CMD    "0x0002"     // 履带速度模式
#define WS_BRUSH_LIFTING_CMD   "0x0003"     // 毛刷升降
#define WS_EIGHT_SWITCH_CMD    "0x0004"     // 八路继电器
#define WS_PUSHFORWARD_CMD     "0x0005"     // PWM推进电机
#define WS_VIDEO_STRAEM_CMD    "0x0006"     // 摄像头 视频流


std::string websocket_getFormattedTime(void);
void websocket_handle_recv_data(json data, WebSocketClient &ws);
void* websocket_handle_thread(void);


class WebSocketClient {
private:
    const std::string uri_;
    const unsigned short port_;
    const int ping_interval_ = 20;
    const int reconnect_delay_ = 5;

    net::io_context ioc_;
    tcp::resolver resolver_;
    websocket::stream<tcp::socket> ws_;

    std::atomic<bool> connected_{false};
    std::string client_id_;
    std::string role_ = "unknown";  // "publisher" or "subscriber"

    std::thread heartbeat_thread_;
    std::thread main_task_thread_;
    std::thread input_monitor_thread_;  // 用于监听 exit 命令
    std::thread auto_send_thread_;  // 用于监听 exit 命令

    bool should_exit_ = false;  // 用户主动退出

public:
    explicit WebSocketClient(const std::string& uri, unsigned short port)
        : uri_(uri), port_(port), resolver_(ioc_), ws_(ioc_){
            role_ = choose_role();
        }
    explicit WebSocketClient(const std::string& uri, unsigned short port, const std::string& role)
        : uri_(uri), port_(port), resolver_(ioc_), ws_(ioc_), role_(role) {}
    // 提供 getter 访问 role（因为它是 private）
    std::string get_role_(void) { return role_; }
    bool get_should_exit_(void){ return should_exit_;}
    void set_should_exit_(bool should_exit){ should_exit_ = should_exit; }
    std::atomic<bool> get_connected_(void){ return connected_.load(); }

    // 获取本机 IP
    std::string get_local_ip(void) {
    //std::string get_available_local_ip() {
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
        // 去除可能的换行或多余字符（hostname 末尾可能没 null 终止）
        hostname[sizeof(hostname) - 1] = '\0';
        // 拼接成 [hostname] ip 格式
        return std::string("[") + hostname + "] " + result;

        //return result;
    }

    // 用户选择角色
    std::string choose_role() {
        while (true) {
            std::cout << "请选择角色：\n";
            std::cout << "1. Publisher（发布者）\n";
            std::cout << "2. Subscriber（订阅者）\n";
            std::cout << "请输入选项 (1/2): ";
            std::string input;
            std::getline(std::cin, input);
            if (input == "1") return "publisher";
            if (input == "2") return "subscriber";
            std::cout << "无效选项，请重新输入。\n";
        }
    }

    // 握手认证
    bool handshake() {
        try {
            json auth_msg = {
                {"type", "auth"},
                {"ip", get_local_ip()},
                {"role", role_}
            };
            ws_.write(net::buffer(auth_msg.dump()));
            std::cout << "[Client] 已发送认证信息: " << auth_msg.dump() << std::endl;

            beast::flat_buffer buffer;
            ws_.read(buffer);
            std::string response = beast::buffers_to_string(buffer.data());
            std::cout << "[Debug] 服务器返回的原始 JSON: " << response << std::endl;
            json data = json::parse(response);

            if (data.value("status", "") == "authorized") {
                if (data.contains("client_id")) {
                    client_id_ = data["client_id"].is_string()
                                     ? data["client_id"].get<std::string>()
                                     : std::to_string(data["client_id"].get<int>());
                    connected_ = true;
                    std::cout << "[Client] 已授权，ID: " << client_id_
                              << "，角色: " << role_ << std::endl;
                    return true;
                }
            }
            std::cout << "[Auth] 认证失败: " << response << std::endl;
            return false;
        } catch (const std::exception& e) {
            std::cout << "[Auth] 认证异常: " << e.what() << std::endl;
            return false;
        }
    }

    // // 心跳线程
    // void heartbeat_loop() {
    //     while (connected_ && !should_exit_) {
    //         std::this_thread::sleep_for(std::chrono::seconds(ping_interval_));
    //         if (!connected_) break;
    //         try {
    //             ws_.ping({});
    //             std::cout << "[Heartbeat] Ping sent.\n";
    //         } catch (const std::exception& e) {
    //             std::cout << "[Heartbeat] Ping 失败: " << e.what() << std::endl;
    //             connected_ = false;
    //             break;
    //         }
    //     }
    // }
    
    void send_json_message(std::string content, std::string reply){
        json msg = {{"type", "publish"}, 
                    {"client_id", client_id_}, 
                    {"content", content}, 
                    {"reply", reply},
                    {"time", std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()
                    ).count()}
                };
        std::cout << "[" << websocket_getFormattedTime() << "] ";
        std::cout << "你（发布内容）: " << msg.dump() <<std::endl;
        try {
            ws_.write(net::buffer(msg.dump()));
        } catch (...) {
            std::cout << "\n[Send] 发送失败。\n";
            connected_ = false;
        }
    }

    /* 
        发送线程：发送命令行读取到的数据
        接收线程：接收广播收到的数据、判断是否需要回复，需要则回复。
    */
    void _base_task(){
        // 输入线程
        input_monitor_thread_ = std::thread([this](std::string reply="yes") {
            std::string content;
            beast::flat_buffer buffer;
            while (connected_ && !should_exit_) {
                // std::cout << "你（发布内容）: ";
                std::getline(std::cin, content);
                if (content == "exit") {
                    should_exit_ = true;
                    connected_ = false;
                    break;
                }
                send_json_message(content, reply);
            }
        });

        // 接收广播线程
        std::thread receive_thread([this]() {
            beast::flat_buffer buffer;
            while (connected_ && !should_exit_) {
                try {
                    buffer.clear();
                    ws_.read(buffer);
                    std::string msg = beast::buffers_to_string(buffer.data());
                    std::cout << "["<< websocket_getFormattedTime() <<"] ";
                    std::cout << " [收到广播]: " << msg << std::endl;
                    json data = json::parse(msg);
                    websocket_handle_recv_data(data, *this);
                    // if(data.value("reply", "") == "yes"){
                    //     std::string content = "666";
                    //     send_json_message(content, "no");
                    // };
                    std::cout << "你（发布内容）: " << std::endl;
                } catch (const beast::system_error& se) {
                    if (se.code() != websocket::error::closed) {
                        std::cout << " [Receive] 读取错误: " << se.what() << std::endl;
                    }
                    connected_ = false;
                    break;
                } catch (const std::exception& e) {
                    std::cout << "\n[Receive] 异常: " << e.what() << std::endl;
                    connected_ = false;
                    break;
                }
            }
        });

        if (input_monitor_thread_.joinable()) input_monitor_thread_.join();
        if (receive_thread.joinable()) receive_thread.join();
        if (role_ == "subscriber"){
            if (auto_send_thread_.joinable()) auto_send_thread_.join();
                std::cout << "subscriber 以释放 auto_send_thread_"<< std::endl;
        }
    }

    // 发布者任务
    void publisher_task() {       
        std::cout << "[Publisher] 已连接，准备发布消息。输入 'exit' 退出。\n";
        _base_task();
    }

    // 订阅者任务
    void subscriber_task() {
        std::cout << "[Subscriber] 已连接，准备发布消息。输入 'exit' 退出。\n";
        _base_task();
    }

    // 主连接逻辑（自动重连）
    void connect_forever() {
        while (!should_exit_) {
            try {
                // 解析地址
                auto const results = resolver_.resolve(uri_, std::to_string(port_));

                // 建立 TCP 连接
                net::connect(ws_.next_layer(), results.begin(), results.end());

                // WebSocket 握手
                ws_.handshake(uri_, "/");

                std::cout << "[Client] 成功连接到服务器。\n";

                // 认证
                if (!handshake()) {
                    ws_.close(websocket::close_code::normal);
                    std::this_thread::sleep_for(std::chrono::seconds(reconnect_delay_));
                    continue;
                }

                // 启动心跳
                //heartbeat_thread_ = std::thread(&WebSocketClient::heartbeat_loop, this);

                // 启动主任务
                if (role_ == "publisher") {
                    main_task_thread_ = std::thread(&WebSocketClient::publisher_task, this);
                } else {
                    main_task_thread_ = std::thread(&WebSocketClient::subscriber_task, this);
                #if 0
                    auto_send_thread_ = std::thread([this](std::string reply="no") {
                        while(1){
                            std::string content = "0x0000@0909#1233.23#192768"; // 0x0001 包头，命令
                            send_json_message(content, reply);
                            std::this_thread::sleep_for(std::chrono::seconds(3));
                        }
                            // 毫秒级延时
                            // std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    });
                #endif
                }

                // 等待任务结束
                if (main_task_thread_.joinable()) main_task_thread_.join();
                connected_ = false;

                // 停止心跳
                //if (heartbeat_thread_.joinable()) heartbeat_thread_.join();

                // 关闭连接
                try {
                    ws_.close(websocket::close_code::normal);
                } catch (...) {}

            } catch (const std::exception& e) {
                std::cout << "[Connection] 连接失败: " << e.what()
                          << "，" << reconnect_delay_ << "s 后重连...\n";
            }

            if (!should_exit_) {
                std::this_thread::sleep_for(std::chrono::seconds(reconnect_delay_));
            }
        }
    }

    // 启动客户端
    void run() {
        std::cout << "WebSocket 客户端启动...\n";
        try {
            connect_forever();
        } catch (const std::exception& e) {
            std::cerr << "程序异常: " << e.what() << std::endl;
        }
    }
};


class Publisher : public WebSocketClient{
private:
public:
    Publisher(const std::string& uri, unsigned short port):WebSocketClient(uri, port ,"publisher") {}
    ~Publisher(void){};
};


class Subscriber : public WebSocketClient{
private:
public:
    Subscriber(const std::string& uri, unsigned short port):WebSocketClient(uri, port ,"subscriber") {}
    ~Subscriber(void){
    };
};



// ================== 主函数 ==================
// int main() {
//     WebSocketClient client("127.0.0.1", 8765);
//     client.run();
//     return 0;
// }