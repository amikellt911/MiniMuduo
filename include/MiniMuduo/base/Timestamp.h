#pragma once

#include <iostream>
#include <string>
#include "MiniMuduo/base/noncopyable.h"
namespace MiniMuduo
{
    namespace base
    {
        class Timestamp
        {
        public:
            static constexpr int kMicroSecondsPerSecond = 1000 * 1000;
        public:
            Timestamp();
            explicit Timestamp(int64_t microSecondsSinceEpoch);
            static Timestamp now();
            std::string toString() const;
            Timestamp operator+(double interval);
            bool operator<(const Timestamp &that) const;
            int64_t microSecondsSinceEpoch() const;
        private:
            int64_t microSecondsSinceEpoch_;
        };
    }
}