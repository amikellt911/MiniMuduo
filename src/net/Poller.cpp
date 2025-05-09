#include "MiniMuduo/net/Poller.h"
#include "MiniMuduo/net/Channel.h"
#include "MiniMuduo/net/EventLoop.h"
namespace MiniMuduo{
    namespace net{
        Poller::Poller(EventLoop *loop)
            :ownerLoop_(loop)
        {}

        bool Poller::hasChannel(Channel *channel) const
        {
            ownerLoop_->assertInLoopThread();
            ChannelMap::const_iterator it = channels_.find(channel->fd());
            return it != channels_.end() && it->second == channel;
        }
    }
}