#pragma once

#include <iostream>
#include <string>
#include "noncopyable.h"
namespace llt_muduo
{
    class Timestamp : public noncopyable
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