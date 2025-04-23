#pragma once

#include<functional>
#include<vector>
#include<atomic>
#include<memory>
#include<mutex>

#include "llt_muduo/base/Timestamp.h"
#include "llt_muduo/base/noncopyable.h"
#include "llt_muduo/base/CurrentThread.h"

namespace llt_muduo{
    namespace net
    {
        class Channel;
        class Poller;
        class EventLoop:base::noncopyable{
            public:
                using Functor = std::function<void()>;

                EventLoop();
                ~EventLoop();

                void loop();
                void quit();

                base::Timestamp pollReturnTime() const{return pollReturnTime_;}

                void runInLoop(Functor cb);

                void queueInLoop(Functor cb);

                void wakeup();

                void updateChannel(Channel *channel);
                void removeChannel(Channel *channel);
                bool hasChannel(Channel *channel);

                bool isInLoopThread() const {return threadId_ == llt_muduo::base::CurrentThread::tid();}
            
            private:
                base::Timestamp pollReturnTime_;

                const pid_t threadId_;

                void handleRead();
                void doPendingFunctors();

                using ChannelList = std::vector<Channel*>;
                ChannelList activeChannels_;
                
                std::unique_ptr<Poller> poller_;
                std::unique_ptr<Channel> wakeupChannel_;

                std::atomic_bool callingPendingFunctors_;
                std::vector<Functor> pendingFunctors_;
                std::mutex mutex_;

                std::atomic_bool looping_;
                std::atomic_bool quit_;
                
                
                
        };


    } // namespace net
    
}