#pragma once

#include "MiniMuduo/base/Timestamp.h"
#include <functional>
#include <atomic>
namespace MiniMuduo{
    namespace net{
        class Timer{
            public:
                using TimerCallBack=std::function<void()>;
                Timer(TimerCallBack cb,MiniMuduo::base::Timestamp when,double interval);
                //Timer();
                void run() const;
                MiniMuduo::base::Timestamp expiration() const;//过期的意思
                bool repeat() const;
                void reset(MiniMuduo::base::Timestamp now);
                void cancel();
                int64_t sequence() const;
            private:
                MiniMuduo::base::Timestamp expiration_;
                const double interval_;
                TimerCallBack cb_;
                bool canceled_;
                const bool repeated_;
                const int64_t sequence_;
                static std::atomic<int64_t> s_numCreated_;

        };
    }
}