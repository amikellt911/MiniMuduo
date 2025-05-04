#pragma once

#include <vector>
#include <functional>
#include <assert.h>
#include <memory>
#include <string>

#include "llt_muduo/base/noncopyable.h"

namespace llt_muduo
{
    namespace net
    {
        class EventLoop;
        class EventLoopThread;
        class EventLoopThreadPool : noncopyable
        {
            public:
                //server传递的回调,server调用start，会调用pool的start，然后pool创建nums个线程（EventLoopThread）,new的时候传入回调，然后在Thread调用threadFunc时调用回调
                using ThreadInitCallback = std::function<void(EventLoop*)>;

                EventLoopThreadPool(EventLoop *baseLoop,const std::string &nameArg);
                ~EventLoopThreadPool();

                void setThreadNum(int nums)
                {
                    assert(0 <= nums);
                    numThreads = nums;
                }

                void start(const ThreadInitCallback &cb = ThreadInitCallback());

                EventLoop *getNextLoop();

                std::vector<EventLoop*> getAllLoops();

                bool started() const
                {
                    return started_;
                }

                const std::string& name() const
                {
                    return name_;
                }

            private:
                EventLoop *baseLoop_;
                std::string name_;
                bool started_;
                int numThreads;
                //下一个loop的索引
                int next_;
                std::vector<std::unique_ptr<EventLoopThread>> threads_;
                //快速访问getnextLoop
                std::vector<EventLoop*> loops_;
        };
    } 
}