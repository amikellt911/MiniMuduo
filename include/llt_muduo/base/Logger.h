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

#define LOG_DEBUG(msg)                                                     \
    do                                                                     \
    {                                                                      \
        if (Logger::Logger::instance().shouldLog(Logger::LogLevel::DEBUG)) \
        {                                                                  \
            Logger::Logger::instance().log(msg, Logger::LogLevel::DEBUG);  \
        }                                                                  \
    } while (0)

#define LOG_INFO(msg)                                                     \
    do                                                                    \
    {                                                                     \
        if (Logger::Logger::instance().shouldLog(Logger::LogLevel::INFO)) \
        {                                                                 \
            Logger::Logger::instance().log(msg, Logger::LogLevel::INFO);  \
        }                                                                 \
    } while (0)
#define LOG_ERROR(msg)                                                     \
    do                                                                     \
    {                                                                      \
        if (Logger::Logger::instance().shouldLog(Logger::LogLevel::ERROR)) \
        {                                                                  \
            Logger::Logger::instance().log(msg, Logger::LogLevel::ERROR);  \
        }                                                                  \
    } while (0)
#define LOG_FATAL(msg)                                                     \
    do                                                                     \
    {                                                                      \
        if (Logger::Logger::instance().shouldLog(Logger::LogLevel::FATAL)) \
        {                                                                  \
            Logger::Logger::instance().log(msg, Logger::LogLevel::FATAL);  \
        }                                                                  \
    } while (0)