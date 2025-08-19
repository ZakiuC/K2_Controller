#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <cstdint>

// 命令参数类型
enum class ParameterType {
    NONE,       // 无参数
    INT8,       // 8位整数
    INT16,      // 16位整数
    INT32,      // 32位整数
    UINT8,      // 8位无符号整数
    UINT16,     // 16位无符号整数
    UINT32,     // 32位无符号整数
    FLOAT,      // 浮点数
    ENUM        // 枚举类型
};

// 命令参数信息
struct ParameterInfo {
    std::string name;           // 参数名称
    ParameterType type;         // 参数类型
    std::string description;    // 参数描述
    std::vector<std::string> enumValues; // 枚举值（仅当type为ENUM时使用）
    int64_t minValue;          // 最小值
    int64_t maxValue;          // 最大值
    size_t byteOffset;         // 在CAN数据帧中的字节偏移量
    
    ParameterInfo(const std::string& n, ParameterType t, const std::string& desc = "", 
                  int64_t min = 0, int64_t max = 0, size_t offset = 0)
        : name(n), type(t), description(desc), minValue(min), maxValue(max), byteOffset(offset) {}
    
    ParameterInfo(const std::string& n, const std::vector<std::string>& values, const std::string& desc = "", size_t offset = 0)
        : name(n), type(ParameterType::ENUM), description(desc), enumValues(values), minValue(0), maxValue(0), byteOffset(offset) {}
};

// 命令信息结构
struct CommandInfo {
    uint8_t command;                    // 命令代码
    std::string name;                   // 命令名称
    std::string description;            // 命令描述
    std::vector<ParameterInfo> parameters; // 参数列表
    
    CommandInfo(uint8_t cmd, const std::string& n, const std::string& desc = "")
        : command(cmd), name(n), description(desc) {}
    
    void addParameter(const ParameterInfo& param) {
        parameters.push_back(param);
    }
};

// 命令管理器类
class CommandManager {
public:
    CommandManager();
    
    // 获取设备类型支持的命令列表
    std::vector<CommandInfo> getSupportedCommands(const std::string& deviceType) const;
    
    // 显示命令列表并让用户选择
    CommandInfo selectCommand(const std::string& deviceType) const;
    
    // 获取命令参数输入
    std::vector<uint8_t> getCommandParameters(const CommandInfo& cmdInfo) const;
    
    // 初始化命令信息
    void initializeCommands();

private:
    std::map<std::string, std::vector<CommandInfo>> deviceCommands;
    
    // 输入验证函数
    bool validateInput(const std::string& input, const ParameterInfo& param, int64_t& value) const;
    void displayCommandList(const std::vector<CommandInfo>& commands) const;
    int64_t getIntegerInput(const ParameterInfo& param) const;
    std::string getEnumInput(const ParameterInfo& param) const;
};
