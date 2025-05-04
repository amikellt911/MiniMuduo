#include "llt_muduo/net/Poller.h"
#include "llt_muduo/net/Channel.h"
#include "llt_muduo/net/EventLoop.h"
namespace llt_muduo{
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