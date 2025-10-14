#include <sys/epoll.h>

#include "MiniMuduo/net/Channel.h"
#include "MiniMuduo/base/Logger.h"
#include "MiniMuduo/net/EventLoop.h"
namespace MiniMuduo{
    namespace net{
        const int Channel::kNoneEvent = 0;
        const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
        const int Channel::kWriteEvent = EPOLLOUT;

        Channel::Channel(EventLoop *loop, int fd)
            : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
        {
        }
        Channel::~Channel(){

        }
        
        void Channel::tie(const std::shared_ptr<void> &obj){
            tie_ = obj;
            tied_ = true;
        }
        void Channel::update(){
            loop_->updateChannel(this);
        }
        void Channel::remove(){
            loop_->removeChannel(this);
        }
        void Channel::handleEvent(MiniMuduo::base::Timestamp receiveTime){
            if(tied_){
                std::shared_ptr<void> guard = tie_.lock();
                //为什么要处理这种情况？明明channel已经被tcpconnection的unique_ptr所拥有，生命周期一样，为什么？
                //因为要预发跨连接执法的问题
                //可能前一个channel发现了什么，处理了，决定将另一个连接断开。
                //但是这个连接断开，不是立即断开，他会queueInLoop，到下一轮循环中断开。
                //但是那个连接正好也被epoll捕捉，活跃事件有channel的裸指针
                //算是一种生命周期的双保险
                //防止channel依赖的回调对象上下文还在
                //其实channel本身因为生命周期通过unique_ptr管理，同生同死，不会出现这个问题
                //但是在某些情况，可能出现连接死了，但是channel还活着的情况。
                if(guard){
                    handleEventWithGuard(receiveTime);
                }
        }
        else//false也需要的原因
        //因为没有绑定TcpConnection
        //Acceptor的listenChannel,Eventloop的wakeupChannel
        {
            handleEventWithGuard(receiveTime);
        }
    }

    void Channel::handleEventWithGuard(MiniMuduo::base::Timestamp receiveTime){
        LOG_INFO("channel handleEvent revents:"+ std::to_string(revents_) + " fd:" + std::to_string(fd_));
        //当对端关闭，并且读端关闭（没有数据可读，通常对端关闭，但是如果还有数据，还是会标记为读，等到读完，才会标记为关闭）
        if((revents_ & EPOLLHUP)&&!(revents_ & EPOLLIN)){
            if(closeCallback_){
                closeCallback_();
            }
        }
        if(revents_ & EPOLLERR){
            if(errorCallback_){
                errorCallback_();
            }

        }
        //PRI代表优先级，urgent,紧急数据
        if(revents_ & (EPOLLIN | EPOLLPRI)){
            if(readCallback_){
                readCallback_(receiveTime);
            }
        }
        if(revents_ & EPOLLOUT){
            if(writeCallback_){
                writeCallback_();
            }
        }
    }
    }
}