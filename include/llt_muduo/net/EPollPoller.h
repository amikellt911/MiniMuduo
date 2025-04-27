#pragma once

#include<vector>
#include<sys/epoll.h>

#include"Poller.h"
#include"base/Timestamp.h"

namespace llt_muduo
{
    namespace net
    {
        class Channel;
        class EPollPoller:public Poller
        {
            public:
                EPollPoller(EventLoop *loop);
                ~EPollPoller() override;

                base::Timestamp poll(int timeoutMs,ChannelList *activeChannels) override;
                void removeChannel(Channel *channel) override;
                void updateChannel(Channel *channel) override;

            private:
                static const int kInitEventListSize = 16;

                void fillActiveChannels(int numEvents,ChannelList *activeChannels) const;
                void update(int operation,Channel *channel);

                using EventList = std::vector<epoll_event>;

                int epollfd_;
                EventList events_;
        }
    }
}