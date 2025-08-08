#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

enum LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class Logger {
public:
    static Logger& getInstance();
    void log(LogLevel level, const std::string& message);
    void setLogFile(const std::string& filename);
    void setConsoleOutput(bool enabled);
    bool isConsoleOutputEnabled() const;

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream logFile;
    std::mutex logMutex;
    bool consoleOutputEnabled;
};

// 日志宏定义
#define LOG_DEBUG(msg) Logger::getInstance().log(DEBUG, msg)
#define LOG_INFO(msg) Logger::getInstance().log(INFO, msg)
#define LOG_WARNING(msg) Logger::getInstance().log(WARNING, msg)
#define LOG_ERROR(msg) Logger::getInstance().log(ERROR, msg)
#define LOG_CRITICAL(msg) Logger::getInstance().log(CRITICAL, msg)