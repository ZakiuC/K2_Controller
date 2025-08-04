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
#include <iomanip>
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
    std::string can_interface;   // CAN接口名称
    int motor_id;                // 电机ID (1-32)
    
    // 默认配置：正向开启继电器，CAN接口为can1，电机ID为4
    MoveConfig() : positive_relay_on(true), can_interface("can1"), motor_id(4) {}
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
    file << "# CAN接口名称\n";
    file << "can_interface=" << config.can_interface << "\n";
    file << "# 电机ID (1-32)\n";
    file << "motor_id=" << config.motor_id << "\n";
    
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
    
    // 临时配置，用于验证
    MoveConfig temp_config;
    bool has_error = false;
    
    std::string line;
    while (std::getline(file, line)) {
        // 跳过注释和空行
        if (line.empty() || line[0] == '#') continue;
        
        size_t pos = line.find('=');
        if (pos == std::string::npos) continue;
        
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        
        try {
            if (key == "positive_relay_on") {
                temp_config.positive_relay_on = (std::stoi(value) != 0);
            }
            else if (key == "can_interface") {
                if (value.empty()) {
                    std::cout << "警告: CAN接口名称为空，使用默认值 can1" << std::endl;
                    has_error = true;
                } else {
                    temp_config.can_interface = value;
                }
            }
            else if (key == "motor_id") {
                int motor_id = std::stoi(value);
                if (motor_id < 1 || motor_id > 32) {
                    std::cout << "警告: 电机ID " << motor_id << " 超出范围(1-32)，使用默认值 4" << std::endl;
                    has_error = true;
                } else {
                    temp_config.motor_id = motor_id;
                }
            }
        } catch (const std::exception& e) {
            std::cout << "警告: 配置项 " << key << " 的值 '" << value << "' 无效，使用默认值" << std::endl;
            has_error = true;
        }
    }
    
    file.close();
    
    if (has_error) {
        std::cout << "配置文件包含无效数据，已使用默认值替换，正在更新配置文件..." << std::endl;
        config = temp_config;
        save_config(config);
    } else {
        config = temp_config;
        std::cout << "配置文件加载成功" << std::endl;
    }
    
    return true;
}

// 显示当前配置
void show_config(const MoveConfig& config) {
    std::cout << "当前Move配置:" << std::endl;
    std::cout << "  正向速度时继电器: " << (config.positive_relay_on ? "开" : "关") << std::endl;
    std::cout << "  负向速度时继电器: " << (config.positive_relay_on ? "关" : "开") << std::endl;
    std::cout << "  零速度时: 不改变继电器状态" << std::endl;
    std::cout << "  CAN接口: " << config.can_interface << std::endl;
    std::cout << "  电机ID: " << config.motor_id << std::endl;
}

// 命令补全相关函数
static std::vector<std::string> get_all_commands()
{
    static std::vector<std::string> commands = {
        "move", "motor", "relay", "echo", "config", "exit", "quit", "help"
    };
    return commands;
}

static std::vector<std::string> get_motor_subcommands()
{
    static std::vector<std::string> subcommands = {
        "stop", "run", "close", "speed"
    };
    return subcommands;
}

static std::vector<std::string> get_relay_subcommands()
{
    static std::vector<std::string> subcommands = {
        "on", "off"
    };
    return subcommands;
}

static std::vector<std::string> get_config_subcommands()
{
    static std::vector<std::string> subcommands = {
        "show", "set", "reset"
    };
    return subcommands;
}

static std::vector<std::string> get_config_set_params()
{
    static std::vector<std::string> params = {
        "positive_relay", "can_interface", "motor_id"
    };
    return params;
}

static std::vector<std::string> get_boolean_values()
{
    static std::vector<std::string> values = {
        "on", "off", "true", "false", "1", "0"
    };
    return values;
}

// 获取自动补全候选项
static std::vector<std::string> get_completions(const std::string& line, int cursor_pos)
{
    std::vector<std::string> completions;
    
    // 提取光标位置之前的内容
    std::string text_before_cursor = line.substr(0, cursor_pos);
    
    // 分割成单词
    std::istringstream iss(text_before_cursor);
    std::vector<std::string> words;
    std::string word;
    while (iss >> word) {
        words.push_back(to_lower(word));
    }
    
    // 检查最后一个字符是否为空格，确定是否需要新单词
    bool need_new_word = text_before_cursor.empty() || text_before_cursor.back() == ' ';
    
    if (words.empty() || (words.size() == 1 && !need_new_word))
    {
        // 补全主命令
        std::string prefix = words.empty() ? "" : words[0];
        for (const auto& cmd : get_all_commands()) {
            if (cmd.find(prefix) == 0) {
                completions.push_back(cmd);
            }
        }
    }
    else if (words.size() >= 1)
    {
        std::string main_cmd = words[0];
        
        if (main_cmd == "motor")
        {
            if (words.size() == 1 && need_new_word)
            {
                // 补全motor子命令
                completions = get_motor_subcommands();
            }
            else if (words.size() == 2 && !need_new_word)
            {
                // 补全motor子命令（部分输入）
                std::string prefix = words[1];
                for (const auto& subcmd : get_motor_subcommands()) {
                    if (subcmd.find(prefix) == 0) {
                        completions.push_back(subcmd);
                    }
                }
            }
        }
        else if (main_cmd == "relay")
        {
            if (words.size() == 1 && need_new_word)
            {
                completions = get_relay_subcommands();
            }
            else if (words.size() == 2 && !need_new_word)
            {
                std::string prefix = words[1];
                for (const auto& subcmd : get_relay_subcommands()) {
                    if (subcmd.find(prefix) == 0) {
                        completions.push_back(subcmd);
                    }
                }
            }
        }
        else if (main_cmd == "config")
        {
            if (words.size() == 1 && need_new_word)
            {
                completions = get_config_subcommands();
            }
            else if (words.size() == 2 && !need_new_word)
            {
                std::string prefix = words[1];
                for (const auto& subcmd : get_config_subcommands()) {
                    if (subcmd.find(prefix) == 0) {
                        completions.push_back(subcmd);
                    }
                }
            }
            else if (words.size() == 2 && words[1] == "set" && need_new_word)
            {
                completions = get_config_set_params();
            }
            else if (words.size() == 3 && words[1] == "set" && !need_new_word)
            {
                std::string prefix = words[2];
                for (const auto& param : get_config_set_params()) {
                    if (param.find(prefix) == 0) {
                        completions.push_back(param);
                    }
                }
            }
            else if (words.size() == 3 && words[1] == "set" && words[2] == "positive_relay" && need_new_word)
            {
                completions = get_boolean_values();
            }
            else if (words.size() == 4 && words[1] == "set" && words[2] == "positive_relay" && !need_new_word)
            {
                std::string prefix = words[3];
                for (const auto& value : get_boolean_values()) {
                    if (value.find(prefix) == 0) {
                        completions.push_back(value);
                    }
                }
            }
        }
    }
    
    return completions;
}

// 查找当前单词的开始位置
static int find_word_start(const std::string& line, int cursor_pos)
{
    int start = cursor_pos;
    while (start > 0 && line[start - 1] != ' ') {
        start--;
    }
    return start;
}

// 查找当前单词的结束位置
static int find_word_end(const std::string& line, int cursor_pos)
{
    int end = cursor_pos;
    while (end < (int)line.length() && line[end] != ' ') {
        end++;
    }
    return end;
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
        else if (ch == '\t') // Tab键自动补全
        {
            std::vector<std::string> completions = get_completions(current_line, cursor_pos);
            
            if (completions.size() == 1)
            {
                // 只有一个匹配项，直接补全
                int word_start = find_word_start(current_line, cursor_pos);
                int word_end = find_word_end(current_line, cursor_pos);
                
                // 替换当前单词
                std::string before = current_line.substr(0, word_start);
                std::string after = current_line.substr(word_end);
                current_line = before + completions[0] + " " + after;
                cursor_pos = before.length() + completions[0].length() + 1;
                
                // 重新显示行
                std::cout << "\r\033[K请输入命令：" << current_line << std::flush;
                // 移动光标到正确位置
                if (cursor_pos < (int)current_line.length())
                {
                    std::cout << "\033[" << (current_line.length() - cursor_pos) << "D" << std::flush;
                }
            }
            else if (completions.size() > 1)
            {
                // 多个匹配项，显示所有可能的补全
                std::cout << std::endl;
                std::cout << "可能的补全:" << std::endl;
                
                // 计算最大长度用于格式化
                size_t max_len = 0;
                for (const auto& comp : completions) {
                    max_len = std::max(max_len, comp.length());
                }
                
                // 每行显示多个项目
                const int items_per_line = 4;
                for (size_t i = 0; i < completions.size(); ++i) {
                    std::cout << std::setw(max_len + 2) << std::left << completions[i];
                    if ((i + 1) % items_per_line == 0 || i == completions.size() - 1) {
                        std::cout << std::endl;
                    }
                }
                
                // 找到公共前缀并补全
                if (!completions.empty()) {
                    std::string common_prefix = completions[0];
                    for (size_t i = 1; i < completions.size(); ++i) {
                        size_t j = 0;
                        while (j < common_prefix.length() && j < completions[i].length() && 
                               common_prefix[j] == completions[i][j]) {
                            j++;
                        }
                        common_prefix = common_prefix.substr(0, j);
                    }
                    
                    // 如果有公共前缀且比当前单词长，则补全到公共前缀
                    int word_start = find_word_start(current_line, cursor_pos);
                    std::string current_word = current_line.substr(word_start, cursor_pos - word_start);
                    
                    if (common_prefix.length() > current_word.length()) {
                        int word_end = find_word_end(current_line, cursor_pos);
                        std::string before = current_line.substr(0, word_start);
                        std::string after = current_line.substr(word_end);
                        current_line = before + common_prefix + after;
                        cursor_pos = before.length() + common_prefix.length();
                    }
                }
                
                // 重新显示提示符和当前行
                std::cout << "请输入命令：" << current_line << std::flush;
                // 移动光标到正确位置
                if (cursor_pos < (int)current_line.length())
                {
                    std::cout << "\033[" << (current_line.length() - cursor_pos) << "D" << std::flush;
                }
            }
            // 如果没有匹配项，忽略Tab键
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

// 全局对象声明（将在main函数中初始化）
CANInterface* can_interface = nullptr;
LH08Controller lh08;
MotorController* motor = nullptr;
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
    
    motor->set_speed(speed);
}

int main()
{
    // 加载配置文件
    if (!load_config(g_move_config)) {
        std::cerr << "配置文件加载失败，使用默认配置" << std::endl;
    }

    // 使用配置文件中的值初始化CAN接口和电机控制器
    can_interface = new CANInterface(g_move_config.can_interface);
    motor = new MotorController(*can_interface, g_move_config.motor_id);

    if (!can_interface->init())
    {
        std::cerr << "CAN 接口初始化失败" << std::endl;
        delete can_interface;
        delete motor;
        return 1;
    }
    // 命令历史记录
    std::vector<std::string> command_history;
    int history_index = 0;

    if (!lh08.openRS232Port())
    {
        std::cerr << "串口打开失败" << std::endl;
        delete can_interface;
        delete motor;
        return 1;
    }

    motor->enable_motor();

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
                motor->stop_motor();
            }
            else if (motor_cmd == "run")
            {
                motor->enable_motor();
            }
            else if (motor_cmd == "close")
            {
                motor->disable_motor();
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
                motor->set_speed(speed);
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
                else if (param == "can_interface")
                {
                    std::string value;
                    if (stream >> value)
                    {
                        if (value.empty())
                        {
                            std::cout << "CAN接口名称不能为空" << std::endl;
                            continue;
                        }
                        g_move_config.can_interface = value;
                        std::cout << "CAN接口设置为: " << value << std::endl;
                        std::cout << "注意: 需要重启程序使CAN接口配置生效" << std::endl;
                        if (save_config(g_move_config))
                        {
                            std::cout << "配置已保存" << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "请提供CAN接口名称 (例如: can0, can1)" << std::endl;
                    }
                }
                else if (param == "motor_id")
                {
                    int value;
                    if (stream >> value)
                    {
                        if (value < 1 || value > 32)
                        {
                            std::cout << "电机ID必须在1-32范围内" << std::endl;
                            continue;
                        }
                        g_move_config.motor_id = value;
                        std::cout << "电机ID设置为: " << value << std::endl;
                        std::cout << "注意: 需要重启程序使电机ID配置生效" << std::endl;
                        if (save_config(g_move_config))
                        {
                            std::cout << "配置已保存" << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "请提供有效的电机ID (1-32)" << std::endl;
                    }
                }
                else
                {
                    std::cout << "未知配置参数: " << param << std::endl;
                    std::cout << "可用参数: positive_relay, can_interface, motor_id" << std::endl;
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
            motor->set_speed(0);
            motor->disable_motor();
            delete can_interface;
            delete motor;
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
            std::cout << "  config set can_interface <name> - 设置CAN接口名称" << std::endl;
            std::cout << "  config set motor_id <id> - 设置电机ID (1-32)" << std::endl;
            std::cout << "  config reset - 重置为默认配置" << std::endl;
            std::cout << "  echo <message> - 回显消息" << std::endl;
            std::cout << "  exit/quit - 退出程序" << std::endl;
            std::cout << std::endl;
            std::cout << "配置说明:" << std::endl;
            std::cout << "  positive_relay: 正向移动时继电器状态 (on/off)" << std::endl;
            std::cout << "  can_interface: CAN接口名称 (例如: can0, can1)" << std::endl;
            std::cout << "  motor_id: 电机ID，范围1-32" << std::endl;
            std::cout << "  负向移动时继电器状态自动为相反值" << std::endl;
            std::cout << "  零速度时不改变继电器状态" << std::endl;
            std::cout << "  注意: CAN接口和电机ID修改后需要重启程序生效" << std::endl;
            std::cout << std::endl;
            std::cout << "快捷键:" << std::endl;
            std::cout << "  ↑/↓ 方向键 - 浏览命令历史记录" << std::endl;
            std::cout << "  ←/→ 方向键 - 移动光标位置" << std::endl;
            std::cout << "  Tab键 - 自动补全命令和参数" << std::endl;
            std::cout << "  Backspace - 删除字符" << std::endl;
        }
        else
        {
            std::cout << "未知命令." << std::endl;
        }
    }

    return 0;
}
