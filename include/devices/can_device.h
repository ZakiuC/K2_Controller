/**
 * @file can_device.h
 * @brief CAN设备头文件
 * @details 定义CAN设备类，继承自Device协议
 *         实现连接、断开连接和命令发送功能
 *         使用设备心跳检测器监控设备状态
 * @author zakiu
 * @date 2025-07-15
 * @note 该文件是设备协议的一部分，遵循C++17标准
 *      - 继承自Device类
 *      - 使用智能指针管理资源
 *      - 支持设备状态更新和心跳检测
 */
#pragma once
#include "device_protocol.h"
#include "can_interface.h"
#include <typeinfo>  // 为 dynamic_cast 提供支持

enum MOTOR_COMMAND
{
    MOTOR_DISABLE = 0x80, // 禁用命令
    MOTOR_STOP = 0x81,    // 停止命令
    MOTOR_RUN = 0x88,     // 运行命令

    MOTOR_SYNC_BRAKE = 0x8C, // 同步抱闸器命令

    MOTOR_GET_MULTI_POSITION = 0x92, // 获取多圈位置命令
    MOTOR_GET_SINGLE_POSITION = 0x94, // 获取单圈位置命令

    MOTOR_GET_STATUS1 = 0x9A, // 获取状态1命令
    MOTOR_CLEAR_ERROR = 0x9B, // 清除错误命令
    MOTOR_GET_STATUS2 = 0x9C, // 获取状态2命令
    MOTOR_GET_STATUS3 = 0x9D, // 获取状态3命令

    MOTOR_TORQUE_FEEDBACK_CONTROL = 0xA1, // 转矩闭环控制命令
    MOTOR_SPEED_FEEDBACK_CONTROL = 0xA2, // 速度闭环控制命令
    MOTOR_MULTI_POSITION_FEEDBACK_CONTROL1 = 0xA3, // 多圈位置闭环控制命令1
    MOTOR_MULTI_POSITION_FEEDBACK_CONTROL2 = 0xA4, // 多圈位置闭环控制命令2
    MOTOR_SINGLE_POSITION_FEEDBACK_CONTROL1 = 0xA5, // 单圈位置闭环控制命令1
    MOTOR_SINGLE_POSITION_FEEDBACK_CONTROL2 = 0xA6, // 单圈位置闭环控制命令2
    MOTOR_INCREMENTAL_POSITION_FEEDBACK_CONTROL1 = 0xA7, // 增量位置闭环控制命令1
    MOTOR_INCREMENTAL_POSITION_FEEDBACK_CONTROL2 = 0xA8, // 增量位置闭环控制命令2
};

enum MOTOR_STATE
{
    ON = 0x00, // 电机开启
    OFF = 0x10 // 电机关闭
};

enum BRAKE_CMD
{
    BRAKE_ON = 0x00,        // 抱闸器断电，刹车启动
    BRAKE_OFF = 0x01,       // 抱闸器通电，刹车释放
    BRAKE_GET_STATUS = 0x10 // 获取抱闸器状态
};

typedef struct
{
    int8_t temperature;     // 电机温度(1℃/LSB)
    uint16_t voltage;       // 母线电压(0.01V/LSB)
    uint16_t current;       // 母线电流(0.01A/LSB)
    MOTOR_STATE motorState; // 电机状态
    uint8_t errorState;     // 错误标志  0位，电压状态（0电压正常，1低压保护）| 3位，温度状态（0温度正常，1过温保护）
} Status1_t;

typedef struct
{
    int8_t temperature; // 电机温度(1℃/LSB)
    int16_t current;    // 转矩电流((66/4096A)/LSB)
    int16_t speed;      // 电机转速(1dps/LSB)
    uint16_t encoder;   // 编码器位置值(14bit编码器的数值范围0~16383，16bit编码器的数值范围0~65535)
} Status2_t;

typedef struct
{
    int8_t temperature; // 电机温度(1℃/LSB)
    int16_t current_A;  // A相电流数据((66/4096 A) / LSB)
    int16_t current_B;  // B相电流数据((66/4096 A) / LSB)
    int16_t current_C;  // C相电流数据((66/4096 A) / LSB)
} Status3_t;

class CANDevice : public Device
{
public:
    CANDevice(const std::string &id);

    bool connect() override;
    bool disconnect() override;
    bool sendCommand(uint8_t command, const uint8_t *data = nullptr, uint8_t response_cmd = 0, uint32_t timeout_ms = 50) override;
    void setInterface(Interface& interface) override {
        CANInterface* can_iface = dynamic_cast<CANInterface*>(&interface);
        if (can_iface) {
            this->can_interface_ = can_iface;
            LOG_DEBUG("设备 " + getId() + " 接口为：" + this->can_interface_->interface_());
        } else {
            LOG_ERROR("设备 " + getId() + " 接口类型不匹配，需要CANInterface类型");
        }
    }

    bool motorCtrl(MOTOR_COMMAND cmd);
    bool motorGetStatus(MOTOR_COMMAND cmd);
    bool motorSyncBrake(BRAKE_CMD cmd);

    bool motorGetPosition(MOTOR_COMMAND cmd);

    bool motorTorqueFeedbackControl(int16_t iqControl);
    bool motorSpeedFeedbackControl(int32_t speedControl);

private:
    bool checkDeviceAlive() override;
    void handleResponse(const struct can_frame &frame);

    std::unique_ptr<DeviceHeartbeat> heartbeat;
    CANInterface* can_interface_;

    Status1_t status1_; // 电机状态1
    Status2_t status2_; // 电机状态2
    Status3_t status3_; // 电机状态3

    int64_t multi_position_; // 多圈位置(正值表示顺时针累计角度，负值表示逆时针累计角度，单位0.01°/LSB)
    uint32_t single_position_; // 单圈位置(以编码器零点为起始点，顺时针增加，再次到达零点时数值回0，单位0.01°/LSB，数值范围0~36000*减速比-1。)
};
