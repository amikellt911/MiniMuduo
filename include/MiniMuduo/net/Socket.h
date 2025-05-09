#pragma once

#include "MiniMuduo/base/noncopyable.h"

namespace MiniMuduo {
    namespace net { 
        class InetAddress;

        class Socket : MiniMuduo::base::noncopyable{
            public:
                explicit Socket(int sockfd)
                    : sockfd_(sockfd)
                {}
                ~Socket();

                int fd() const { return sockfd_; }

                void bindAddress(const InetAddress& localaddr);
                void listen();
                int accept(InetAddress* peeraddr);
                //只关闭写端，没有关闭读端，因为可能要接受客户端的响应
                void shutdownWrite();

                //Nagle算法，负责将多个小数据包合并
                void setTcpNoDelay(bool on);
                void setReuseAddr(bool on);
                void setReusePort(bool on);
                void setKeepAlive(bool on);
            
            private:
                const int sockfd_;
        };
    }

}