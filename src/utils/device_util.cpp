#include "device_util.h"


/**
 * @brief 从设备ID字符串中提取数字部分
 * @param deviceId 设备ID字符串，格式为 "device_<number>"
 * @return int 提取的数字部分，如果格式不正确则返回-1
 */
int getDeviceIdFromString(const std::string &deviceId)
{
    // 查找下划线的位置
    size_t pos = deviceId.find('_');

    // 提取下划线后的部分并转换为整数
    if (pos != std::string::npos)
    {
        int number = std::stoi(deviceId.substr(pos + 1)); // 从下划线后面提取数字
    }
    else
    {
        return -1; // 如果没有下划线，返回-1表示无效
    }
}
