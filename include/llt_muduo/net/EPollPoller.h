#pragma once

#include<vector>
#include<sys/epoll.h>

#include"MiniMuduo/net/Poller.h"
#include"MiniMuduo/base/Timestamp.h"

namespace MiniMuduo
{
    namespace net
    {
        class Channel;
        class EPollPoller:public Poller
        {
            public:
                EPollPoller(EventLoop *loop);
                ~EPollPoller() override;

                MiniMuduo::base::Timestamp poll(int timeoutMs,ChannelList *activeChannels) override;
                void removeChannel(Channel *channel) override;
                void updateChannel(Channel *channel) override;

            private:
                static const int kInitEventListSize = 16;

                void fillActiveChannels(int numEvents,ChannelList *activeChannels);
                void update(int operation,Channel *channel);

                using EventList = std::vector<epoll_event>;

                int epollfd_;
                EventList events_;
        };
    }
}