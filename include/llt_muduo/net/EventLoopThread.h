#pragma once 

#include <functional>
#include <mutex>
#include <condition_variable>

#include "MiniMuduo/base/noncopyable.h"
#include "MiniMuduo/net/Thread.h"
#include "MiniMuduo/net/EventLoop.h"

namespace MiniMuduo
{
    namespace net
    {
        
        class EventLoop;
        class EventLoopThread : MiniMuduo::base::noncopyable
        {
            public:
                using ThreadInitCallback = std::function<void(EventLoop*)>;

                explicit EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                                const std::string& name = std::string());

                ~EventLoopThread();

                //获取loop_，顺便包装了thread的start
                EventLoop* startLoop();

            private:
                void threadFunc();

                EventLoop* loop_;
                bool exiting_;
                Thread thread_;
                std::mutex mutex_;  
                std::condition_variable cond_;
                ThreadInitCallback callback_;          

        };

    } // namespace net
     
}

