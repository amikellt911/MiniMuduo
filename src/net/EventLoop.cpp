#include "llt_muduo/net/EventLoop.h"
#include "llt_muduo/net/Channel.h"
#include "llt_muduo/net/Poller.h"
#include "sys/eventfd.h"
namespace llt_muduo
{
    namespace net
    {

        __thread EventLoop *t_LoopInThisThread = nullptr;
        // 10000毫秒，10秒
        const int kPollTimeMs = 10000;
        // 先想一想构造和析构函数

        // 1.构造函数，想想需要构造哪些东西？
        // 4个bool类型变量，都初始化为false
        // pollReturnTime_默认初始化，或者不写好像也行，自动就初始化了
        // poller_，使用make_unique?
        // 先创建wakeupFd_，使用eventfd?
        // 然后再创建wakeupChannel_
        // 之后要把wakeupChannel_注册到poller_，activeChannels_，channels_中
        // 线程id也需要初始化

        EventLoop::EventLoop() : looping_(false),
                                 quit_(false),
                                 eventHandling_(false),
                                 callingPendingFunctors_(false),
                                 threadId_(llt_muduo::base::CurrentThread::tid()),
                                 wakeupFd_(createEventfd()),
                                 wakeupChannel_(new Channel(this, wakeupFd_)),
                                 poller_(Poller::newDefaultPoller(this))
        //,pollReturnTime_(Timestamp::now())
        {
            std::string msg = "EventLoop created " + std::to_string(threadId_);
            LOG_DEBUG(msg);
            if (t_LoopInThisThread)
            {
                LOG_FATAL("Another EventLoop " + std::to_string(t_LoopInThisThread->threadId_) + " exists in this thread " + std::to_string(threadId_));
            }
            else
            {
                t_LoopInThisThread = this;
            }
            wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
            // 通过enableReading(),设置自己（Channel）关心的事件
            // 再通过update(),->loop的update，->poller的update
            // 最后由poller的updateChannel完成注册添加等操作
            // 这体现了封装和职责分离的原则
            wakeupChannel_->enableReading();
            // poller_->addChannel(wakeupChannel_);
            // activeChannels_.insert(wakeupChannel_);
            // channels_.insert(std::make_pair(wakeupFd_, wakeupChannel_));
        }

        EventLoop::~EventLoop()
        {
            wakeupChannel_->disableAll();
            wakeupChannel_->remove();
            ::close(wakeupFd_);
            t_LoopInThisThread = nullptr;
            // 先别写，想想有没有什么线程安全等问题？
            //  string msg = "EventLoop " + std::to_string(threadId_) + " destructs";
            //  LOG_DEBUG(msg);
        }

        int createEventfd()
        {
            int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
            if (evtfd < 0)
            {
                LOG_FATAL("Failed in eventfd" + std::to_string(errno));
            }
            return evtfd;
        }

        void EventLoop::loop()
        {
            looping_ = true;
            quit_ = false;

            LOG_INFO("EventLoop " + std::to_string(threadId_) + " start looping");
            while (!quit_)
            {
                activeChannels_.clear();
                pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
                eventHandling_ = true;
                for (Channel *channel : activeChannels_)
                {
                    channel->handleEvent(pollReturnTime_);
                }
                eventHandling_ = false;
                doPendingFunctors();
            }
            LOG_INFO("EventLoop " + std::to_string(threadId_) + " stop looping");
            looping_ = false;
        }

        void EventLoop::quit()
        {
            quit_ = true;
            // 因为如果不是在本线程，loop可能阻塞在epoll_wait，所以需要唤醒
            // 如果在本线程，说明之前再执行某个回调函数，所以调用了这个quit，说明此时不在阻塞，在执行doPendingFunctors
            // 执行完毕后，下一次循环就会自动退出
            if (!isInLoopThread())
            {
                wakeup();
            }
        }

        void EventLoop::runInLoop(Functor cb)
        {
            if (isInLoopThread())
            {
                cb();
            }
            else
            {
                queueInLoop(std::move(cb));
            }
        }
        void EventLoop::queueInLoop(Functor cb)
        {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                pendingFunctors_.emplace_back(std::move(cb));
            }
            if (!isInLoopThread() || callingPendingFunctors_)
            {
                wakeup();
            }
        }
        void EventLoop::handleRead()
        {
            uint64_t one = 1;
            ssize_t n = ::read(wakeupFd_, &one, sizeof(one));
            if (n != sizeof(one))
            {
                LOG_ERROR("EventLoop::handleRead() reads " + std::to_string(n) + " bytes instead of 8");
            }
        }
        void EventLoop::wakeup()
        {
            uint64_t one = 1;
            ssize_t n = ::write(wakeupFd_, &one, sizeof(one));
            if (n != sizeof(one))
            {
                LOG_ERROR("EventLoop::wakeup() writes " + std::to_string(n) + " bytes instead of 8");
            }
        }
        void EventLoop::doPendingFunctors()
        {
            std::vector<Functor> functors;
            callingPendingFunctors_ = true;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                functors.swap(pendingFunctors_);
            }
            for (const Functor &functor : functors)
            {
                functor();
            }
            callingPendingFunctors_ = false;
        }
        void EventLoop::updateChannel(Channel *channel)
        {
            assert(channel->ownerLoop() == this);
            assertInLoopThread();
            poller_->updateChannel(channel);
        }
        void EventLoop::removeChannel(Channel *channel)
        {
            assert(channel->ownerLoop() == this);
            assertInLoopThread();
            if (eventHandling_)
            {
                assert(channels_.find(channel->fd()) == channels_.end() || channel->isNoneEvent());
            }
            poller_->removeChannel(channel);
        }
    }

}
