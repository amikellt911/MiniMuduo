#include <sys/epoll.h>

#include "llt_muduo/net/Channel.h"
#include "llt_muduo/base/Logger.h"
#include "llt_muduo/net/EventLoop.h"
namespace llt_muduo{
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
        void Channel::handleEvent(base::Timestamp receiveTime){
            if(tied_){
                std::shared_ptr<void> guard = tie_.lock();
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

    void Channel::handleEventWithGuard(base::Timestamp receiveTime){
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