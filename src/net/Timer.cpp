#include "MiniMuduo/net/Timer.h"

namespace MiniMuduo{
    namespace net{
        std::atomic<int64_t> Timer::s_numCreated_(0);
            Timer::Timer(TimerCallBack cb,MiniMuduo::base::Timestamp when,double interval)
            :cb_(std::move(cb)),
            expiration_(when),
            interval_(interval),
            canceled_(false),
            repeated_(interval>0.0),
            sequence_(s_numCreated_.fetch_add(1)+1)
            {

            }
            // Timer::Timer()
            // :expiration_(MiniMuduo::base::Timestamp::now())
            // ,canceled_(true),
            // repeated_(false),
            // sequence_(0),
            // interval_(0.0)
            // {

            // }
            void Timer::run() const{
                if(cb_&&!canceled_)
                {
                    cb_();
                }
            }
            MiniMuduo::base::Timestamp Timer::expiration() const//过期的意思
            {
                return expiration_;
            }
            bool Timer::repeat() const
            {
                return repeated_;
            }
            void Timer::reset(MiniMuduo::base::Timestamp now)
            {
                if(repeat())
                {
                    expiration_=now+interval_;
                }
            }
            void Timer::cancel()
            {
                canceled_=true;
            }
            int64_t Timer::sequence() const
            {
                return sequence_;
            }

        
    }
}