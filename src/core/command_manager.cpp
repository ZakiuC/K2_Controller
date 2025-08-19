#include "command_manager.h"
#include "can_device.h"
#include "logger.h"
#include <iostream>
#include <sstream>
#include <limits>
#include <algorithm>

CommandManager::CommandManager()
{
    initializeCommands();
}

void CommandManager::initializeCommands()
{
    std::vector<CommandInfo> canCommands;

    // 基本控制命令
    CommandInfo disableCmd(MOTOR_DISABLE, "电机禁用", "禁用电机运行");
    canCommands.push_back(disableCmd);

    CommandInfo stopCmd(MOTOR_STOP, "电机停止", "停止电机运行");
    canCommands.push_back(stopCmd);

    CommandInfo runCmd(MOTOR_RUN, "电机运行", "启动电机运行");
    canCommands.push_back(runCmd);

    // 抱闸器控制
    CommandInfo syncBrakeCmd(MOTOR_SYNC_BRAKE, "同步抱闸器", "控制抱闸器状态");
    syncBrakeCmd.addParameter(ParameterInfo("抱闸器状态",
                                            {"抱闸器断电(刹车启动)", "抱闸器通电(刹车释放)", "获取抱闸器状态"},
                                            "选择抱闸器操作", 0)); // 第1个字节
    canCommands.push_back(syncBrakeCmd);

    // 状态查询命令
    CommandInfo getStatus1Cmd(MOTOR_GET_STATUS1, "获取状态1", "获取电机状态信息1(温度、电压、电流等)");
    canCommands.push_back(getStatus1Cmd);

    CommandInfo getStatus2Cmd(MOTOR_GET_STATUS2, "获取状态2", "获取电机状态信息2(温度、转矩电流、转速、编码器)");
    canCommands.push_back(getStatus2Cmd);

    CommandInfo getStatus3Cmd(MOTOR_GET_STATUS3, "获取状态3", "获取电机状态信息3(温度、三相电流)");
    canCommands.push_back(getStatus3Cmd);

    CommandInfo clearErrorCmd(MOTOR_CLEAR_ERROR, "清除错误", "清除电机错误状态");
    canCommands.push_back(clearErrorCmd);

    // 位置查询命令
    CommandInfo getMultiPosCmd(MOTOR_GET_MULTI_POSITION, "获取多圈位置", "获取电机多圈位置信息");
    canCommands.push_back(getMultiPosCmd);

    CommandInfo getSinglePosCmd(MOTOR_GET_SINGLE_POSITION, "获取单圈位置", "获取电机单圈位置信息");
    canCommands.push_back(getSinglePosCmd);

    // 转矩控制
    CommandInfo torqueCtrlCmd(MOTOR_TORQUE_FEEDBACK_CONTROL, "转矩闭环控制", "控制电机转矩输出");
    torqueCtrlCmd.addParameter(ParameterInfo("转矩控制值", ParameterType::INT16,
                                             "转矩控制值(-2000到2000，对应实际转矩的-32.5到32.5Nm)", -2000, 2000, 3)); // 从第4个字节开始
    canCommands.push_back(torqueCtrlCmd);

    // 速度控制
    CommandInfo speedCtrlCmd(MOTOR_SPEED_FEEDBACK_CONTROL, "速度闭环控制", "控制电机转速");
    speedCtrlCmd.addParameter(ParameterInfo("速度控制值", ParameterType::INT32,
                                            "速度控制值(单位: 0.01dps/LSB)", -360000, 360000, 3)); // 从第4个字节开始
    canCommands.push_back(speedCtrlCmd);

    // 多圈位置控制1
    CommandInfo multiPosCtrl1Cmd(MOTOR_MULTI_POSITION_FEEDBACK_CONTROL1, "多圈位置控制1", "多圈位置闭环控制");
    multiPosCtrl1Cmd.addParameter(ParameterInfo("位置控制值", ParameterType::INT32,
                                                "位置控制值(单位: 0.01°/LSB)", INT32_MIN, INT32_MAX, 3)); // 从第4个字节开始
    canCommands.push_back(multiPosCtrl1Cmd);

    // 多圈位置控制2
    CommandInfo multiPosCtrl2Cmd(MOTOR_MULTI_POSITION_FEEDBACK_CONTROL2, "多圈位置控制2", "多圈位置闭环控制(最大速度限制)");
    multiPosCtrl2Cmd.addParameter(ParameterInfo("最大速度", ParameterType::UINT16,
                                                "限制电机旋转最大速度(单位: 1dps/LSB)", 0, 360, 1)); // 从第2个字节开始
    multiPosCtrl2Cmd.addParameter(ParameterInfo("位置控制值", ParameterType::INT32,
                                                "位置控制值(单位: 0.01°/LSB)", INT32_MIN, INT32_MAX, 3)); // 从第4个字节开始

    canCommands.push_back(multiPosCtrl2Cmd);

    // 单圈位置控制1
    CommandInfo singlePosCtrl1Cmd(MOTOR_SINGLE_POSITION_FEEDBACK_CONTROL1, "单圈位置控制1", "单圈位置闭环控制");
    singlePosCtrl1Cmd.addParameter(ParameterInfo("旋转方向", ParameterType::UINT8,
                                                 "限制电机旋转方向,0: 顺时针,1: 逆时针", 0)); // 从第1个字节开始
    singlePosCtrl1Cmd.addParameter(ParameterInfo("位置控制值", ParameterType::UINT32,
                                                 "位置控制值(单位: 0.01°/LSB，范围0-35999)", 0, 35999, 3)); // 从第4个字节开始
    canCommands.push_back(singlePosCtrl1Cmd);

    // 单圈位置控制2
    CommandInfo singlePosCtrl2Cmd(MOTOR_SINGLE_POSITION_FEEDBACK_CONTROL2, "单圈位置控制2", "单圈位置闭环控制(带速度限制))");
    singlePosCtrl2Cmd.addParameter(ParameterInfo("旋转方向", ParameterType::UINT8,
                                                 "限制电机旋转方向,0: 顺时针,1: 逆时针", 0)); // 从第1个字节开始
    singlePosCtrl2Cmd.addParameter(ParameterInfo("最大速度", ParameterType::UINT16,
                                                 "最大速度限制(单位: 1dps/LSB)", 0, 65535, 1)); // 从第2个字节开始
    singlePosCtrl2Cmd.addParameter(ParameterInfo("位置控制值", ParameterType::UINT32,
                                                 "位置控制值(单位: 0.01°/LSB，范围0-35999)", 0, 35999, 3)); // 从第4个字节开始
    canCommands.push_back(singlePosCtrl2Cmd);

    // 增量位置控制1
    CommandInfo incPosCtrl1Cmd(MOTOR_INCREMENTAL_POSITION_FEEDBACK_CONTROL1, "增量位置控制1", "增量位置闭环控制");
    incPosCtrl1Cmd.addParameter(ParameterInfo("增量位置值", ParameterType::INT32,
                                              "增量位置值(单位: 0.01°/LSB)", INT32_MIN, INT32_MAX, 3)); // 从第4个字节开始
    canCommands.push_back(incPosCtrl1Cmd);

    // 增量位置控制2
    CommandInfo incPosCtrl2Cmd(MOTOR_INCREMENTAL_POSITION_FEEDBACK_CONTROL2, "增量位置控制2", "增量位置闭环控制(带速度限制)");
    incPosCtrl2Cmd.addParameter(ParameterInfo("最大速度", ParameterType::UINT16,
                                              "最大速度限制(单位: 1dps/LSB)", 0, 65535, 1)); // 从第2个字节开始
    incPosCtrl2Cmd.addParameter(ParameterInfo("增量位置值", ParameterType::INT32,
                                              "增量位置值(单位: 0.01°/LSB)", INT32_MIN, INT32_MAX, 3)); // 从第4个字节开始
    canCommands.push_back(incPosCtrl2Cmd);

    deviceCommands["CAN"] = canCommands;
    deviceCommands["motor"] = canCommands; // 兼容性
}

std::vector<CommandInfo> CommandManager::getSupportedCommands(const std::string &deviceType) const
{
    auto it = deviceCommands.find(deviceType);
    if (it != deviceCommands.end())
    {
        return it->second;
    }
    return {};
}

CommandInfo CommandManager::selectCommand(const std::string &deviceType) const
{
    auto commands = getSupportedCommands(deviceType);
    if (commands.empty())
    {
        LOG_ERROR("设备类型 " + deviceType + " 不支持任何命令");
        throw std::runtime_error("不支持的设备类型");
    }

    displayCommandList(commands);

    int choice;
    while (true)
    {
        std::cout << "请选择命令 (1-" << commands.size() << "): ";
        std::cin >> choice;

        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "无效输入，请输入数字！\n";
            continue;
        }

        if (choice >= 1 && choice <= static_cast<int>(commands.size()))
        {
            return commands[choice - 1];
        }

        std::cout << "选择超出范围，请重新选择！\n";
    }
}

void CommandManager::displayCommandList(const std::vector<CommandInfo> &commands) const
{
    std::cout << "\n================ 支持的命令列表 ================\n";
    for (size_t i = 0; i < commands.size(); ++i)
    {
        std::cout << (i + 1) << ". " << commands[i].name;
        if (!commands[i].description.empty())
        {
            std::cout << " - " << commands[i].description;
        }
        if (!commands[i].parameters.empty())
        {
            std::cout << " [需要参数]";
        }
        std::cout << "\n";
    }
    std::cout << "============================================\n";
}

std::vector<uint8_t> CommandManager::getCommandParameters(const CommandInfo &cmdInfo) const
{
    std::vector<uint8_t> data(7, 0); // CAN数据帧除去命令字节的7个字节

    if (cmdInfo.parameters.empty())
    {
        std::cout << "命令 '" << cmdInfo.name << "' 不需要参数，直接发送。\n";
        return data;
    }

    std::cout << "\n命令 '" << cmdInfo.name << "' 需要以下参数:\n";

    for (size_t i = 0; i < cmdInfo.parameters.size(); ++i)
    {
        const auto &param = cmdInfo.parameters[i];
        std::cout << "\n参数 " << (i + 1) << ": " << param.name;
        if (!param.description.empty())
        {
            std::cout << "\n描述: " << param.description;
        }

        if (param.type == ParameterType::ENUM)
        {
            std::string enumResult = getEnumInput(param);
            // 对于枚举参数，将选择的索引存储到指定位置
            auto it = std::find(param.enumValues.begin(), param.enumValues.end(), enumResult);
            if (it != param.enumValues.end() && param.byteOffset < data.size())
            {
                data[param.byteOffset] = static_cast<uint8_t>(std::distance(param.enumValues.begin(), it));
            }
        }
        else
        {
            int64_t value = getIntegerInput(param);

            // 根据参数类型和指定的字节偏移量将值写入数据数组
            switch (param.type)
            {
            case ParameterType::INT8:
            case ParameterType::UINT8:
                if (param.byteOffset < data.size())
                {
                    data[param.byteOffset] = static_cast<uint8_t>(value);
                }
                break;
            case ParameterType::INT16:
            case ParameterType::UINT16:
                if (param.byteOffset + 1 < data.size())
                {
                    data[param.byteOffset] = static_cast<uint8_t>(value & 0xFF);
                    data[param.byteOffset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
                }
                break;
            case ParameterType::INT32:
            case ParameterType::UINT32:
                if (param.byteOffset + 3 < data.size())
                {
                    data[param.byteOffset] = static_cast<uint8_t>(value & 0xFF);
                    data[param.byteOffset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
                    data[param.byteOffset + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
                    data[param.byteOffset + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
                }
                break;
            default:
                break;
            }
        }
    }

    return data;
}

int64_t CommandManager::getIntegerInput(const ParameterInfo &param) const
{
    int64_t value;
    while (true)
    {
        if (param.minValue != param.maxValue)
        {
            std::cout << "请输入 " << param.name << " (" << param.minValue << " 到 " << param.maxValue << "): ";
        }
        else
        {
            std::cout << "请输入 " << param.name << ": ";
        }

        std::cin >> value;

        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "无效输入，请输入数字！\n";
            continue;
        }

        if (param.minValue != param.maxValue && (value < param.minValue || value > param.maxValue))
        {
            std::cout << "输入值超出范围 [" << param.minValue << ", " << param.maxValue << "]，请重新输入！\n";
            continue;
        }

        return value;
    }
}

std::string CommandManager::getEnumInput(const ParameterInfo &param) const
{
    std::cout << "\n可选值:\n";
    for (size_t i = 0; i < param.enumValues.size(); ++i)
    {
        std::cout << (i + 1) << ". " << param.enumValues[i] << "\n";
    }

    int choice;
    while (true)
    {
        std::cout << "请选择 (1-" << param.enumValues.size() << "): ";
        std::cin >> choice;

        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "无效输入，请输入数字！\n";
            continue;
        }

        if (choice >= 1 && choice <= static_cast<int>(param.enumValues.size()))
        {
            return param.enumValues[choice - 1];
        }

        std::cout << "选择超出范围，请重新选择！\n";
    }
}
