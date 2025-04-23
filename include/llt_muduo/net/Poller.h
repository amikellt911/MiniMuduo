#pragma once

#include<vector>
#include<unordered_map>

#include "llt_muduo/base/Timestamp.h"
#include "llt_muduo/base/noncopyable.h"

namespace llt_muduo
{
    namespace net{
        class Channel;
        class EventLoop;

        class Poller:base::noncopyable{
            public:
                using ChannelList = std::vector<Channel*>;

                Poller(EventLoop *loop);
                virtual ~Poller()=default;

                virtual base::Timestamp poll(int timeoutMs,ChannelList *activeChannels)=0;
                virtual void updateChannel(Channel *channel)=0;
                virtual void removeChannel(Channel *Channel)=0;

                bool hasChannel(Channel *channel) const;

                static Poller *newDefaultPoller(EventLoop *loop);

            protected:
                using ChannelMap=std::unordered_map<int,Channel *>;
                ChannelMap channels_;
            private:
                EventLoop *ownerLoop_;
            
        };
    }
}