#pragma once

#include "MiniMuduo/base/Timestamp.h"   
#include "MiniMuduo/base/noncopyable.h"
#include "MiniMuduo/net/Channel.h"
#include "MiniMuduo/net/Callbacks.h"
#include "MiniMuduo/net/Timer.h"
#include <memory>
#include <set>
namespace MiniMuduo {
namespace net { 
    class EventLoop;

    class TimerQueue : MiniMuduo::base::noncopyable
    {

    public:
        explicit TimerQueue(EventLoop *loop);
        ~TimerQueue();
        //唯一暴露在外接口
        void addTimer(int64_t sequence,MiniMuduo::net::TimerCallBack cb, MiniMuduo::base::Timestamp when, double interval);
        void cancelTimer(int64_t sequence);
        void resetTimer(int64_t sequence,MiniMuduo::base::Timestamp when,double interval);
        
    private:

        EventLoop *loop_;
        const int timerfd_;
        Channel timerfdChannel_;
        //防止插入时重置timefd
        bool callingExpiredTimers_;
        //pair默认先比较第一个，再比较第二个，而且默认less compare
        using Entry=std::pair<MiniMuduo::base::Timestamp, Timer*>;
        using TimerList = std::set<Entry>;
        using ActiveTimerSet = std::unordered_map<Timer*,std::unique_ptr<Timer>>;
        TimerList timers_;// 只负责排序，存裸指针
        ActiveTimerSet activeTimers_; // 只负责对象生命周期

        std::unordered_map<int64_t,Timer*> timerIdFind; 
        //真正的过期时间
        MiniMuduo::base::Timestamp expiration_;

    private:
        void addTimerInLoop(std::unique_ptr<Timer> timer);
        void handleRead();

        std::vector<Entry> getExpired(MiniMuduo::base::Timestamp now);
        void reset(std::vector<Entry>& expired,MiniMuduo::base::Timestamp now);

        bool insert(Timer* timer);

        void resetTimerfd(MiniMuduo::base::Timestamp expiration);
        void readTimerfd();

        void cancelTimerInLoop(int64_t sequence);

        void resetTimerInLoop(int64_t sequence,MiniMuduo::base::Timestamp when,double interval);

        bool hasTimer(int64_t sequence);
    };
    

    
}
}
