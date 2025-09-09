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
        callingExpiredTimers_(false),
        expiration_(0)//同理invaild
    {
        timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead,this));
        timerfdChannel_.enableReading();
        //初始不关注
        resetTimerfd(MiniMuduo::base::Timestamp::invalid());
    }

    TimerQueue::~TimerQueue() {
        // 1. 先把 Channel 从 Poller 中移除，停止所有事件通知
        timerfdChannel_.disableAll();
        timerfdChannel_.remove();
        
        // 2. 关闭文件描述符
        ::close(timerfd_);

        // 3. 清理所有 Timer 对象
        activeTimers_.clear();
        
        // 4. 清理裸指针容器
        timers_.clear();
        timerIdFind.clear();

        // loop_ 等其他成员会自动析构
    }

    void TimerQueue::addTimer(int64_t sequence,MiniMuduo::net::TimerCallBack cb, MiniMuduo::base::Timestamp when, double interval)
    {
        auto timer = std::make_unique<Timer>(sequence,std::move(cb),when,interval);
        //使用 lambda 表达式来代替 std::bind,因为bind的拷贝和unique_ptr的移动，会产生各种奇怪的错误，这也是effective C++推崇的
        //放弃bind,选择lambda
        //mutable是因为lambda的 operator默认是const的
        //非常hacky的操作，
        //因为lambda获取参数必须要是拷贝，但是unique_ptr不能拷贝，只能移动
        //所以包装一层shared拷贝
        //然后再里面在move掉
        //unique_ptr的析构函数里面有一个if(!=nullptr)再执行delete操作，但是move之后默认置空了，所以没有内存泄露的问题
        auto shared_unique_ptr=std::make_shared<std::unique_ptr<Timer>>(std::move(timer));
        loop_->runInLoop([this, shared_unique_ptr]() mutable {
            // 在 lambda 体内，调用真正的处理函数
            // 这里必须用 std::move，因为 addTimerInLoop 的参数是 unique_ptr 按值传递
            addTimerInLoop(std::move(*shared_unique_ptr));
        });
    }

    void TimerQueue::addTimerInLoop(std::unique_ptr<Timer> timer)
    { 
        loop_->assertInLoopThread();
        //move前获取
        MiniMuduo::base::Timestamp when = timer->expiration();
        Timer* p_timer = timer.get();
        timerIdFind[timer->sequence()]=p_timer;
        activeTimers_[p_timer]=std::move(timer);
        bool eraliestChanged=insert(p_timer);
        if(eraliestChanged&&!callingExpiredTimers_)
        {
            resetTimerfd(when);
        }
    }

    void TimerQueue::cancelTimer(int64_t sequence)
    {
        loop_->runInLoop([this,sequence]() {
            //如果因为这个函数太慢，导致headread被调用，那就是你的问题了，而且inloop函数里面也有一个判断
            // if (!hasTimer(sequence)) {
            //     LOG_ERROR("TimerQueue::cancelTimer() -> can not find timer");
            //     return;
            // }
            cancelTimerInLoop(sequence);
        });
    }

    void TimerQueue::cancelTimerInLoop(int64_t sequence)
    {
        loop_->assertInLoopThread();
        auto it = timerIdFind.find(sequence);
        if(it!=timerIdFind.end())
        {
            Timer* timer = it->second;
            MiniMuduo::base::Timestamp now = MiniMuduo::base::Timestamp::now();
            MiniMuduo::base::Timestamp when = timer->expiration();
            if(now+5.0*MiniMuduo::base::Timestamp::kMicroSecondsPerSecond < when)
                timer->cancel();
            else {
                Entry entry(when,timer);
                bool wasEarliest = (!timers_.empty() && timers_.begin()->second == timer);
                timers_.erase(entry);
                if (wasEarliest) {
                    if (!timers_.empty()) {
                        resetTimerfd(timers_.begin()->first);
                    } else {
                        // 所有定时器都没了，让 timerfd 失效
                        resetTimerfd(MiniMuduo::base::Timestamp::invalid()); // 传一个无效/过去的时间
                    }
                }

                timerIdFind.erase(sequence);
                activeTimers_.erase(timer);
            }
        }
        else 
        {
            LOG_ERROR("TimerQueue::cancelTimer() -> can not find timer");
        }
    }

    void TimerQueue::resetTimer(int64_t sequence,MiniMuduo::base::Timestamp when,double interval)
    {//可能要改为int作为返回值，因为reset之后,sequence会变
        loop_->runInLoop([this,sequence,when,interval]() {
            // if (!hasTimer(sequence)) {
            //     LOG_ERROR("TimerQueue::resetTimer() -> can not find timer");
            //     return;
            // }
            resetTimerInLoop(sequence,when,interval);
        });
    }
    //时间早了或者晚了，这种异常的封装处理，还是交给上层吧，底层还是不要太多的处理
    void TimerQueue::resetTimerInLoop(int64_t sequence,MiniMuduo::base::Timestamp when,double interval)
    {
        loop_->assertInLoopThread();
        auto it = timerIdFind.find(sequence);
        //防御性编程，防止lambda表达式的时间和timefd的竞态关系
        if(it!=timerIdFind.end())
        {
            // std::function<void()> cb=timerIdFind[sequence]->getCallBack();
            // cancelTimerInLoop(sequence);
            // //因为sequence是父loop持有，所以不能轻易改变
            // addTimer(sequence,std::move(cb),when,interval);
            Timer* timer=timerIdFind[sequence];
            MiniMuduo::base::Timestamp when1=timer->expiration();
            Entry entry(when1,timer);
            timers_.erase(entry);
            timers_.insert({when,timer});
            activeTimers_[timer].get()->reset(when,interval);
            resetTimerfd(timers_.begin()->second->expiration()); 
        }
        else 
        {
            LOG_ERROR("TimerQueue::resetTimer() -> can not find timer");
        }
    }

    bool TimerQueue::hasTimer(int64_t sequence)
    {
        loop_->assertInLoopThread();
        auto it = timerIdFind.find(sequence);
        if(it!=timerIdFind.end())
        {
            return true;
        }
        else
        {
            return false;
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
        //
        timers_.erase(timers_.begin(),end);
        return expired;
    }

    void TimerQueue::reset(std::vector<Entry> &expired,MiniMuduo::base::Timestamp now)
    {
        for(Entry &expiredTimer : expired)
        {
            if(expiredTimer.second->repeat()&&expiredTimer.second->isValid())
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
                //如果之前只是设置canceled标志位，那么要删除
                //如果是reset后
                //if(timerIdFind.count(expiredTimer.second->sequence()))
                size_t n2=timerIdFind.erase(expiredTimer.second->sequence());
                size_t n=activeTimers_.erase(expiredTimer.second);
                if(n+n2!=2)
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
    
    void TimerQueue::resetTimerfd(MiniMuduo::base::Timestamp expiration)
    {
        expiration_=expiration;
        struct itimerspec newValue;
        struct itimerspec oldValue;
        
        // 清零，避免包含垃圾值
        newValue = {};
        oldValue = {};

        // 设置第一次超时时间
        if(expiration!=MiniMuduo::base::Timestamp::invalid())
        newValue.it_value = howMuchTimeFromNow(expiration);
        
        // 调用系统调用来设置定时器
        // 第一个参数是 timerfd_
        // 第二个参数 flags = 0，表示 newValue.it_value 是一个相对时间
        // 第三个参数是新的设置
        // 第四个参数是旧的设置（我们不关心，所以传 nullptr 或者一个空结构体指针）
        // 传0定时器会失效
        int ret = ::timerfd_settime(timerfd_, 0, &newValue, &oldValue);
        //0表示成功，-1表示失败
        if (ret) {
            LOG_ERROR("TimerQueue::handleRead() -> timerfd_settime()");
        }
    }


}
}