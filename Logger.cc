#include <iostream>

#include "Logger.h"
// #include "Timestamp.h"

// 获取日志唯一的实例对象 单例
Logger &Logger::instance()
{
    static Logger logger;
    return logger;
}

// 设置日志级别
void Logger::setLogLevel(int level)
{
    logLevel_ = level;
}

// 写日志 [级别信息] time : msg
void Logger::log(std::string msg)
{
    std::string pre = "";
    switch (logLevel_)
    {
    case INFO:
        pre = "[INFO]";
        break;
    case ERROR:
        pre = "[ERROR]";
        break;
    case FATAL:
        pre = "[FATAL]";
        break;
    case DEBUG:
        pre = "[DEBUG]";
        break;
    default:
        break;
    }

    auto time = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(time);

    char buf[128] = {0};
    std::tm tm_time;
    localtime_r(&t, &tm_time);
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d",
             tm_time.tm_year + 1900,
             tm_time.tm_mon + 1,
             tm_time.tm_mday,
             tm_time.tm_hour,
             tm_time.tm_min,
             tm_time.tm_sec);
    // 打印时间和msg
    std::cout << pre + buf << " : " << msg << std::endl;
}