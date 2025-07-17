/**
 * @file can_device.cpp
 * @brief CAN设备实现文件
 * @details 实现CAN设备的连接、断开连接和命令发送功能
 *          使用设备心跳检测器监控设备状态
 * @author zakiu
 * @date 2025-07-15
 */
#include "can_device.h"
#include "can_device_config.h"

/**
 * @brief CANDevice构造函数
 * @param id 设备唯一标识符
 * @details 初始化CAN设备，设置设备类型为"CAN"
 */
CANDevice::CANDevice(const std::string &id) : Device(id, "CAN"), can_interface_(nullptr)
{
    LOG_INFO(" 创建 CAN 设备: [" + id + "]");
    heartbeat = std::make_unique<DeviceHeartbeat>(this);
}

/**
 * @brief 连接CAN设备
 * @details 实现连接逻辑，启动心跳检测器
 * @return bool 连接成功返回true，失败返回false
 */
bool CANDevice::connect()
{
    LOG_INFO("正在连接 CAN 设备: [" + id + "]");
    if (!motorCtrl(MOTOR_RUN))
    {
        updateStatus(DeviceStatus::DISCONNECTED);
    }
    else
    {
        updateStatus(DeviceStatus::CONNECTED);
    }
    // 实际CAN连接逻辑
    heartbeat->start();
    return true;
}

/**
 * @brief 断开CAN设备连接
 * @details 实现断开连接逻辑，停止心跳检测器
 * @return bool 断开成功返回true，失败返回false
 */
bool CANDevice::disconnect()
{
    LOG_INFO("正在断开 CAN 设备: [" + id + "]");
    if (heartbeat)
    {
        heartbeat->stop();
        heartbeat.reset();
    }
    updateStatus(DeviceStatus::DISCONNECTED);
    return true;
}

/**
 * @brief 发送命令到CAN设备
 * @details 发送CAN帧并等待响应
 * @param command 要发送的命令
 * @param data 附加数据（可选）
 * @param response_cmd 期望的响应命令（默认为0，表示使用发送的命令作为响应）
 * @param timeout_ms 超时时间（默认为50毫秒）
 * @return bool 发送和接收成功返回true，失败返回false
 * @note 如果没有指定响应命令，则使用发送的命令作为响应命令
 *       如果发送的命令不需要响应，可以不输入response_cmd使用默认值0
 *      如果需要等待响应，确保在发送命令时设置正确的response_cmd
 *      超时时间可以根据实际情况调整，默认为50毫秒
 */
bool CANDevice::sendCommand(uint8_t command, const uint8_t *data, uint8_t response_cmd, uint32_t timeout_ms)
{
    struct can_frame frame;

    frame.can_id = 0x140 + getDeviceIdFromString(id); // 标准帧 ID
    frame.can_dlc = 8;                                // 数据长度固定8字节

    frame.data[0] = command;
    for (int i = 1; i < 8; i++)
    {
        frame.data[i] = data ? data[i] : 0x00; // 填充数据或默认0
    }

    if (!can_interface_ || !can_interface_->send_frame(frame))
    {
        return false; // 发送失败
    }
    LOG_DEBUG("命令 0x" + std::to_string(command) + " 发送成功。");

    if (response_cmd == 0)
    {
        response_cmd = command; // 如果没有指定响应命令，则使用发送的命令作为响应命令
    }
    // 记录当前时间
    auto start_time = std::chrono::steady_clock::now();
    // 循环检查是否收到响应
    while (true)
    {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();

        // 超时检查
        if (elapsed_time >= timeout_ms)
        {
            LOG_ERROR("等待命令响应超时: 0x" + std::to_string(response_cmd) + " after " + std::to_string(elapsed_time) + " ms");
            return false;
        }

        if (can_interface_ && can_interface_->receive_frame(frame, 50)) // 每50ms检查一次
        {
            // 获取接收到帧时的时间并重新计算
            current_time = std::chrono::steady_clock::now();
            elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();

            // 检查返回的帧是否正确
            if (frame.can_id == (0x140 + std::stoi(id)) && frame.data[0] == response_cmd)
            {
                LOG_DEBUG("命令 0x" + std::to_string(response_cmd) + " 接收成功。等待响应时间: " + std::to_string(elapsed_time) + " ms");
#if CAN_DEVICE_HANDLE_RESPONSE_ENABLE
                // 解析返回数据
                handleResponse(frame);
#endif
                return true; // 发送和接收都成功
            }
        }
    }
}

bool CANDevice::motorCtrl(MOTOR_COMMAND cmd)
{
    if (cmd <= MOTOR_RUN)
    {
        return sendCommand(cmd);
    }
    LOG_WARNING("无效的电机状态控制命令(此函数仅处理电机 启动/停止/关闭): " + std::to_string(static_cast<int>(cmd)));
    return false;
}

bool CANDevice::motorGetStatus(MOTOR_COMMAND cmd)
{
    if (cmd <= MOTOR_GET_STATUS3 && cmd != MOTOR_CLEAR_ERROR)
    {
        return sendCommand(cmd);
    }
    LOG_WARNING("无效的电机状态读取命令(此函数仅处理电机 启动/停止/关闭): " + std::to_string(static_cast<int>(cmd)));
    return false;
}

bool CANDevice::motorSyncBrake(BRAKE_CMD cmd)
{
    return sendCommand(MOTOR_SYNC_BRAKE, reinterpret_cast<const uint8_t *>(&cmd));
}

bool CANDevice::motorGetPosition(MOTOR_COMMAND cmd)
{
    if (cmd == MOTOR_GET_MULTI_POSITION || cmd == MOTOR_GET_SINGLE_POSITION)
    {
        return sendCommand(cmd);
    }
    LOG_WARNING("无效的电机位置获取命令: " + std::to_string(static_cast<int>(cmd)));
    return false;
}

/**
 * @brief 闭环控制电机转矩
 * @details 发送速度闭环控制命令
 * @param iqControl 转矩控制值，范围为-2048到2048
 * @return bool 发送成功返回true，失败返回false
 * @note iqControl的范围是-2048到2048，超过此范围会返回错误
 *      对应TS电机实际转矩电流范围-33A~33A
 *      该命令中的控制值iqControl不受上位机中的Max Torque Current值限制
 */
bool CANDevice::motorTorqueFeedbackControl(int16_t iqControl)
{
    if (iqControl < -2048 || iqControl > 2048)
    {
        LOG_ERROR("转矩控制值超出范围: " + std::to_string(iqControl));
        return false;
    }

    uint8_t data[8];
    data[4] = static_cast<uint8_t>(iqControl & 0xFF);        // 低字节
    data[5] = static_cast<uint8_t>((iqControl >> 8) & 0xFF); // 高字节
    return sendCommand(MOTOR_TORQUE_FEEDBACK_CONTROL, data);
}

/**
 * @brief 闭环控制电机速度
 * @details 发送速度闭环控制命令
 * @param speedControl 速度控制值，实际转速为 0.01dps/LSB
 * @return bool 发送成功返回true，失败返回false
 * @note 该命令下电机的speedControl由上位机中的Max Speed值限制。
 * 该控制模式下，电机的最大加速度由上位机中的Max Acceleration值限制。
 * 该控制模式下，电机的最大转矩电流由上位机中的Max Torque Current值限制。
 */
bool CANDevice::motorSpeedFeedbackControl(int32_t speedControl)
{
    uint8_t data[8];
    data[4] = static_cast<uint8_t>(speedControl & 0xFF);         // 低字节
    data[5] = static_cast<uint8_t>((speedControl >> 8) & 0xFF);  // 次低字节
    data[6] = static_cast<uint8_t>((speedControl >> 16) & 0xFF); // 次高字节
    data[7] = static_cast<uint8_t>((speedControl >> 24) & 0xFF); // 高字节
    return sendCommand(MOTOR_SPEED_FEEDBACK_CONTROL, data);
}

/**
 * @brief 检查CAN设备是否存活
 * @details 实现心跳检测逻辑
 * @return bool 设备存活返回true，未存活返回false
 */
bool CANDevice::checkDeviceAlive()
{
    bool isAlive = true;

    // 输出检查结果
    if (!motorGetStatus(MOTOR_GET_STATUS1))
    {
        isAlive = false;
        LOG_ERROR("设备 " + id + " 未响应状态1请求，可能已断开连接或故障。");
    }
    else
    {
        LOG_DEBUG("设备 " + id + " 状态1检查通过。");
    }
    if (!motorGetStatus(MOTOR_GET_STATUS2))
    {
        isAlive = false;
        LOG_ERROR("设备 " + id + " 未响应状态2请求，可能已断开连接或故障。");
    }
    else
    {
        LOG_DEBUG("设备 " + id + " 状态2检查通过。");
    }
    if (!motorGetStatus(MOTOR_GET_STATUS3))
    {
        isAlive = false;
        LOG_ERROR("设备 " + id + " 未响应状态3请求，可能已断开连接或故障。");
    }
    else
    {
        LOG_DEBUG("设备 " + id + " 状态3检查通过。");
    }

    return isAlive;
}

void CANDevice::handleResponse(const can_frame &frame)
{
    uint8_t status_code = frame.data[0];
    switch (status_code)
    {
    case MOTOR_GET_STATUS1:
        // 解析状态1数据
        status1_.temperature = frame.data[1];
        status1_.voltage = ((frame.data[3] << 8) | frame.data[2]);
        status1_.current = ((frame.data[5] << 8) | frame.data[4]);
        status1_.motorState = static_cast<MOTOR_STATE>(frame.data[6]);
        status1_.errorState = frame.data[7];
        char errorStateHex[10];
        std::sprintf(errorStateHex, "0x%04X", static_cast<int>(status1_.errorState));
        LOG_DEBUG("读取状态1: \n\t电机温度: " + std::to_string(status1_.temperature) + "℃"
                                                                                       "\n\t母线电压: " +
                  std::to_string(status1_.voltage) + "V"
                                                     "\n\t母线电流: " +
                  std::to_string(status1_.current) + "A"
                                                     "\n\t电机状态: " +
                  (status1_.motorState == MOTOR_STATE::OFF ? "关闭" : "开启") +
                  "\n\t错误状态: " + errorStateHex);
        break;

    case MOTOR_GET_STATUS2:
        // 解析状态2数据
        status2_.temperature = frame.data[1];
        status2_.current = (frame.data[3] << 8) | frame.data[2];
        status2_.speed = (frame.data[5] << 8) | frame.data[4];
        status2_.encoder = (frame.data[7] << 8) | frame.data[6];
        LOG_DEBUG("读取状态2: \n\t电机温度: " + std::to_string(status2_.temperature) + "℃"
                                                                                       "\n\t母线电流: " +
                  std::to_string(status2_.current) + "A"
                                                     "\n\t电机速度: " +
                  std::to_string(status2_.speed) + "dps"
                                                   "\n\t编码器: " +
                  std::to_string(status2_.encoder));
        break;

    case MOTOR_GET_STATUS3:
        // 解析状态3数据
        status3_.temperature = frame.data[1];
        status3_.current_A = (frame.data[3] << 8) | frame.data[2];
        status3_.current_B = (frame.data[5] << 8) | frame.data[4];
        status3_.current_C = (frame.data[7] << 8) | frame.data[6];
        LOG_DEBUG("读取状态3: \n\t电机温度: " + std::to_string(status3_.temperature) + "℃"
                                                                                       "\n\t电流A: " +
                  std::to_string(status3_.current_A) + "A"
                                                       "\n\t电流B: " +
                  std::to_string(status3_.current_B) + "A"
                                                       "\n\t电流C: " +
                  std::to_string(status3_.current_C) + "A");
        break;
    case MOTOR_GET_MULTI_POSITION:
        multi_position_ = 0;
        for (int i = 1; i < 8; i++)
        {
            multi_position_ |= (static_cast<int64_t>(frame.data[i]) << ((i - 1) * 8));
        }
        LOG_DEBUG("读取多圈位置: " + std::to_string(multi_position_) + " (单位: 0.01°/LSB)");
        break;

    case MOTOR_GET_SINGLE_POSITION:
        single_position_ = 0;
        single_position_ = (static_cast<uint32_t>(frame.data[4])) |
                           (static_cast<uint32_t>(frame.data[5]) << 8) |
                           (static_cast<uint32_t>(frame.data[6]) << 16) |
                           (static_cast<uint32_t>(frame.data[7]) << 24);
        LOG_DEBUG("读取单圈位置: " + std::to_string(single_position_) + " (单位: 0.01°/LSB, 范围: 0~36000*减速比-1)");
        break;

    default:
        LOG_WARNING("未解析: " + std::to_string(status_code));
        break;
    }
}
