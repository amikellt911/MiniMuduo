#pragma once

#include "MiniMuduo/base/Timestamp.h"
#include <functional>
#include <
namespace MiniMuduo{
    namespace net{
        class Timer{
            public:
                using TimerCallBack=std::function<void()>;
                Timer(TimerCallBack cb,MiniMuduo::base::Timestamp when,int64_t interval);
                void run();
                MiniMuduo::base::Timestamp expiration();//过期的意思
                bool isRepeat();
                void reset(MiniMuduo::base::Timestamp now);
                void cancel();
            private:
                MiniMuduo::base::Timestamp expiration_;
                double interval_;
                TimerCallBack cb_;
                bool canceled_;
                bool repeated_;
                int64_t sequence_;
                static atomic

        };
    }
}