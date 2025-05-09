#pragma once
#include "MiniMuduo/base/noncopyable.h"
#include "MiniMuduo/base/Logger.h"
#include "MiniMuduo/base/Timestamp.h"
#include "MiniMuduo/base/CurrentThread.h"

#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <atomic>
#include <assert.h>
#include <mutex>
namespace MiniMuduo
{
    namespace net
    {

        class Channel;
        class Poller;
        class EventLoop : MiniMuduo::base::noncopyable
        {
        public:
            using Functor = std::function<void()>;

            EventLoop();
            ~EventLoop();

            void loop();
            void quit();

            void runInLoop(Functor cb);
            void queueInLoop(Functor cb);

            void wakeup();

            void updateChannel(Channel *channel);
            void removeChannel(Channel *channel);

            void assertInLoopThread() const { assert(isInLoopThread()); };

            bool isInLoopThread() const { return threadId_ == MiniMuduo::base::CurrentThread::tid(); }

        private:
            void handleRead();
            void doPendingFunctors();

        private:
            using ChannelList = std::vector<Channel *>;
            // 是否需要shared_ptr？
            // 不需要，我们只是观察，不是共享和拥有
            using ChannelMap = std::map<int, Channel *>;

            std::atomic_bool looping_;
            std::atomic_bool quit_;
            std::atomic_bool callingPendingFunctors_;
            // 断言或调试有用，是一个好习惯
            std::atomic_bool eventHandling_;

            // 我也不知道要不要这个，自动补全帮我生成的，感觉好像是需要的陈硕的书好像也讲了，好像是记录时间，计算效率的
            // 用于记录poll返回时间，并将其传给Channel的handEvent()
            MiniMuduo::base::Timestamp pollReturnTime_;

            // Poller
            std::unique_ptr<Poller> poller_;
            // 等待队列
            std::vector<Functor> pendingFunctors_;
            // 是否需要？需要，因为是要先创建该fd，再创建Channel
            int wakeupFd_;

            // 唤醒机制
            std::unique_ptr<Channel> wakeupChannel_;

            // 线程id，是否需要，每次assert直接使用CurrentThread::tid(),还是保存一个？应该不需要吧
            // 需要,因为assert需要频繁调用，频繁调用函数CurrentThread::tid()，不如直接保存
            const pid_t threadId_;

            // Channel队列,便于Poller管理
            ChannelList activeChannels_;
            // Channel映射表，便于查找Channel
            ChannelMap channels_;

            // 锁，保证等待队列在queueinLoop的安全
            std::mutex mutex_;
        };

    } // namespace net
} // namespace MiniMuduo
