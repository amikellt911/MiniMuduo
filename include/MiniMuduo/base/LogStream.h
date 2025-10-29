#pragma once

#include <sstream>
#include <string>

namespace MiniMuduo
{
    namespace base
    {

        class LogStream
        {
        public:
            LogStream() = default;

            template <typename T>
            LogStream &operator<<(const T &val)
            {
                buffer_ << val;
                return *this;
            }

            const std::string str() const
            {
                return buffer_.str();
            }

        private:
            std::stringstream buffer_;
        };

    } // namespace base
} // namespace MiniMuduo
