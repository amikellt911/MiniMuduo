#pragma once

#include <atomic>
#include <string>
#include <fstream>
#include "MiniMuduo/base/noncopyable.h"

namespace MiniMuduo
{

    namespace base
    {
        enum class LogLevel
        {
            DEBUG,
            INFO,
            ERROR,
            FATAL
        };

        class Logger : public noncopyable
        {
        public:
            Logger();
            ~Logger();

            static Logger &instance()
            {
                static Logger instance_;
                return instance_;
            }

            void setGlobalLogLevel(LogLevel level);

            void log(const std::string &msg, LogLevel level,const char *file, int line);

            inline bool shouldLog(LogLevel level)
            {
                return level >= Logger::instance().globalLogLevel_;
            }

        private:
            LogLevel globalLogLevel_;
            std::atomic_flag logLock_;
            std::ofstream logFile_;
            std::string logFilePath_;
            std::string logFileName_;
        };
    }

} // namespace MiniMuduo

#define LOG_DEBUG(msg)                                                                      \
    do                                                                                      \
    {                                                                                        \
        if (MiniMuduo::base::Logger::instance().shouldLog(MiniMuduo::base::LogLevel::DEBUG)) \
        {                                                                                   \
            MiniMuduo::base::Logger::instance().log(msg, MiniMuduo::base::LogLevel::DEBUG,__FILE__, __LINE__); \
        }                                                                                   \
    } while (0)

#define LOG_INFO(msg)                                                                      \
    do                                                                                     \
    {                                                                                      \
        if (MiniMuduo::base::Logger::instance().shouldLog(MiniMuduo::base::LogLevel::INFO)) \
        {                                                                                  \
            MiniMuduo::base::Logger::instance().log(msg, MiniMuduo::base::LogLevel::INFO,__FILE__, __LINE__); \
        }                                                                                  \
    } while (0)
//普通错误，整体可控，虽然有bug
#define LOG_ERROR(msg)                                                                      \
    do                                                                                      \
    {                                                                                       \
        if (MiniMuduo::base::Logger::instance().shouldLog(MiniMuduo::base::LogLevel::ERROR)) \
        {                                                                                   \
            MiniMuduo::base::Logger::instance().log(msg, MiniMuduo::base::LogLevel::ERROR,__FILE__, __LINE__); \
        }                                                                                   \
    } while (0)
//致命错误，会影响核心功能
#define LOG_FATAL(msg)                                                                      \
    do                                                                                      \
    {                                                                                       \
        if (MiniMuduo::base::Logger::instance().shouldLog(MiniMuduo::base::LogLevel::FATAL)) \
        {                                                                                   \
            MiniMuduo::base::Logger::instance().log(msg, MiniMuduo::base::LogLevel::FATAL,__FILE__, __LINE__); \
        }                                                                                   \
    } while (0)
    