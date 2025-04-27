#pragma once

#include <atomic>
#include <string>
#include <fstream>
#include "noncopyable.h"

namespace llt_muduo
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

            void log(const std::string &msg, LogLevel level);

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

} // namespace llt_muduo

#define LOG_DEBUG(msg)                                                                      \
    do                                                                                      \
    {                                                                                       \
        if (llt_muduo::base::Logger::instance().shouldLog(llt_muduo::base::LogLevel::DEBUG)) \
        {                                                                                   \
            llt_muduo::base::Logger::instance().log(msg, llt_muduo::base::LogLevel::DEBUG); \
        }                                                                                   \
    } while (0)

#define LOG_INFO(msg)                                                                      \
    do                                                                                     \
    {                                                                                      \
        if (llt_muduo::base::Logger::instance().shouldLog(llt_muduo::base::LogLevel::INFO)) \
        {                                                                                  \
            llt_muduo::base::Logger::instance().log(msg, llt_muduo::base::LogLevel::INFO); \
        }                                                                                  \
    } while (0)
//普通错误，整体可控，虽然有bug
#define LOG_ERROR(msg)                                                                      \
    do                                                                                      \
    {                                                                                       \
        if (llt_muduo::base::Logger::instance().shouldLog(llt_muduo::base::LogLevel::ERROR)) \
        {                                                                                   \
            llt_muduo::base::Logger::instance().log(msg, llt_muduo::base::LogLevel::ERROR); \
        }                                                                                   \
    } while (0)
//致命错误，会影响核心功能
#define LOG_FATAL(msg)                                                                      \
    do                                                                                      \
    {                                                                                       \
        if (llt_muduo::base::Logger::instance().shouldLog(llt_muduo::base::LogLevel::FATAL)) \
        {                                                                                   \
            llt_muduo::base::Logger::instance().log(msg, llt_muduo::base::LogLevel::FATAL); \
        }                                                                                   \
    } while (0)