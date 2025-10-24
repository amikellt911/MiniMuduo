#include "MiniMuduo/net/Acceptor.h"
#include "MiniMuduo/base/Logger.h"
#include "MiniMuduo/net/InetAddress.h"
#include "MiniMuduo/net/EventLoop.h"
        //更奇妙的地方，
        //.cpp没有EventLoop.h
        //因为.cpp只需要通过loop初始化Channel
        //.cpp没有使用loop的成员函数和成员变量的地方
        //所以东西都通过Channel来调用loop来操作
        //所以Channel.cpp里面有EventLoop.h，
        //这叫做最小头文件包含，太妙了

namespace MiniMuduo{
    namespace net{
        static int createNonblockingSocketFd()
        {
            int sockfd = ::socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,IPPROTO_TCP);
            if(sockfd < 0)
            {
                LOG_FATAL("Acceptor::createNonblockingSocketFd()");
            }
            return sockfd;
        }

        Acceptor::Acceptor(EventLoop *loop,const InetAddress &listenAddr,bool reuseport)
            :loop_(loop),
            acceptSocket_(createNonblockingSocketFd()),
            acceptChannel_(loop,acceptSocket_.fd()),
            listening_(false)
        { 
            //方便程序快速重启
            acceptSocket_.setReuseAddr(true);
            //让多个套接字可以连接同一端口，实现内核层面负载均衡
            //准确来说是多acceptor模式，如果出现单个acceptor面对新连接出现性能瓶颈的情况
            if(reuseport)
            acceptSocket_.setReusePort(true);
            acceptSocket_.bindAddress(listenAddr);
            acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));
        }

        Acceptor::~Acceptor()
        {
            acceptChannel_.disableAll();
            acceptChannel_.remove();
        }

        void Acceptor::listen()
        {
            loop_->assertInLoopThread();
            listening_ = true;
            acceptSocket_.listen();
            acceptChannel_.enableReading();//包含了update
        }

        void Acceptor::handleRead()
        { 
            loop_->assertInLoopThread();
            InetAddress peerAddr;
            int connfd = acceptSocket_.accept(&peerAddr);
            if(connfd >= 0){
                if(newConnectionCallback_){
                    newConnectionCallback_(connfd,peerAddr);
                }
                else{
                    ::close(connfd);
                }
            }
            else{
                LOG_ERROR("Acceptor::handleRead");
                if(errno == EMFILE){
                    LOG_ERROR("Acceptor::handleRead() too many open files");
                }
            }
        }
    }
}