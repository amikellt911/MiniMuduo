#include <errno.h>
#include <unistd.h>
#include <string.h>//strerror
#include <string>
#include "llt_muduo/net/EPollPoller.h"
#include "llt_muduo/base/Logger.h"
#include "llt_muduo/net/Channel.h"

namespace llt_muduo
{
    namespace net
    {
        const int kNew = -1;    // channel还未添加至Poller中
        const int kAdded = 1;   // channel已添加至Poller中
        const int kDeleted = 2; // channel已从Poller中删除

        EPollPoller::EPollPoller(EventLoop *loop)
            : Poller(loop),
              epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
              events_(kInitEventListSize)
        {
            if (epollfd_ < 0)
            {
                //正常的""是const char*，是指针，相加并不会字符串拼接，不会得到你想要的结果
                LOG_FATAL(std::string("EPollPoller::EPollPoller: ") + ::strerror(errno));
            }
        }

        EPollPoller::~EPollPoller()
        {
            ::close(epollfd_);
        }

        llt_muduo::base::Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
        {
            LOG_DEBUG(std::string("poll ") + "fd total count:" + std::to_string(channels_.size()));

            int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
            int saveErrno = errno;
            llt_muduo::base::Timestamp now(llt_muduo::base::Timestamp::now());

            if (numEvents > 0)
            {
                LOG_DEBUG(std::string("EPollPoller::EPollPoller: ") + "fd total count:" + std::to_string(numEvents));
                fillActiveChannels(numEvents, activeChannels);
                if (numEvents == events_.size())
                {
                    events_.resize(events_.size() * 2);
                }
                else if (numEvents == 0)
                {
                    // 因为长时间没有新事件，自然而然是超时，是debug，而不是errno
                    LOG_DEBUG("Timeout!");
                }
                else if (numEvents < 0 && saveErrno != EINTR)
                {
                    errno = saveErrno;
                    LOG_ERROR("EPollPoller::poll");
                }
                return now;
            }
        }

        void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
        {
            for (int i = 0; i < numEvents; ++i)
            {
                Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
                channel->set_revents(events_[i].events);
                activeChannels->push_back(channel);
            }
        }

        void EPollPoller::updateChannel(Channel *channel)
        {
            const int index = channel->index();
            LOG_DEBUG("EPollPoller::updateChannel fd=" + std::to_string(channel->fd()) + " events=" + std::to_string(channel->events()));
            if (index == kNew || index == kDeleted)
            {
                int fd = channel->fd();
                if (index == kNew)
                {
                    channels_[fd] = channel;
                }
                else
                {
                    
                } 
                channel->set_index(kAdded);
                update(EPOLL_CTL_ADD, channel);
            }else
            {
                int fd=channel->fd();
                if(channel->isNoneEvent()){
                    update(EPOLL_CTL_DEL,channel);
                    channel->set_index(kDeleted);
                }   
                else{
                    update(EPOLL_CTL_MOD,channel);
                }
            }
        }

        void EPollPoller::removeChannel(Channel *channel)
        {
            int fd = channel->fd();
            int index = channel->index();
            LOG_DEBUG("fd=" + std::to_string(fd) + " events=" + std::to_string(channel->events()));
            channels_.erase(fd);
            if (index == kAdded)
            {
                update(EPOLL_CTL_DEL, channel);
            }
            channel->set_index(kDeleted);
        }

        void EPollPoller::update(int operation, Channel *channel)
        {
            struct epoll_event event;
            ::memset(&event, 0, sizeof(event));
            int fd = channel->fd();
            event.events = channel->events();
            event.data.ptr = channel;
            event.data.fd = fd;
            if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
            {
                //如果无法删除，最坏的情况是还在错误的监听一个我们不想要对fd，但是整个程序还是可以正常运行的
                if(operation==EPOLL_CTL_DEL){
                    LOG_ERROR(std::string("epoll_ctl del")+::strerror(errno));
                }
                //如果无法添加监控或者修改监控，那么事件驱动模型的核心功能都无法保证
                else{
                    LOG_FATAL(std::string("epoll_ctl add/mod")+::strerror(errno));

                }
            }
        }

    }

}
