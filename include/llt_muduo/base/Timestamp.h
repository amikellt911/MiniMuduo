#pragma once

#include <iostream>
#include <string>
#include "llt_muduo/base/noncopyable.h"
namespace llt_muduo
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