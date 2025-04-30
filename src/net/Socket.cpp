
#include"Socket.h"
#include"InetAddress.h"
#include"llt_muduo/base/Logger.h"

namespace llt_muduo {
    namespace net {
        Socket::~Socket(){
            ::close(sockfd_);
        }

        void Socket::bindAddress(const InetAddress& localaddr){
            if(::bind(sockfd_, (sockaddr*)localaddr.getSockAddr(), sizeof(sockaddr_in)) != 0){
                //FATAL而不是ERROR，因为绑定连接失败这种问题非常严重
                LOG_FATAL("bind sockfd error"+localaddr.toIpPort()+std::to_string(sockfd_)+"errno:"+std::to_string(errno));
            }
        }

        void Socket::listen(){
            //listen失败不是超时，通常是资源配置存在致命问题
            if(::listen(sockfd_, 1024) != 0){
                LOG_FATAL("listen sockfd error"+std::to_string(sockfd_)+"errno:"+std::to_string(errno));
            }
        }

        int Socket::accept(InetAddress* peeraddr){
            sockaddr_in addr;
            socklen_t len = sizeof(addr);
            ::memset(&addr, 0, sizeof(addr));
            //accept4,一次系统调用=accept+fcntl的两次调用，原子性
            //但是仅在linux可用，其他系统如macos还需手动调用fcntl
            //4值得是第4个参数，而不是指这是第4个accept系列函数
            int connfd = ::accept4(sockfd_, (sockaddr*)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
            if(connfd >= 0){
                peeraddr->setSockAddr(addr);
            }
            return connfd;
        }

        void Socket::shutdownWrite(){
            if(::shutdown(sockfd_, SHUT_WR) < 0){
                LOG_ERROR("shutdownWrite error");
            }
        }

        void Socket::setTcpNoDelay(bool on){
            int optval = on ? 1 : 0;
            ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
        }

        void Socket::setReuseAddr(bool on){
            int optval = on ? 1 : 0;
            ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        }

        void Socket::setReusePort(bool on){
            int optval = on ? 1 : 0;
            ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        }

        void Socket::setKeepAlive(bool on){
            int optval = on ? 1 : 0;
            ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
        }
    }

}