#include "MiniMuduo/net/EventLoopThreadPool.h"
#include "MiniMuduo/net/EventLoopThread.h"
#include "MiniMuduo/base/Logger.h"

namespace MiniMuduo{
    namespace net{
        EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
                :baseLoop_(baseLoop),
                 name_(nameArg),
                 started_(false),
                 numThreads(0),
                 next_(0){
            assert(baseLoop != nullptr);
        }

        EventLoopThreadPool::~EventLoopThreadPool(){
            // Don't delete loop, it's stack variable
        }

        void EventLoopThreadPool::start(const ThreadInitCallback &cb){ 
            assert(!started_);
            started_ = true;

            for(int i = 0; i < numThreads; ++i){
                char buf[name_.size() + 32];
                snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
                EventLoopThread *t = new EventLoopThread(cb, buf);
                threads_.push_back(std::unique_ptr<EventLoopThread>(t));
                loops_.push_back(t->startLoop());
            }
            //如果只有baseLoop_，则直接调用回调函数
            if(numThreads == 0 && cb){
                cb(baseLoop_);
            }
        }

        //轮询
        //之所以不需要锁或者原子类为他们的线程安全负责
        //是因为他们本质上运行在同一个线程baseloop
        //当有新链接到来，他们本质是在channel中
        //其实还是在loop循环中一个一个串行（顺序）执行
        EventLoop* EventLoopThreadPool::getNextLoop(){
            //不设置成nullptr，是因为有可能只有mainReactor，没有subReactor
            EventLoop *loop = baseLoop_;

            if(!loops_.empty()){
                loop = loops_[next_];
                ++next_;
                if(next_ >= loops_.size()){
                    next_ = 0;
                }

            }
            return loop;
        }

        std::vector<EventLoop*> EventLoopThreadPool::getAllLoops(){
            if(loops_.empty())
            {
                return std::vector<EventLoop*>(1, baseLoop_);
            }
            return loops_;
        }
    }
}