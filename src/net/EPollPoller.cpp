#include <errno.h>
#include <unistd.h>
#include <string.h> //strerror
#include <string>
#include "MiniMuduo/net/EPollPoller.h"
#include "MiniMuduo/base/Logger.h"
#include "MiniMuduo/net/Channel.h"

namespace MiniMuduo
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
                // 正常的""是const char*，是指针，相加并不会字符串拼接，不会得到你想要的结果
                LOG_FATAL(std::string("EPollPoller::EPollPoller: ") + ::strerror(errno));
            }
        }

        EPollPoller::~EPollPoller()
        {
            ::close(epollfd_);
        }

        MiniMuduo::base::Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
        {
            LOG_DEBUG(std::string("poll ") + "fd total count:" + std::to_string(channels_.size()));

            int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
            int saveErrno = errno;
            MiniMuduo::base::Timestamp receiveTime(MiniMuduo::base::Timestamp::now());
            if (numEvents > 0)
            {
                LOG_INFO(std::string("EPollPoller::poll: ") + std::to_string(numEvents) + " events happened");
                fillActiveChannels(numEvents, activeChannels);
                if (numEvents == events_.size())
                {
                    events_.resize(events_.size() * 2);
                }
            }
            else if (numEvents == 0)
            {
                LOG_DEBUG(std::string("EPollPoller::poll: ") + "timeout");
            }
            else
            {
                if (saveErrno != EINTR)
                {
                    LOG_FATAL(std::string("EPollPoller::poll: ") + ::strerror(saveErrno));
                }
            }
            return receiveTime;
        }
        void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels)
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
            }
            else
            {
                int fd = channel->fd();
                if (channel->isNoneEvent())
                {
                    update(EPOLL_CTL_DEL, channel);
                    channel->set_index(kDeleted);
                }
                else
                {
                    update(EPOLL_CTL_MOD, channel);
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
                // // +++ 调试代码 +++
                // std::cout << "EPollPoller::update: operation=" << operation
                //           << ", channel_ptr=" << static_cast<void*>(channel) // 打印指针地址
                //           << ", channel_fd=" << channel->fd()
                //           << std::endl;
                // // +++++++++++++++

            event.data.ptr = channel;
            //联合体，把指针内容修改了
            //event.data.fd = fd;
            if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
            {
                        std::cerr << "EPollPoller::update epoll_ctl error for fd=" << fd
                  << ", operation=" << operation
                  << ", errno=" << errno << " (" << strerror(errno) << ")"
                  << std::endl;
                // 如果无法删除，最坏的情况是还在错误的监听一个我们不想要对fd，但是整个程序还是可以正常运行的
                if (operation == EPOLL_CTL_DEL)
                {
                    LOG_ERROR(std::string("epoll_ctl del") + ::strerror(errno));
                }
                // 如果无法添加监控或者修改监控，那么事件驱动模型的核心功能都无法保证
                else
                {
                    LOG_FATAL(std::string("epoll_ctl add/mod") + ::strerror(errno));
                }
            }
        }

    }

}
