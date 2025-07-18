#include <iostream>
#include <unistd.h>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <vector>
#include <termios.h>
#include <cstdio>
#include <fstream>
#include <map>
#include "CANInterface.h"
#include "MotorController.h"
#include "LH08Controller.h"

// 将字符串转换为小写
static std::string to_lower(const std::string &str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });
    return result;
}

// 配置结构体
struct MoveConfig {
    bool positive_relay_on;      // 正向速度时继电器是否开启 (true=开, false=关)
    
    // 默认配置：正向开启继电器，负向关闭继电器
    MoveConfig() : positive_relay_on(true) {}
};

// 全局配置变量
MoveConfig g_move_config;
const std::string CONFIG_FILE = "move_config.txt";

// 保存配置到文件
bool save_config(const MoveConfig& config) {
    std::ofstream file(CONFIG_FILE);
    if (!file.is_open()) {
        std::cerr << "无法创建配置文件: " << CONFIG_FILE << std::endl;
        return false;
    }
    
    file << "# Move配置文件\n";
    file << "# 正向速度时继电器状态 (1=开, 0=关)\n";
    file << "# 负向速度时继电器状态自动为相反值\n";
    file << "# 零速度时不改变继电器状态\n";
    file << "positive_relay_on=" << (config.positive_relay_on ? 1 : 0) << "\n";
    
    file.close();
    return true;
}

// 从文件加载配置
bool load_config(MoveConfig& config) {
    std::ifstream file(CONFIG_FILE);
    if (!file.is_open()) {
        std::cout << "配置文件不存在，使用默认配置并创建配置文件..." << std::endl;
        config = MoveConfig(); // 使用默认配置
        return save_config(config);
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // 跳过注释和空行
        if (line.empty() || line[0] == '#') continue;
        
        size_t pos = line.find('=');
        if (pos == std::string::npos) continue;
        
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        
        if (key == "positive_relay_on") {
            config.positive_relay_on = (std::stoi(value) != 0);
        }
    }
    
    file.close();
    std::cout << "配置文件加载成功" << std::endl;
    return true;
}

// 显示当前配置
void show_config(const MoveConfig& config) {
    std::cout << "当前Move配置:" << std::endl;
    std::cout << "  正向速度时继电器: " << (config.positive_relay_on ? "开" : "关") << std::endl;
    std::cout << "  负向速度时继电器: " << (config.positive_relay_on ? "关" : "开") << std::endl;
    std::cout << "  零速度时: 不改变继电器状态" << std::endl;
}

// 终端设置结构
static struct termios old_tio, new_tio;

// 设置终端为原始模式
static void set_raw_mode()
{
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    new_tio.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

// 恢复终端模式
static void restore_terminal()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
}

// 读取带历史记录的命令行
static std::string read_command_with_history(std::vector<std::string> &history, int &history_index)
{
    std::string current_line;
    std::string display_line;
    int cursor_pos = 0;

    std::cout << "请输入命令：" << std::flush;

    set_raw_mode();

    while (true)
    {
        int ch = getchar();

        if (ch == '\n' || ch == '\r')
        {
            std::cout << std::endl;
            break;
        }
        else if (ch == 27) // ESC sequence
        {
            ch = getchar();
            if (ch == '[')
            {
                ch = getchar();
                if (ch == 'A') // 上箭头
                {
                    if (!history.empty() && history_index > 0)
                    {
                        history_index--;
                        // 清除当前行
                        std::cout << "\r\033[K请输入命令：" << std::flush;
                        current_line = history[history_index];
                        cursor_pos = current_line.length();
                        std::cout << current_line << std::flush;
                    }
                }
                else if (ch == 'B') // 下箭头
                {
                    if (!history.empty())
                    {
                        if (history_index < (int)history.size() - 1)
                        {
                            history_index++;
                            // 清除当前行
                            std::cout << "\r\033[K请输入命令：" << std::flush;
                            current_line = history[history_index];
                            cursor_pos = current_line.length();
                            std::cout << current_line << std::flush;
                        }
                        else if (history_index == (int)history.size() - 1)
                        {
                            history_index = history.size();
                            // 清除当前行，显示空行
                            std::cout << "\r\033[K请输入命令：" << std::flush;
                            current_line.clear();
                            cursor_pos = 0;
                        }
                    }
                }
                else if (ch == 'C') // 右箭头
                {
                    if (cursor_pos < (int)current_line.length())
                    {
                        cursor_pos++;
                        std::cout << "\033[C" << std::flush; // 光标右移
                    }
                }
                else if (ch == 'D') // 左箭头
                {
                    if (cursor_pos > 0)
                    {
                        cursor_pos--;
                        std::cout << "\033[D" << std::flush; // 光标左移
                    }
                }
            }
        }
        else if (ch == 127 || ch == 8) // Backspace
        {
            if (cursor_pos > 0)
            {
                current_line.erase(cursor_pos - 1, 1);
                cursor_pos--;
                // 重新显示行
                std::cout << "\r\033[K请输入命令：" << current_line << std::flush;
                // 移动光标到正确位置
                if (cursor_pos < (int)current_line.length())
                {
                    std::cout << "\033[" << (current_line.length() - cursor_pos) << "D" << std::flush;
                }
            }
        }
        else if (ch >= 32 && ch <= 126) // 可打印字符
        {
            current_line.insert(cursor_pos, 1, ch);
            cursor_pos++;
            // 重新显示行
            std::cout << "\r\033[K请输入命令：" << current_line << std::flush;
            // 移动光标到正确位置
            if (cursor_pos < (int)current_line.length())
            {
                std::cout << "\033[" << (current_line.length() - cursor_pos) << "D" << std::flush;
            }
        }
    }

    restore_terminal();
    return current_line;
}

static void control_relay(LH08Controller &lh08, bool state)
{
    if (state)
    {
        std::cout << "继电器开." << std::endl;
        lh08.setStatus(1, 0x02);
    }
    else
    {
        std::cout << "继电器关." << std::endl;
        lh08.setStatus(1, 0x01);
    }
}

const std::string CAN_INTERFACE = "can1";
const int MOTOR_ID = 3; // 电机ID (1-32)
                        // 初始化 CAN 接口
CANInterface can_interface(CAN_INTERFACE);

LH08Controller lh08;
MotorController motor(can_interface, MOTOR_ID);
std::string command;

void move(int32_t speed)
{
    if (speed > 0)
    {
        // 正向速度
        bool relay_state = g_move_config.positive_relay_on;
        // std::cout << "正向移动，速度: " << speed << "，继电器: " << (relay_state ? "开" : "关") << std::endl;
        control_relay(lh08, relay_state);
    }
    else if (speed < 0)
    {
        // 负向速度，继电器状态与正向相反
        bool relay_state = !g_move_config.positive_relay_on;
        // std::cout << "负向移动，速度: " << speed << "，继电器: " << (relay_state ? "开" : "关") << std::endl;
        control_relay(lh08, relay_state);
    }
    else
    {
        // 零速度，不改变继电器状态
        // std::cout << "零速/停止，速度: " << speed << "，继电器状态不变" << std::endl;
    }
    
    motor.set_speed(speed);
}

int main()
{
    // 加载配置文件
    if (!load_config(g_move_config)) {
        std::cerr << "配置文件加载失败，使用默认配置" << std::endl;
    }

    if (!can_interface.init())
    {
        std::cerr << "CAN 接口初始化失败" << std::endl;
        return 1;
    }
    // 命令历史记录
    std::vector<std::string> command_history;
    int history_index = 0;

    if (!lh08.openRS232Port())
    {
        std::cerr << "串口打开失败" << std::endl;
        return 1;
    }

    motor.enable_motor();

    std::cout << "Controller 已启动" << std::endl;
    std::cout << "输入 'help' 查看可用命令" << std::endl;

    while (1)
    {
        command = read_command_with_history(command_history, history_index);

        // 如果命令不为空且与历史记录中最后一个不同，则添加到历史记录
        if (!command.empty() && (command_history.empty() || command_history.back() != command))
        {
            command_history.push_back(command);
            // 限制历史记录大小
            if (command_history.size() > 100)
            {
                command_history.erase(command_history.begin());
            }
        }
        // 重置历史索引到最新位置
        history_index = command_history.size();

        std::istringstream stream(command);
        std::string cmd;
        stream >> cmd;
        cmd = to_lower(cmd); // 转换为小写

        if (cmd == "move")
        {
            int32_t speed;
            if (!(stream >> speed))
            {
                std::cout << "错误: 请输入有效的速度数值 (例如: move 100)" << std::endl;
                continue;
            }
            
            // 检查是否还有多余的非数字字符
            std::string remaining;
            if (stream >> remaining)
            {
                std::cout << "错误: 速度参数包含无效字符，请输入纯数字 (例如: move 100)" << std::endl;
                continue;
            }
            
            move(speed);
        }
        else if (cmd == "relay")
        {
            std::string relay_cmd;
            stream >> relay_cmd;
            relay_cmd = to_lower(relay_cmd); // 转换为小写

            if (relay_cmd == "on")
            {
                control_relay(lh08, true);
            }
            else if (relay_cmd == "off")
            {
                control_relay(lh08, false);
            }
            else
            {
                std::cout << "未知继电器命令: " << relay_cmd << std::endl;
            }
        }
        else if (cmd == "motor")
        {
            std::string motor_cmd;
            stream >> motor_cmd;
            motor_cmd = to_lower(motor_cmd); // 转换为小写

            if (motor_cmd == "stop")
            {
                motor.stop_motor();
            }
            else if (motor_cmd == "run")
            {
                motor.enable_motor();
            }
            else if (motor_cmd == "close")
            {
                motor.disable_motor();
            }
            else if (motor_cmd == "speed")
            {
                int32_t speed;
                if (!(stream >> speed))
                {
                    std::cout << "错误: 请输入有效的速度数值 (例如: motor speed 100)" << std::endl;
                    continue;
                }
                
                // 检查是否还有多余的非数字字符
                std::string remaining;
                if (stream >> remaining)
                {
                    std::cout << "错误: 速度参数包含无效字符，请输入纯数字 (例如: motor speed 100)" << std::endl;
                    continue;
                }
                
                std::cout << "设置电机速度为: " << speed << std::endl;
                motor.set_speed(speed);
            }
            else
            {
                std::cout << "未知电机命令: " << motor_cmd << std::endl;
            }
        }
        else if (cmd == "echo")
        {
            std::string echo;
            stream >> echo;
            std::cout << "回显: " << echo << std::endl;
        }
        else if (cmd == "config")
        {
            std::string config_cmd;
            stream >> config_cmd;
            config_cmd = to_lower(config_cmd);
            
            if (config_cmd == "show")
            {
                show_config(g_move_config);
            }
            else if (config_cmd == "set")
            {
                std::string param;
                stream >> param;
                param = to_lower(param);
                
                if (param == "positive_relay")
                {
                    std::string value;
                    if (stream >> value)
                    {
                        value = to_lower(value);
                        if (value == "on" || value == "1" || value == "true")
                        {
                            g_move_config.positive_relay_on = true;
                            std::cout << "正向时继电器设置为: 开 (负向时自动为: 关)" << std::endl;
                        }
                        else if (value == "off" || value == "0" || value == "false")
                        {
                            g_move_config.positive_relay_on = false;
                            std::cout << "正向时继电器设置为: 关 (负向时自动为: 开)" << std::endl;
                        }
                        else
                        {
                            std::cout << "请使用 on/off, 1/0, 或 true/false" << std::endl;
                            continue;
                        }
                        if (save_config(g_move_config))
                        {
                            std::cout << "配置已保存" << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "请提供有效的值 (on/off)" << std::endl;
                    }
                }
                else
                {
                    std::cout << "未知配置参数: " << param << std::endl;
                    std::cout << "可用参数: positive_relay" << std::endl;
                }
            }
            else if (config_cmd == "reset")
            {
                g_move_config = MoveConfig(); // 重置为默认配置
                if (save_config(g_move_config))
                {
                    std::cout << "配置已重置为默认值并保存" << std::endl;
                    show_config(g_move_config);
                }
            }
            else
            {
                std::cout << "未知配置命令: " << config_cmd << std::endl;
                std::cout << "可用命令: show, set, reset" << std::endl;
            }
        }
        else if (cmd == "exit" || cmd == "quit")
        {
            motor.set_speed(0);
            motor.disable_motor();
            std::cout << "退出程序." << std::endl;
            break;
        }
        else if (cmd == "help")
        {
            std::cout << "可用命令 (不区分大小写):" << std::endl;
            std::cout << "  move <speed> - 根据配置移动电机并控制继电器" << std::endl;
            std::cout << "  motor stop - 停止电机" << std::endl;
            std::cout << "  motor run - 启动电机" << std::endl;
            std::cout << "  motor close - 关闭电机" << std::endl;
            std::cout << "  motor speed <value> - 设置电机速度" << std::endl;
            std::cout << "  relay on - 打开继电器" << std::endl;
            std::cout << "  relay off - 关闭继电器" << std::endl;
            std::cout << "  config show - 显示当前配置" << std::endl;
            std::cout << "  config set positive_relay <on/off> - 设置正向时继电器状态" << std::endl;
            std::cout << "  config reset - 重置为默认配置" << std::endl;
            std::cout << "  echo <message> - 回显消息" << std::endl;
            std::cout << "  exit/quit - 退出程序" << std::endl;
            std::cout << std::endl;
            std::cout << "配置说明:" << std::endl;
            std::cout << "  positive_relay: 正向移动时继电器状态 (on/off)" << std::endl;
            std::cout << "  负向移动时继电器状态自动为相反值" << std::endl;
            std::cout << "  零速度时不改变继电器状态" << std::endl;
            std::cout << std::endl;
            std::cout << "快捷键:" << std::endl;
            std::cout << "  ↑/↓ 方向键 - 浏览命令历史记录" << std::endl;
            std::cout << "  ←/→ 方向键 - 移动光标位置" << std::endl;
            std::cout << "  Backspace - 删除字符" << std::endl;
        }
        else
        {
            std::cout << "未知命令." << std::endl;
        }
    }

    return 0;
}
