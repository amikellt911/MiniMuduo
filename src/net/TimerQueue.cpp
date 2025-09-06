#include "MiniMuduo/net/TimerQueue.h"
#include "MiniMuduo/net/EventLoop.h"
#include <sys/timerfd.h>
#include <sys/unistd.h>
#include <MiniMuduo/base/Logger.h>
namespace MiniMuduo { 
    namespace net { 
        TimerQueue::TimerQueue(EventLoop *loop)
        :loop_(loop),
        timerfd_(::timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK | TFD_CLOEXEC)),
        timerfdChannel_(loop,timerfd_),
        timers_(),
        callingExpiredTimers_(false)
    {
        timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead,this));
    }

    void TimerQueue::addTimer(MiniMuduo::net::TimerCallBack cb, MiniMuduo::base::Timestamp when, double interval)
    {
        auto timer = std::make_unique<Timer>(std::move(cb),when,interval);
        //使用 lambda 表达式来代替 std::bind,因为bind的拷贝和unique_ptr的移动，会产生各种奇怪的错误，这也是effective C++推崇的
        //放弃bind,选择lambda
        //mutable是因为lambda的 operator默认是const的
        loop_->runInLoop([this, timer_ptr = std::move(timer)]() mutable {
            // 在 lambda 体内，调用真正的处理函数
            // 这里必须用 std::move，因为 addTimerInLoop 的参数是 unique_ptr 按值传递
            addTimerInLoop(std::move(timer_ptr));
        });
    }
    
    void TimerQueue::addTimerInLoop(std::unique_ptr<Timer> timer)
    { 
        loop_->assertInLoopThread();
        //move前获取
        MiniMuduo::base::Timestamp when = timer->expiration();
        Timer* p_timer = timer.get();
        activeTimers_[p_timer]=std::move(timer);
        bool eraliestChanged=insert(p_timer);
        if(eraliestChanged&&!callingExpiredTimers_)
        {
            resetTimerfd(when);
        }
    }

    bool TimerQueue::insert(Timer* timer)
    {
        MiniMuduo::base::Timestamp when = timer->expiration();
        bool eraliestChanged = timers_.empty() || when<timers_.begin()->first;
        timers_.insert({when,timer});
        return eraliestChanged;
    }

    void TimerQueue::handleRead()
    {
        readTimerfd();
        MiniMuduo::base::Timestamp now=MiniMuduo::base::Timestamp::now();
        std::vector<Entry> expired = getExpired(now);
        callingExpiredTimers_ = true;
        for(const Entry &expiredTimer : expired)
        {
            expiredTimer.second->run();
        }
        callingExpiredTimers_ = false;
        reset(expired,now);

        if(!timers_.empty())
        {
            MiniMuduo::base::Timestamp nextExpire = timers_.begin()->first;
            resetTimerfd(nextExpire);
        }
    }

    std::vector<TimerQueue::Entry> TimerQueue::getExpired(MiniMuduo::base::Timestamp now)
    {
        std::vector<Entry> expired;
        //默认是空指针，所以不需要无参构造
        Entry sentry(now,reinterpret_cast<Timer*>(UINTPTR_MAX));
        TimerList::iterator end = timers_.lower_bound(sentry);
        //因为move本质其实是 *it++=move(e)
        //然后看你有没有移动函数
        //但是set的迭代器只有const,所以move后会是const&&，但是右值转移的vector的push_back只有&&没有const&&
        //所以他还会拷贝，为了防止语义问题，所以换成了拷贝写法
        //std::move(timers_.begin(),end,std::back_inserter(expired));
        for (auto it = timers_.begin(); it != end; ++it) {
            expired.push_back(*it);
        }
        //删了可能有问题，因为我需要他的原来的裸指针才能一一对应
        timers_.erase(timers_.begin(),end);
        return expired;
    }

    void TimerQueue::reset(std::vector<Entry> &expired,MiniMuduo::base::Timestamp now)
    {
        for(Entry &expiredTimer : expired)
        {
            if(expiredTimer.second->repeat())
            {
                //bug，因为first是时间戳，first没有变
                // expiredTimer.second->reset(now);
                // timers_.insert(expiredTimer);
                //insert(std::move(expiredTimer.second));
                expiredTimer.second->reset(now);
                insert(std::move(expiredTimer.second));
            }
            else
            {
                //不然就删除
                size_t n=activeTimers_.erase(expiredTimer.second);
                if(n!=1)
                {
                    LOG_ERROR("TimerQueue::handleRead() -> reset deletes "+std::to_string(n)+" timers instead of 1");
                }
            }
        }
    }

    void TimerQueue::readTimerfd()
    {
        uint64_t howmany;
        size_t n=::read(timerfd_,&howmany,sizeof howmany);
        if(n!=sizeof howmany)
        {
            LOG_ERROR("TimerQueue::handleRead() reads "+std::to_string(n)+" bytes instead of 8");
        }
    }

    void TimerQueue::resetTimerfd(MiniMuduo::base::Timestamp expiration)
    {
        struct itimerspec newValue;
        struct itimerspec oldValue;
        
        // 清零，避免包含垃圾值
        newValue = {};
        oldValue = {};

        // 设置第一次超时时间
        newValue.it_value = howMuchTimeFromNow(expiration);
        
        // 调用系统调用来设置定时器
        // 第一个参数是 timerfd_
        // 第二个参数 flags = 0，表示 newValue.it_value 是一个相对时间
        // 第三个参数是新的设置
        // 第四个参数是旧的设置（我们不关心，所以传 nullptr 或者一个空结构体指针）
        int ret = ::timerfd_settime(timerfd_, 0, &newValue, &oldValue);
        //0表示成功，-1表示失败
        if (ret) {
            LOG_ERROR("TimerQueue::handleRead() -> timerfd_settime()");
        }
    }

    // 辅助函数：计算从现在到指定超时时间的时间差
    static struct timespec howMuchTimeFromNow(MiniMuduo::base::Timestamp when) {
    // 获取从 Epoch 到现在的微秒数
    int64_t microseconds = when.microSecondsSinceEpoch()
                         - MiniMuduo::base::Timestamp::now().microSecondsSinceEpoch();
    
    // 如果时间差小于 100 微秒，我们就把它当作立即超时
    if (microseconds < 100) {
        microseconds = 100;
    }
    
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(
        microseconds / MiniMuduo::base::Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(
        (microseconds % MiniMuduo::base::Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts;
    }
}
}