#include"llt_muduo/net/Poller.h"
#include"llt_muduo/net/EPollPoller.h"
#include"llt_muduo/base/Logger.h"
namespace llt_muduo{
namespace net{
//工厂模式，不放在Poller或者EPollPoller中，避免头文件相互包含依赖
//提高模块化，减少耦合
Poller *Poller::newDefaultPoller(EventLoop *loop)
{
    if(::getenv("MUDUO_USE_POLL"))
    {
        LOG_FATAL("Poll don't support now");
        return nullptr;
    }else{
        return new EPollPoller(loop);
    }
}

}
}