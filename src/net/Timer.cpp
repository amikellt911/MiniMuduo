#include "MiniMuduo/net/Timer.h"

namespace MiniMuduo{
    namespace net{
        class Timer{
                Timer(TimerCallBack cb,MiniMuduo::base::Timestamp when,int64_t interval)
                :cb_(std::move(cb)),
                expiration_(when),
                interval_(interval),
                canceled_(false),
                repeated_(interval>0.0)
                {

                }
                void run();
                MiniMuduo::base::Timestamp expiration();//过期的意思
                bool isRepeat();
                void reset(MiniMuduo::base::Timestamp now);
                void cancel();
            

        };
    }
}