#pragma once

#include "MiniMuduo/base/LogStream.h"
#include "MiniMuduo/base/Logger.h"

namespace MiniMuduo
{
    namespace base
    {

        class LogMessage
        {
        public:
            LogMessage(LogLevel level, const char *file, int line)
                : level_(level), file_(file), line_(line) {}

            ~LogMessage()
            {
                Logger::instance().log(stream_.str(), level_, file_, line_);
            }

            LogStream &stream() { return stream_; }

        private:
            LogStream stream_;
            LogLevel level_;
            const char *file_;
            int line_;
        };

    } // namespace base
} // namespace MiniMuduo

#define LOG_STREAM_DEBUG if (MiniMuduo::base::Logger::instance().shouldLog(MiniMuduo::base::LogLevel::DEBUG)) \
    MiniMuduo::base::LogMessage(MiniMuduo::base::LogLevel::DEBUG, __FILE__, __LINE__).stream()

#define LOG_STREAM_INFO  if (MiniMuduo::base::Logger::instance().shouldLog(MiniMuduo::base::LogLevel::INFO)) \
    MiniMuduo::base::LogMessage(MiniMuduo::base::LogLevel::INFO, __FILE__, __LINE__).stream()

#define LOG_STREAM_ERROR if (MiniMuduo::base::Logger::instance().shouldLog(MiniMuduo::base::LogLevel::ERROR)) \
    MiniMuduo::base::LogMessage(MiniMuduo::base::LogLevel::ERROR, __FILE__, __LINE__).stream()

#define LOG_STREAM_FATAL if (MiniMuduo::base::Logger::instance().shouldLog(MiniMuduo::base::LogLevel::FATAL)) \
    MiniMuduo::base::LogMessage(MiniMuduo::base::LogLevel::FATAL, __FILE__, __LINE__).stream()
