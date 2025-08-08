/**
 * @file logger.cpp
 * @brief 日志记录器实现文件
 * @details 提供线程安全的日志记录功能，支持多种日志级别和文件输出
 * @author zakiu
 * @date 2025-07-15
 */

#include "logger.h"
#include <iostream>
#include <filesystem>  // 用于路径操作

/**
 * @brief 获取Logger单例实例
 * @details 使用线程安全的单例模式，保证全局只有一个Logger实例
 * @return Logger& Logger实例的引用
 */
Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

/**
 * @brief Logger构造函数
 * @details 私有构造函数，初始化日志系统
 *          - 创建logs目录（如果不存在）
 *          - 设置默认日志文件为logs/device_control.log
 *          - 默认启用终端输出
 */
Logger::Logger() : consoleOutputEnabled(true) {
    // 创建日志目录（如果不存在）
    std::string logDir = "logs";
    if (!std::filesystem::exists(logDir)) {
        std::filesystem::create_directory(logDir);
    }
    
    // 打开默认日志文件
    setLogFile(logDir + "/device_control.log");
}

/**
 * @brief Logger析构函数
 * @details 关闭打开的日志文件，释放资源
 */
Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

/**
 * @brief 记录日志信息
 * @details 线程安全的日志记录函数，同时输出到控制台和文件
 *          - 使用互斥锁保证线程安全
 *          - 自动添加时间戳和日志级别
 *          - 同时输出到控制台和日志文件
 * @param level 日志级别 (DEBUG, INFO, WARNING, ERROR, CRITICAL)
 * @param message 要记录的日志消息
 */
void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);

    // 确保日志文件已打开
    if (!logFile.is_open()) {
        std::cerr << "日志文件未打开！" << std::endl;
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_tm = *std::localtime(&now_time_t);
    
    std::ostringstream timestamp;
    timestamp << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");

    // 将日志级别枚举转换为字符串
    std::string levelStr;
    switch(level) {
        case DEBUG: levelStr = "DEBUG"; break;
        case INFO: levelStr = "INFO"; break;
        case WARNING: levelStr = "WARNING"; break;
        case ERROR: levelStr = "ERROR"; break;
        case CRITICAL: levelStr = "CRITICAL"; break;
    }

    // 构造完整的日志条目
    std::ostringstream logEntry;
    logEntry << "[" << timestamp.str() << "] [" << levelStr << "] " << message;

    // 根据开关决定是否输出到控制台
    if (consoleOutputEnabled) {
        std::cout << logEntry.str() << std::endl;
    }

    // 写入文件
    logFile << logEntry.str() << std::endl;
    logFile.flush();
}

/**
 * @brief 设置日志文件路径
 * @details 线程安全地更改日志输出文件
 *          - 关闭当前日志文件（如果已打开）
 *          - 尝试打开新的日志文件
 *          - 如果目录不存在则自动创建
 *          - 如果无法创建文件则回退到控制台输出
 * @param filename 新的日志文件路径
 */
void Logger::setLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    // 关闭当前文件（如果已打开）
    if (logFile.is_open()) {
        logFile.close();
    }
    
    // 打开新文件
    logFile.open(filename, std::ios::out | std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "打开日志文件失败: " << filename << std::endl;
        // 尝试创建父目录
        auto parent_path = std::filesystem::path(filename).parent_path();
        if (!parent_path.empty() && !std::filesystem::exists(parent_path)) {
            std::filesystem::create_directories(parent_path);
            logFile.open(filename, std::ios::out | std::ios::app);
        }
        
        // 如果仍然失败，使用标准错误输出
        if (!logFile.is_open()) {
            std::cerr << "无法创建或打开日志文件，日志将只输出到控制台" << std::endl;
        }
    }
}

/**
 * @brief 设置控制台输出开关
 * @details 控制日志是否输出到控制台终端
 * @param enabled true表示启用控制台输出，false表示禁用
 */
void Logger::setConsoleOutput(bool enabled) {
    std::lock_guard<std::mutex> lock(logMutex);
    consoleOutputEnabled = enabled;
}

/**
 * @brief 获取控制台输出状态
 * @details 返回当前控制台输出是否启用
 * @return bool true表示控制台输出启用，false表示禁用
 */
bool Logger::isConsoleOutputEnabled() const {
    return consoleOutputEnabled;
}