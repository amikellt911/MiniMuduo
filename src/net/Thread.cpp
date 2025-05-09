#include"MiniMuduo/net/Thread.h"
#include"MiniMuduo/base/CurrentThread.h"
#include<assert.h>
namespace MiniMuduo{
    namespace net{
        //static和extern必须要声明类型
        std::atomic_int Thread::numCreated_(0);

        Thread::Thread(ThreadFunc func,const std::string& name)
            :started_(false),
            joined_(false),
            tid_(0),
            func_(std::move(func)),
            name_(name)
        {
            setDefaultName();
        }

        Thread::~Thread()
        {
            if(started_&&!joined_)
            {
                thread_->detach();
            }
        }

        //不在构造函数启动，而专门写一个函数，是为了创建的状态和行为的分离
        //并且也是为了异常的安全，如果在构造函数中，如果构造失败，但是线程启动了，那就坏了
        void Thread::start()
        { 
            assert(!started_);
            started_=true;
            //为什么需要这个同步机制让父进程获得tid？
            //如果不等待，可能父线程或者其他线程可能会立刻访问这个tid_
            //比如在日志记录，监控等操作中，还是为了内部的一致性，确保Thread对象在start()函数返回后处于一个信息一致完整就绪的状态
            thread_ = std::shared_ptr<std::thread>(new std::thread([&]() {
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    tid_ = MiniMuduo::base::CurrentThread::tid();
                }                               
                cond_.notify_one();
                func_();                     
            }));

            {
                std::unique_lock<std::mutex> lock(mutex_);
                cond_.wait(lock, [&]() { return tid_ != 0; });
            }
            
        }

        void Thread::join()
        {
            assert(started_);
            assert(!joined_);
            joined_=true;
            thread_->join();
        }

        void Thread::setDefaultName()
        {
            int num = ++numCreated_;
            if(name_.empty())
            {
                char buf[32];
                snprintf(buf,sizeof buf,"Thread%d",num);
                name_ = buf;
            }
        }
    }
}