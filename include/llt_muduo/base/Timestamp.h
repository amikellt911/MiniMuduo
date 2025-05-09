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
            Timestamp();
            explicit Timestamp(int64_t microSecondsSinceEpoch);
            static Timestamp now();
            std::string toString() const;

        private:
            int64_t microSecondsSinceEpoch_;
        };
    }
}