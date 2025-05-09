#include "MiniMuduo/net/EventLoopThread.h"
#include "MiniMuduo/net/EventLoop.h"

namespace MiniMuduo
{
    namespace net
    {
        //这种传参传引用，只要你存储的及时，没用引用类型作为存储就行（虽然这会多一层拷贝），但是对于这种多线程的生命周期不明显的情况，这是值得的
         EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name)
         : loop_(nullptr),
           exiting_(false),
           thread_(std::bind(&EventLoopThread::threadFunc, this), name),
           mutex_(),
           cond_(),
           callback_(cb)
         {
         }

         EventLoopThread::~EventLoopThread()
         {
            exiting_ = true;
             if (loop_ != nullptr)
             {
                 loop_->quit();
                 thread_.join();
             }
         }

         EventLoop* EventLoopThread::startLoop()
         {
              thread_.start();
              EventLoop *loop = nullptr;
              {
                std::unique_lock<std::mutex> lock(mutex_);
                cond_.wait(lock, [this](){
                    return loop_ != nullptr;
                });
                loop = loop_;
              }
              //为什么要返回一个stack变量，而不是成员变量loop_
              //因为很有可能threadFunc也就是loop()，极速退出了
              //但是我们仍然loop的值，不管作为日志还是什么
              //loop局部，而不是loop_，因为threadFunc和startloop是并发执行，可能会threadFunc先指向完，导致loop_为空
              return loop;
         }

         void EventLoopThread::threadFunc()
         {
             EventLoop loop;
             if(callback_){
                callback_(&loop);
             }
             {
                 std::unique_lock<std::mutex> lock(mutex_);
                 loop_ = &loop;
                 cond_.notify_one();
             }
             //阻塞着，一开始可能看不出来
             loop.loop();
             std::unique_lock<std::mutex> lock(mutex_);
             loop_ = nullptr;
         }
    } 
}