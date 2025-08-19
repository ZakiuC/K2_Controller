#include "main.h"

// 服务器 ip 端口
// #define SERVER_IP       "192.168.0.130"
// #define SERVER_PORT     8765

#define SERVER_IP       "115.190.42.239"
#define SERVER_PORT     8080

extern sysdev sys;

void* websocket_handle_thread(void)
{
    Subscriber Sub(SERVER_IP, SERVER_PORT);
    std::cout << Sub.get_role_() << std::endl;
    Sub.run();
    return NULL; 
}


std::string websocket_getFormattedTime(void) {
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();
    
    // 转换为time_t以获取日期信息
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    
    // 提取纳秒级部分
    auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()) % 1000000000;

    // 格式化日期时间为字符串
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(6) << (now_ns.count()/1000); // 显示微秒级，即纳秒除以1000
    
    return ss.str();
}

void websocket_handle_recv_data(json data, WebSocketClient &ws){
    std::string contents = data.value("content", "");  // 获取字符串
    if (contents.empty()) {
        std::cout << "Received empty content\n";
        return;
    }
    std::string cmd;
    std::string token;  //解析出来的临时 单个字符串
    std::string content = "OK";
    std::vector<std::string> tokens;    // tokens[0]、tokens[1]、tokens[2]
    std::stringstream ss(contents);
    std::getline(ss, cmd, '@');  // 取出 "0x0001"
    while (std::getline(ss, token, '#')) {
        tokens.push_back(token);
    }

    // 使用 if-else 替代 switch
    if (cmd == WS_TRANCK_POSTION_CMD) {     // 履带电机 位置模式 控制
        std::cout << "履带电机 位置模式 控制..." << std::endl;
        if(tokens.size() != 4)
            content = "in set_angle_speed_cur, num of param is not 4.";
        else{
            sys.motorcan->set_angle_speed_cur(std::atof(tokens[0].c_str()), std::stoi(tokens[1]), std::stoi(tokens[2]), std::stoi(tokens[3]));
        }
        if(data.value("reply", "") == "yes"){
            ws.send_json_message(content, "no");
        };
    }
    else if (cmd == WS_TRANCK_SPEED_CMD) {  // 履带电机 速度模式 控制
        std::cout << "履带电机 速度模式 控制..." << std::endl;
        if(tokens.size() != 3)
            content = "in set_speed_cur_spdmode, num of param is not 3.";
        else{
            sys.motorcan->set_speed_cur_spdmode(std::atof(tokens[0].c_str()), std::stoi(tokens[1]), std::stoi(tokens[2]));
        }
        if(data.value("reply", "") == "yes"){
            ws.send_json_message(content, "no");
        };
    }
    else if (cmd == WS_BRUSH_LIFTING_CMD) {
        std::cout << "毛刷升降控制..." << std::endl;
        if(tokens.size() != 2){
            content = "in send_pos_cmd, num of param is not 2.";
        }else{
            sys.BrushMotor[0]->send_pos_cmd(500, std::stoi(tokens[0]));
            sys.BrushMotor[1]->send_pos_cmd(500, std::stoi(tokens[1]));
        }
        if(data.value("reply", "") == "yes"){
            ws.send_json_message(content, "no");
        };
    }
    else if (cmd == WS_EIGHT_SWITCH_CMD) {  // 8路继电器
        std::cout << "毛刷升降控制..." << std::endl;
        if(tokens.size() != 1){
            content = "in setStatus08, num of param is not 1.";
        }else{
            sys.lh08->setStatus08(tokens[0]);
        }
        if(data.value("reply", "") == "yes"){
            ws.send_json_message(content, "no");
        };
    }
    else if (cmd == WS_PUSHFORWARD_CMD) {   // PWM推进电机
        std::cout << "0x0005视频流控制" << std::endl;
        
    }
    else if (cmd == WS_VIDEO_STRAEM_CMD) {  // 摄像头推流
        std::cout << "0x0006控制" << std::endl;
        if(tokens.size() != 1){
            content = "in handleCamera, num of param is not 1.";
        }else{
            handleCamera(std::stoi(tokens[0])); // "1" 则开启视频流， "0" 则关闭视频流
        }
        if(data.value("reply", "") == "yes"){
            ws.send_json_message(content, "no");
        };
    }
    else {
        std::cout << "no this cmd: " << cmd << std::endl;
    }
}